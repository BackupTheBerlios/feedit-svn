// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#define REFRESH_INTERVAL (5*60*1000)

class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CTrayIconImpl<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler,
	public IWorkerThreadClient,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDocHostUIHandlerDispatch, &IID_IDocHostUIHandlerDispatch, &LIBID_ATLLib>,
	public IDispEventImpl<0, CMainFrame, &SHDocVw::DIID_DWebBrowserEvents2, &SHDocVw::LIBID_SHDocVw, 1, 0>
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	TCHAR m_dbPath[MAX_PATH];
	CCommandBarCtrl m_CmdBar;
	CMultiPaneStatusBarCtrl m_statusBar;
	CProgressBarCtrl m_progressBar;
	CSplitterWindow m_vSplit;
	CHorSplitterWindow m_hSplit;
	CCustomTreeViewCtrl m_treeView;
	CCustomListViewCtrl m_listView;
	CAxWindow m_htmlView;
	CComPtr<IWebBrowser2> m_htmlCtrl;
	CComPtr<ADODB::_Connection> m_connection;
	CImageList m_dragImage;
	int m_downloads;
	BOOL m_dragging;
	BOOL m_mustRefresh;
	int m_newItems;
	HTREEITEM m_feedsRoot;
	HTREEITEM m_itemDrag;
	HTREEITEM m_itemDrop;
	HCURSOR m_arrowCursor;
	HCURSOR m_noCursor;
	CWorkerThread<> m_updateThread;
	HANDLE m_hTimer;

	CMainFrame() : m_downloads(0), m_dragging(FALSE), m_mustRefresh(FALSE), m_newItems(0),
		m_feedsRoot(NULL), m_itemDrag(NULL), m_itemDrop(NULL), m_arrowCursor(LoadCursor(NULL, IDC_ARROW)),
		m_noCursor(LoadCursor(NULL, IDC_NO))
	{
		::SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, m_dbPath);
		::PathAppend(m_dbPath, "FeedIt");

		if(::GetFileAttributes(m_dbPath) == INVALID_FILE_ATTRIBUTES)
		{
			::CreateDirectory(m_dbPath, NULL);
		}

		::PathAppend(m_dbPath, "FeedIt.mdb");

		if(::GetFileAttributes(m_dbPath) == INVALID_FILE_ATTRIBUTES)
		{
			CComPtr<ADOX::_Catalog> catalog;
			catalog.CoCreateInstance(CComBSTR("ADOX.Catalog"));
			ATLASSERT(catalog != NULL);
			catalog->Create(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath);
			CComPtr<ADODB::_Connection> connection;
			connection.CoCreateInstance(CComBSTR("ADODB.Connection"));
			ATLASSERT(connection != NULL);
			connection->Open(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath, _bstr_t(), _bstr_t(), 0);
			connection->Execute(_bstr_t("CREATE TABLE Folders (ID AUTOINCREMENT UNIQUE NOT NULL, Name VARCHAR(255) NOT NULL)"), NULL, 0);
			connection->Execute(_bstr_t("CREATE TABLE Feeds (ID AUTOINCREMENT UNIQUE NOT NULL, FolderID INTEGER NOT NULL, Name VARCHAR(255) NOT NULL, URL VARCHAR(255) NOT NULL, LastUpdate DATETIME NOT NULL, RefreshInterval INTEGER NOT NULL, MaxAge INTEGER NOT NULL, NavigateURL VARCHAR(1) NOT NULL)"), NULL, 0);
			connection->Execute(_bstr_t("CREATE TABLE News (ID AUTOINCREMENT UNIQUE NOT NULL, FeedID INTEGER NOT NULL, Title VARCHAR(255) NOT NULL, URL VARCHAR(255) NOT NULL, Issued DATETIME NOT NULL, Description MEMO, Unread VARCHAR(1) NOT NULL, Flagged VARCHAR(1) NOT NULL, CONSTRAINT NewsC1 UNIQUE (FeedID, URL))"), NULL, 0);
		}
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(m_htmlView.IsChild(pMsg->hwnd))
		{
			CComPtr<IOleInPlaceActiveObject> pIOAO;
			m_htmlView.QueryControl(&pIOAO);
			ATLASSERT(pIOAO != NULL);

			if(pIOAO->TranslateAccelerator(pMsg) == S_OK)
				return TRUE;
		}

		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return FALSE;
	}

	void RefreshTree(HTREEITEM root = NULL)
	{
		if(root == NULL)
		{
			root = m_feedsRoot;
		}

		HTREEITEM i = m_treeView.GetChildItem(root);

		while(i != NULL)
		{
			FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));

			if(feeddata != NULL)
			{
				feeddata->m_unread = GetUnreadItemCount(feeddata->m_id);
				CAtlString txt;
				m_treeView.GetItemText(i, txt);
				m_treeView.SetItemText(i, txt);
			}
			else
			{
				RefreshTree(i);
			}

			i = m_treeView.GetNextSiblingItem(i);
		}

		if(root == m_feedsRoot)
		{
			m_treeView.Invalidate();
		}
	}

	HTREEITEM GetFeedTreeItem(int feedid, HTREEITEM root = NULL)
	{
		if(root == NULL)
		{
			root = m_feedsRoot;
		}

		HTREEITEM i = m_treeView.GetChildItem(root);
		HTREEITEM ret = NULL;

		while(i != NULL && ret == NULL)
		{
			FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));

			if(feeddata != NULL && feeddata->m_id == feedid)
			{
				ret = i;
			}
			else
			{
				ret = GetFeedTreeItem(feedid, i);
			}

			i = m_treeView.GetNextSiblingItem(i);
		}

		return ret;
	}

	void RefreshList()
	{
		HTREEITEM i = m_treeView.GetSelectedItem();
		FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));
		FolderData* folderdata = dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i));

		if(feeddata != NULL || folderdata != NULL || i == m_feedsRoot)
		{
			{
				CAtlMap<int, bool> entrymap;
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued";
				}

				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

				if(!recordset->EndOfFile)
				{
					recordset->MoveFirst();

					while(!recordset->EndOfFile)
					{
						entrymap[(int)recordset->Fields->GetItem("News.ID")->Value] = true;
						recordset->MoveNext();
					}
				}

				for(int idx = 0; idx < m_listView.GetItemCount(); ++idx)
				{
					NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

					if(!entrymap.Lookup(newsdata->m_id))
					{
						m_listView.DeleteItem(idx--);
						delete newsdata;
					}
					else
					{
						entrymap.RemoveKey(newsdata->m_id);
					}
				}

				if(!recordset->BOF)
				{
					recordset->MoveFirst();

					while(!recordset->EndOfFile)
					{
						if(entrymap.Lookup((int)recordset->Fields->GetItem("News.ID")->Value))
						{
							NewsData* newsdata = new NewsData();
							newsdata->m_id = recordset->Fields->GetItem("News.ID")->Value;
							newsdata->m_url = recordset->Fields->GetItem("News.URL")->Value;
							newsdata->m_title = recordset->Fields->GetItem("Title")->Value;
							newsdata->m_description = recordset->Fields->GetItem("Description")->Value;
							newsdata->m_issued = recordset->Fields->GetItem("Issued")->Value;
							newsdata->m_feedTreeItem = GetFeedTreeItem(recordset->Fields->GetItem("Feeds.ID")->Value);
							feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));

							if((_bstr_t)recordset->Fields->GetItem("Unread")->Value == _bstr_t("1"))
							{
								m_listView.InsertItem(0, newsdata->m_issued, 1);
								newsdata->m_unread = true;
							}
							else
							{
								m_listView.InsertItem(0, newsdata->m_issued, 0);
								newsdata->m_unread = false;
							}

							m_listView.AddItem(0, 1, CAtlString(recordset->Fields->GetItem("Title")->Value));
							m_listView.AddItem(0, 2, feeddata->m_name);
							m_listView.SetItemData(0, (DWORD_PTR)newsdata);
						}

						recordset->MoveNext();
					}
				}
			}
		}
	}

	virtual BOOL OnIdle()
	{
		if(m_mustRefresh)
		{
			if(m_newItems > 0)
			{
				CAtlString str;
				str.Format("There are %d new items", m_newItems);
				Notify(str, "FeedIt update");
				RefreshTree();
				RefreshList();
				m_newItems = 0;
			}

			m_mustRefresh = FALSE;
			LARGE_INTEGER liDueTime;
			liDueTime.QuadPart = -10000 * (__int64) REFRESH_INTERVAL;
			::SetWaitableTimer(m_hTimer, &liDueTime, 0,  NULL, NULL, FALSE);
		}

		HTREEITEM i = m_treeView.GetSelectedItem();
		FeedData* feeddata = NULL;
		FolderData* folderdata = NULL;

		if(i != NULL)
		{
			feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));
			folderdata = dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i));
		}

		if(i != NULL && i != m_feedsRoot && m_treeView.GetChildItem(i) == NULL)
			UISetState(ID_FILE_DELETE, UPDUI_ENABLED);
		else
			UISetState(ID_FILE_DELETE, UPDUI_DISABLED);

		if(feeddata != NULL)
			UISetState(ID_VIEW_PROPERTIES, UPDUI_ENABLED);
		else
			UISetState(ID_VIEW_PROPERTIES, UPDUI_DISABLED);

		if(feeddata != NULL || folderdata != NULL || i == m_feedsRoot)
			UISetState(ID_ACTIONS_MARKREAD, UPDUI_ENABLED);
		else
			UISetState(ID_ACTIONS_MARKREAD, UPDUI_DISABLED);

		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_FILE_DELETE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_PROPERTIES, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_ACTIONS_MARKREAD, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_ACTIONS_BACK, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_ACTIONS_FORWARD, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		NOTIFY_HANDLER(IDC_TREE, TVN_BEGINDRAG, OnBeginDrag)
		NOTIFY_HANDLER(IDC_TREE, TVN_SELCHANGED, OnTreeSelectionChanged)
		NOTIFY_HANDLER(IDC_TREE, TVN_BEGINLABELEDIT, OnTreeBeginLabelEdit)
		NOTIFY_HANDLER(IDC_TREE, TVN_ENDLABELEDIT, OnTreeEndLabelEdit)
		NOTIFY_HANDLER(IDC_LIST, LVN_ITEMCHANGED, OnListSelectionChanged)
		COMMAND_ID_HANDLER(ID_SHOW, OnShow)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW_FEED, OnFileNewFeed)
		COMMAND_ID_HANDLER(ID_FILE_NEW_FOLDER, OnFileNewFolder)
		COMMAND_ID_HANDLER(ID_FILE_DELETE, OnFileDelete)
		COMMAND_ID_HANDLER(ID_VIEW_PROPERTIES, OnViewProperties)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKREAD, OnActionsMarkRead)
		COMMAND_ID_HANDLER(ID_ACTIONS_BACK, OnActionsBack)
		COMMAND_ID_HANDLER(ID_ACTIONS_FORWARD, OnActionsForward)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_NEXT_PANE, OnNextPane)
		COMMAND_ID_HANDLER(ID_PREV_PANE, OnPrevPane)
		CHAIN_MSG_MAP_MEMBER(m_treeView)
		CHAIN_MSG_MAP_MEMBER(m_listView)
		CHAIN_MSG_MAP(CTrayIconImpl<CMainFrame>)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

	BEGIN_COM_MAP(CMainFrame)
		COM_INTERFACE_ENTRY(IDocHostUIHandlerDispatch)
	END_COM_MAP()

	BEGIN_SINK_MAP(CMainFrame)
		SINK_ENTRY_EX(0, SHDocVw::DIID_DWebBrowserEvents2, DISPID_DOWNLOADBEGIN, OnDownloadBegin)
		SINK_ENTRY_EX(0, SHDocVw::DIID_DWebBrowserEvents2, DISPID_DOWNLOADCOMPLETE, OnDownloadComplete)
		SINK_ENTRY_EX(0, SHDocVw::DIID_DWebBrowserEvents2, DISPID_PROGRESSCHANGE, OnProgressChange)
		SINK_ENTRY_EX(0, SHDocVw::DIID_DWebBrowserEvents2, DISPID_COMMANDSTATECHANGE, OnCommandStateChange)
		SINK_ENTRY_EX(0, SHDocVw::DIID_DWebBrowserEvents2, DISPID_STATUSTEXTCHANGE, OnStatusTextChange)
	END_SINK_MAP()

	HRESULT CloseHandle(HANDLE hObject)
	{
		if (!::CloseHandle(hObject))
			return AtlHresultFromLastError();

		return S_OK;
	}

	HRESULT Execute(DWORD_PTR /*dwParam*/, HANDLE /*hObject*/)
	{
		::CoInitialize(NULL);
		CComPtr<ADODB::_Recordset> recordset;
		recordset.CoCreateInstance(CComBSTR("ADODB.Recordset"));
		ATLASSERT(recordset != NULL);
		recordset->CursorLocation = ADODB::adUseServer;
		SYSTEMTIME t;
		::GetSystemTime(&t);
		COleDateTime now(t);
		recordset->Open(_bstr_t("Feeds"), _variant_t(m_connection), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();

			while(!recordset->EndOfFile)
			{
				COleDateTime t1;
				t1.ParseDateTime((_bstr_t)recordset->Fields->GetItem("LastUpdate")->Value);
				COleDateTimeSpan s1(0, 0, recordset->Fields->GetItem("RefreshInterval")->Value, 0);

				if(t1+s1 < now)
				{
					int feedid = recordset->Fields->GetItem("ID")->Value;
					_bstr_t url = recordset->Fields->GetItem("URL")->Value;
					GetFeedNews(feedid, url);
					recordset->Fields->GetItem("LastUpdate")->Value = (BSTR)CComBSTR(now.Format("%Y/%m/%d %H:%M:%S"));
					recordset->Update();
				}

				recordset->MoveNext();
			}
		}

		::CoUninitialize();
		m_mustRefresh = TRUE;
		PostMessage(WM_NULL);
		return S_OK;
	}

	HTREEITEM MoveChildItem(HTREEITEM hItem, HTREEITEM htiNewParent, HTREEITEM htiAfter)
	{
		TV_INSERTSTRUCT tvstruct;
		HTREEITEM hNewItem;
		CAtlString sText;

		// get information of the source item
		tvstruct.item.hItem = hItem;
		tvstruct.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		m_treeView.GetItem(&tvstruct.item);
		m_treeView.GetItemText(hItem, sText);

		tvstruct.item.cchTextMax = sText.GetLength();
		tvstruct.item.pszText = sText.LockBuffer();

		//insert the item at proper location
		tvstruct.hParent = htiNewParent;
		tvstruct.hInsertAfter = htiAfter;
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
		hNewItem = m_treeView.InsertItem(&tvstruct);
		sText.ReleaseBuffer();

		//now copy item data and item state.
		m_treeView.SetItemData(hNewItem, m_treeView.GetItemData(hItem));
		m_treeView.SetItemState(hNewItem, m_treeView.GetItemState(hItem, TVIS_STATEIMAGEMASK), TVIS_STATEIMAGEMASK);

		//now delete the old item
		m_treeView.DeleteItem(hItem);

		return hNewItem;
	}

	CAtlString SniffFeedName(const _bstr_t& url)
	{
		CComPtr<MSXML2::IXMLDOMDocument2> xmldocument;
		xmldocument.CoCreateInstance(CComBSTR("Msxml2.DOMDocument"));
		ATLASSERT(xmldocument != NULL);
		xmldocument->async = FALSE;
		xmldocument->setProperty(_bstr_t("SelectionLanguage"), _variant_t("XPath"));
		xmldocument->setProperty(_bstr_t("SelectionNamespaces"), _variant_t("xmlns:rss09=\"http://my.netscape.com/rdf/simple/0.9/\" xmlns:rss10=\"http://purl.org/rss/1.0/\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:atom=\"http://purl.org/atom/ns#\""));
		xmldocument->load(_variant_t(url));
		CComPtr<MSXML2::IXMLDOMNode> node = xmldocument->selectSingleNode(_bstr_t("/rss/channel"));

		if(node != NULL)
		{
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("title"));

			if(titlenode != NULL)
				return CAtlString(titlenode->text.GetBSTR());
			else
				return CAtlString("(No name)");
		}

		node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss09:channel"));

		if(node != NULL)
		{
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss09:title"));

			if(titlenode != NULL)
				return CAtlString(titlenode->text.GetBSTR());
			else
				return CAtlString("(No name)");
		}

		node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss10:channel"));

		if(node != NULL)
		{
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss10:title"));

			if(titlenode != NULL)
				return CAtlString(titlenode->text.GetBSTR());
			else
				return CAtlString("(No name)");
		}

		node = xmldocument->selectSingleNode(_bstr_t("/atom:feed"));

		if(node != NULL)
		{
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("atom:title"));

			if(titlenode != NULL)
				return CAtlString(titlenode->text.GetBSTR());
			else
				return CAtlString("(No name)");
		}

		return CAtlString("(Unknown feed type)");
	}

	void AddNewsToFeed(int feedid, const _bstr_t& title, const _bstr_t& url, const _bstr_t& description, const _bstr_t& date)
	{
		try
		{
			CAtlString t1((const char*)date);
			COleDateTime dt(atoi(t1.Mid(0, 4)), atoi(t1.Mid(5, 2)), atoi(t1.Mid(8, 2)), atoi(t1.Mid(11, 2)), atoi(t1.Mid(14, 2)), atoi(t1.Mid(17, 2)));

			if(t1.Mid(19, 1) == "+")
				dt -= COleDateTimeSpan(0, atoi(t1.Mid(20, 2)), atoi(t1.Mid(23, 2)), 0);

			if(t1.Mid(19, 1) == "-")
				dt += COleDateTimeSpan(0, atoi(t1.Mid(20, 2)), atoi(t1.Mid(23, 2)), 0);

			CAtlString t2 = dt.Format("%Y/%m/%d %H:%M:%S");

			CComPtr<ADODB::_Recordset> recordset;
			recordset.CoCreateInstance(CComBSTR("ADODB.Recordset"));
			ATLASSERT(recordset != NULL);
			recordset->CursorLocation = ADODB::adUseServer;
			recordset->Open(_bstr_t("News"), _variant_t(m_connection), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
			recordset->AddNew();
			recordset->Fields->GetItem("FeedID")->Value = feedid;
			recordset->Fields->GetItem("Title")->Value = title;
			recordset->Fields->GetItem("URL")->Value = url;
			recordset->Fields->GetItem("Description")->Value = description;
			recordset->Fields->GetItem("Issued")->Value = _bstr_t(t2);
			recordset->Fields->GetItem("Unread")->Value = _bstr_t("1");
			recordset->Fields->GetItem("Flagged")->Value = _bstr_t("0");
			recordset->Update();
			++m_newItems;
		}
		catch(...)
		{
		}
	}

	void GetFeedNews(int feedid, const _bstr_t& url)
	{
		CComPtr<MSXML2::IXMLDOMDocument2> xmldocument;
		xmldocument.CoCreateInstance(CComBSTR("Msxml2.DOMDocument"));
		ATLASSERT(xmldocument != NULL);
		xmldocument->async = FALSE;
		xmldocument->setProperty(_bstr_t("SelectionLanguage"), _variant_t("XPath"));
		xmldocument->setProperty(_bstr_t("SelectionNamespaces"), _variant_t("xmlns:rss09=\"http://my.netscape.com/rdf/simple/0.9/\" xmlns:rss10=\"http://purl.org/rss/1.0/\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:atom=\"http://purl.org/atom/ns#\""));
		xmldocument->load(_variant_t(url));

		CComPtr<MSXML2::IXMLDOMNodeList> nodes = xmldocument->selectNodes(_bstr_t("/rss/channel/item"));

		if(nodes != NULL && nodes->length > 0)
		{
			CComPtr<MSXML2::IXMLDOMNode> node;

			while((node = nodes->nextNode()) != NULL)
			{
				CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("title"));
				CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("link"));
				CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("description"));
				CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("date"));
				AddNewsToFeed(feedid, titlenode->text, urlnode->text, descriptionnode->text, datenode->text);
			}

			return;
		}

		nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss09:item"));

		if(nodes != NULL && nodes->length > 0)
		{
			CComPtr<MSXML2::IXMLDOMNode> node;

			while((node = nodes->nextNode()) != NULL)
			{
				CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss09:title"));
				CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("rss09:link"));
				CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("rss09:description"));
				CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("dc:date"));
				AddNewsToFeed(feedid, titlenode->text, urlnode->text, descriptionnode->text, datenode->text);
			}

			return;
		}

		nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss10:item"));

		if(nodes != NULL && nodes->length > 0)
		{
			CComPtr<MSXML2::IXMLDOMNode> node;

			while((node = nodes->nextNode()) != NULL)
			{
				CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss10:title"));
				CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("rss10:link"));
				CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("rss10:description"));
				CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("dc:date"));
				AddNewsToFeed(feedid, titlenode->text, urlnode->text, descriptionnode->text, datenode->text);
			}

			return;
		}

		nodes = xmldocument->selectNodes(_bstr_t("/atom:feed/atom:entry"));

		if(nodes != NULL && nodes->length > 0)
		{
			CComPtr<MSXML2::IXMLDOMNode> baseurlnode = xmldocument->selectSingleNode(_bstr_t("/atom:feed/atom:link[@rel=\"alternate\"]/@href"));
			CComPtr<MSXML2::IXMLDOMNode> node;

			while((node = nodes->nextNode()) != NULL)
			{
				CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("atom:title"));
				CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("atom:link[@rel=\"alternate\"]/@href"));
				CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("atom:content"));
				CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("atom:issued"));
				TCHAR buf[1024];
				DWORD buflen = 1024;
				AtlCombineUrl(baseurlnode->text, urlnode->text, buf, &buflen, ATL_URL_NO_ENCODE);
				AddNewsToFeed(feedid, titlenode->text, buf, descriptionnode->xml, datenode->text);
			}

			return;
		}
	}

	int GetUnreadItemCount(int feedid)
	{
		CComPtr<ADODB::_Command> command;
		command.CoCreateInstance(CComBSTR("ADODB.Command"));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT COUNT(*) AS ItemCount FROM News WHERE FeedID=? AND Unread='1'";
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feedid)));
		CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();
			return (int)recordset->Fields->GetItem("ItemCount")->Value;
		}

		return 0;
	}

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// create command bar window
		HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
		// attach menu
		m_CmdBar.AttachMenu(GetMenu());
		// load command bar images
		m_CmdBar.LoadImages(IDR_MAINFRAME);
		// remove old menu
		SetMenu(NULL);

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

		CreateSimpleStatusBar();
		m_statusBar.SubclassWindow(m_hWndStatusBar);
		int panes[] = { ID_DEFAULT_PANE, IDR_PROGRESS };
		m_statusBar.SetPanes(panes, sizeof(panes) / sizeof(int), false);
		m_statusBar.SetPaneWidth(IDR_PROGRESS, 100);
		m_progressBar.Create(m_statusBar.m_hWnd, rcDefault, NULL, WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		UIAddToolBar(hWndToolBar);
		UISetCheck(ID_VIEW_TOOLBAR, 1);
		UISetCheck(ID_VIEW_STATUS_BAR, 1);

		// client rect for vertical splitter
		CRect rcVert;
		GetClientRect(&rcVert);

		// create the vertical splitter
		m_vSplit.Create(m_hWnd, rcVert, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		m_vSplit.SetSplitterExtendedStyle(0);

		// client rect for horizontal splitter
		CRect rcHorz;
		GetClientRect(&rcHorz);

		// create the horizontal splitter. Note that vSplit is parent of hSplit
		m_hSplit.Create(m_vSplit.m_hWnd, rcHorz, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		m_hSplit.SetSplitterExtendedStyle(0);

		// add the horizontal splitter to the right pane (1) of vertical splitter
		m_vSplit.SetSplitterPane(1, m_hSplit);

		m_treeView.Create(m_vSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS, WS_EX_CLIENTEDGE);
		::SendMessage(m_treeView.m_hWnd, CCM_SETVERSION, 5, 0);
		m_treeView.SetDlgCtrlID(IDC_TREE);
		CImageList tvil;
		tvil.Create(IDB_TREE_IMAGELIST, 16, 2, RGB(192, 192, 192));
		m_treeView.SetImageList(tvil.Detach(), TVSIL_NORMAL);
		m_vSplit.SetSplitterPane(0, m_treeView);

		m_listView.Create(m_hSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
		::SendMessage(m_listView.m_hWnd, CCM_SETVERSION, 5, 0);
		m_listView.SetDlgCtrlID(IDC_LIST);
		CImageList lvil;
		lvil.Create(IDB_LIST_IMAGELIST, 16, 2, RGB(192, 192, 192));
		m_listView.SetImageList(lvil.Detach(), LVSIL_SMALL);
		m_hSplit.SetSplitterPane(0, m_listView);

		m_htmlView.Create(m_hSplit.m_hWnd, rcDefault, _T("about:blank"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE);
		m_hSplit.SetSplitterPane(1, m_htmlView);
		m_htmlView.SetExternalUIHandler(static_cast<IDocHostUIHandlerDispatch*>(this));
		m_htmlView.QueryControl(&m_htmlCtrl);
		DispEventAdvise(m_htmlCtrl);

		m_hWndClient = m_vSplit.m_hWnd;

		// set the vertical splitter parameters
		m_vSplit.m_cxyMin = 100; // minimum size
		m_vSplit.SetSplitterPos(150); // from left

		// set the horizontal splitter parameters
		m_hSplit.m_cxyMin = 100; // minimum size
		m_hSplit.SetSplitterPos(150); // from top

		m_feedsRoot = m_treeView.InsertItem(_T("Feeds"), TVI_ROOT, TVI_LAST);
		m_treeView.SetItemImage(m_feedsRoot, 1, 1);

		m_connection.CoCreateInstance(CComBSTR("ADODB.Connection"));
		ATLASSERT(m_connection != NULL);
		m_connection->Open(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath, _bstr_t(), _bstr_t(), 0);
		CComPtr<ADODB::_Command> command;
		command.CoCreateInstance(CComBSTR("ADODB.Command"));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT * FROM Folders";
		CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();

			while(!recordset->EndOfFile)
			{
				FolderData* folderitemdata = new FolderData();
				folderitemdata->m_id = recordset->Fields->GetItem("ID")->Value;
				folderitemdata->m_name = recordset->Fields->GetItem("Name")->Value;
				HTREEITEM folderitem = m_treeView.InsertItem(folderitemdata->m_name, m_feedsRoot, TVI_LAST);
				m_treeView.SetItemImage(folderitem, 1, 1);
				m_treeView.SetItemData(folderitem, (DWORD_PTR)folderitemdata);
				CComPtr<ADODB::_Command> subcommand;
				subcommand.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(subcommand != NULL);
				subcommand->ActiveConnection = m_connection;
				subcommand->CommandText = "SELECT * FROM Feeds WHERE FolderID=?";
				subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(folderitemdata->m_id)));
				CComPtr<ADODB::_Recordset> subrecordset = subcommand->Execute(NULL, NULL, 0);

				if(!subrecordset->EndOfFile)
				{
					subrecordset->MoveFirst();

					while(!subrecordset->EndOfFile)
					{
						FeedData* feeditemdata = new FeedData();
						feeditemdata->m_id = subrecordset->Fields->GetItem("ID")->Value;
						feeditemdata->m_name = subrecordset->Fields->GetItem("Name")->Value;
						feeditemdata->m_unread = GetUnreadItemCount(feeditemdata->m_id);
						feeditemdata->m_navigateURL = atoi(_bstr_t(subrecordset->Fields->GetItem("NavigateURL")->Value));
						HTREEITEM feeditem = m_treeView.InsertItem(feeditemdata->m_name, folderitem, TVI_LAST);
						m_treeView.SetItemImage(feeditem, 0, 0);
						m_treeView.SetItemData(feeditem, (DWORD_PTR)feeditemdata);
						subrecordset->MoveNext();
					}
				}

				m_treeView.Expand(folderitem);
				recordset->MoveNext();
			}
		}

		CComPtr<ADODB::_Command> subcommand;
		subcommand.CoCreateInstance(CComBSTR("ADODB.Command"));
		ATLASSERT(subcommand != NULL);
		subcommand->ActiveConnection = m_connection;
		subcommand->CommandText = "SELECT * FROM Feeds WHERE FolderID=0";
		CComPtr<ADODB::_Recordset> subrecordset = subcommand->Execute(NULL, NULL, 0);

		if(!subrecordset->EndOfFile)
		{
			subrecordset->MoveFirst();

			while(!subrecordset->EndOfFile)
			{
				int id = subrecordset->Fields->GetItem("ID")->Value;
				FeedData* feeditemdata = new FeedData();
				feeditemdata->m_id = subrecordset->Fields->GetItem("ID")->Value;
				feeditemdata->m_name = subrecordset->Fields->GetItem("Name")->Value;
				feeditemdata->m_unread = GetUnreadItemCount(feeditemdata->m_id);
				feeditemdata->m_navigateURL = atoi(_bstr_t(subrecordset->Fields->GetItem("NavigateURL")->Value));
				HTREEITEM feeditem = m_treeView.InsertItem(feeditemdata->m_name, m_feedsRoot, TVI_LAST);
				m_treeView.SetItemImage(feeditem, 0, 0);
				m_treeView.SetItemData(feeditem, (DWORD_PTR)feeditemdata);
				subrecordset->MoveNext();
			}
		}

		m_treeView.Expand(m_feedsRoot);
		m_treeView.SortChildren(m_feedsRoot, TRUE);

		m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
		m_listView.AddColumn("Date", 0);
		m_listView.AddColumn("Title", 1);
		m_listView.AddColumn("Feed", 2);
		m_listView.SetColumnWidth(0, 150);
		m_listView.SetColumnWidth(1, 500);
		m_listView.SetColumnWidth(2, 150);

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		InstallIcon(_T("FeedIt"), hIconSmall, IDR_TRAY_POPUP);

		CReBarSettings rs;
		CReBarCtrl rbc = m_hWndToolBar;

		if(rs.Load("Software\\FeedIt", "ReBar"))
			rs.ApplyTo(rbc);

		CSplitterSettings ss1;

		if(ss1.Load("Software\\FeedIt", "HSplit"))
			ss1.ApplyTo(m_hSplit);

		CSplitterSettings ss2;

		if(ss2.Load("Software\\FeedIt", "VSplit"))
			ss2.ApplyTo(m_vSplit);

		m_updateThread.Initialize();
		m_hTimer = ::CreateWaitableTimer(NULL, FALSE, NULL);
		m_updateThread.AddHandle(m_hTimer, this, NULL);
		LARGE_INTEGER liDueTime;
		liDueTime.QuadPart = -10000;
		::SetWaitableTimer(m_hTimer, &liDueTime, 0,  NULL, NULL, FALSE);

		m_treeView.SelectItem(m_feedsRoot);
		return 0;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		m_updateThread.Shutdown();

		CWindowSettings ws;
		ws.GetFrom(*this);
		ws.Save("Software\\FeedIt", "MainFrame");

		CReBarSettings rs;
		CReBarCtrl rbc = m_hWndToolBar;
		rs.GetFrom(rbc);
		rs.Save("Software\\FeedIt", "ReBar");

		CSplitterSettings ss1;
		ss1.GetFrom(m_hSplit);
		ss1.Save("Software\\FeedIt", "HSplit");

		CSplitterSettings ss2;
		ss2.GetFrom(m_vSplit);
		ss2.Save("Software\\FeedIt", "VSplit");

		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		if(wParam == SIZE_MINIMIZED)
		{
			ShowWindow(SW_HIDE);
			LONG_PTR exstyle = GetWindowLongPtr(GWL_EXSTYLE);
			exstyle |= WS_EX_TOOLWINDOW;
			SetWindowLongPtr(GWL_EXSTYLE, exstyle);
		}
		else
		{
			if(::IsWindow(m_statusBar.m_hWnd))
			{
				UpdateLayout();
				CRect rcProgress;
				m_statusBar.GetPaneRect(IDR_PROGRESS, &rcProgress);
				::InflateRect(&rcProgress, 6, -2);
				::OffsetRect(&rcProgress, 6, 0);
				m_progressBar.MoveWindow(rcProgress);
			}
		}

		return 0;
	}

	LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = FALSE;
		UINT flags = wParam;
		CPoint point(lParam);

		if(m_dragging)
		{
			CPoint pt = point;
			ClientToScreen(&pt);
			CImageList::DragMove(pt);
			point = pt;
			m_treeView.ScreenToClient(&point);
			HTREEITEM hitem;
			if((hitem = m_treeView.HitTest(point, &flags)) != NULL)
			{
				CImageList::DragShowNolock(FALSE);
				bool isFolder = (dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(hitem)) != NULL);

				if(hitem == m_feedsRoot || isFolder)
				{
					if(::GetCursor() != m_arrowCursor)
						::SetCursor(m_arrowCursor);
					m_treeView.SelectDropTarget(hitem);
					m_itemDrop = hitem;
				}
				else if(::GetCursor() != m_noCursor)
				{
					::SetCursor(m_noCursor);
					m_treeView.SelectDropTarget(NULL);
					m_itemDrop = NULL;
				}

				CImageList::DragShowNolock(TRUE);
			}
			else if(::GetCursor() != m_noCursor)
			{
				::SetCursor(m_noCursor);
				m_treeView.SelectDropTarget(NULL);
				m_itemDrop = NULL;
			}
		}
		else 
		{
			::SetCursor(m_arrowCursor);
		}

		return 0;
	}

	LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		bHandled = FALSE;
		UINT flags = wParam;
		CPoint point(lParam);

		if(m_dragging)
		{
			ClientToScreen(&point);
			m_treeView.ScreenToClient(&point);
			m_dragging = FALSE;
			CImageList::DragLeave(m_hWnd);
			CImageList::EndDrag();
			ReleaseCapture();
			m_dragImage.Destroy();

			// Remove drop target highlighting
			m_treeView.SelectDropTarget(NULL);

			if(m_itemDrag == m_itemDrop)
				return 0;

			HTREEITEM hitem;
			if(((hitem = m_treeView.HitTest(point, &flags)) == NULL))
				return 0;

			bool isFolder = (dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(hitem)) != NULL);

			if(hitem == m_feedsRoot || isFolder)
			{
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Feeds SET FolderID=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(m_itemDrop))->m_id)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(m_itemDrag))->m_id)));
				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				m_treeView.Expand(m_itemDrop, TVE_EXPAND);
				HTREEITEM htiNew = MoveChildItem(m_itemDrag, m_itemDrop, TVI_LAST);
				m_treeView.DeleteItem(m_itemDrag);
				m_treeView.SelectItem(htiNew);
			}
		}

		return 0;
	}

	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		CPoint pt(lParam);
		CPoint npt(-1, -1);
		CRect rc;
		m_treeView.GetWindowRect(&rc);

		if((pt == npt && (m_treeView.m_hWnd == ::GetFocus() || ::IsChild(m_treeView.m_hWnd, ::GetFocus()))) || rc.PtInRect(pt))
		{
			if(pt == npt)
			{
				CRect r;
				HTREEITEM i = m_treeView.GetSelectedItem();

				if(i != NULL)
				{
					m_treeView.GetItemRect(i, &r, TRUE);
					pt = r.TopLeft();
				}
				else
				{
					pt = rc.TopLeft();
				}

				m_treeView.ClientToScreen(&pt);
			}

			CMenu menu;
			if(!menu.LoadMenu(IDR_TREE_POPUP))
				return 0;
			CMenuHandle popup(menu.GetSubMenu(0));
			PrepareMenu(popup);
			SetForegroundWindow(m_hWnd);
			popup.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
			menu.DestroyMenu();
		}

		/*
		m_listView.GetWindowRect(&rc);

		if(rc.PtInRect(pt))
		{
			CMenu menu;
			if(!menu.LoadMenu(IDR_LIST_POPUP))
				return 0;
			CMenuHandle popup(menu.GetSubMenu(0));
			PrepareMenu(popup);
			CPoint pos;
			GetCursorPos(&pos);
			SetForegroundWindow(m_hWnd);
			popup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, m_hWnd);
			menu.DestroyMenu();
		}
		*/

		return 0;
	}

	LRESULT OnBeginDrag(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;
		bool isFeed = (dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(pNMTreeView->itemNew.hItem)) != NULL);

		if(!isFeed)
			return 0;

		m_itemDrag = pNMTreeView->itemNew.hItem;
		m_itemDrop = NULL;
		m_dragImage = m_treeView.CreateDragImage(m_itemDrag);

		if(m_dragImage == NULL)
			return 0;

		m_dragging = TRUE;
		m_dragImage.BeginDrag(0, CPoint(0, 0));
		CPoint pt = pNMTreeView->ptDrag;
		m_treeView.ClientToScreen(&pt);
		m_dragImage.DragEnter(NULL, pt);
		SetCapture();
		return 0;
	}

	LRESULT OnTreeSelectionChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		for(int idx = 0; idx < m_listView.GetItemCount(); ++idx)
		{
			delete dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));
		}

		m_listView.DeleteAllItems();
		_variant_t url("about:blank");
		HTREEITEM i = m_treeView.GetSelectedItem();
		FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));
		FolderData* folderdata = dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i));

		if(feeddata != NULL || folderdata != NULL || i == m_feedsRoot)
		{
			{
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued";
				}

				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

				if(!recordset->EndOfFile)
				{
					recordset->MoveFirst();

					while(!recordset->EndOfFile)
					{
						NewsData* newsdata = new NewsData();
						newsdata->m_id = recordset->Fields->GetItem("News.ID")->Value;
						newsdata->m_url = recordset->Fields->GetItem("News.URL")->Value;
						newsdata->m_title = recordset->Fields->GetItem("Title")->Value;
						newsdata->m_description = recordset->Fields->GetItem("Description")->Value;
						newsdata->m_issued = recordset->Fields->GetItem("Issued")->Value;
						newsdata->m_feedTreeItem = GetFeedTreeItem(recordset->Fields->GetItem("Feeds.ID")->Value);
						feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));

						if((_bstr_t)recordset->Fields->GetItem("Unread")->Value == _bstr_t("1"))
						{
							m_listView.InsertItem(0, newsdata->m_issued, 1);
							newsdata->m_unread = true;
						}
						else
						{
							m_listView.InsertItem(0, newsdata->m_issued, 0);
							newsdata->m_unread = false;
						}

						m_listView.AddItem(0, 1, CAtlString(recordset->Fields->GetItem("Title")->Value));
						m_listView.AddItem(0, 2, feeddata->m_name);
						m_listView.SetItemData(0, (DWORD_PTR)newsdata);
						recordset->MoveNext();
					}
				}
			}

			TCHAR tmpPath[MAX_PATH];
			::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, tmpPath);
			::PathAppend(tmpPath, "FeedIt");

			if(::GetFileAttributes(tmpPath) == INVALID_FILE_ATTRIBUTES)
			{
				::CreateDirectory(tmpPath, NULL);
			}

			::PathAppend(tmpPath, "Temp.htm");
			HANDLE hFile = ::CreateFile(tmpPath, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if(hFile != NULL)
			{
				CAtlString tmp;
				WriteLine(hFile, "<html>");
				WriteLine(hFile, "<head>");
				WriteLine(hFile, "<META http-equiv=\"Content-Type\" content=\"text/html\">");
				WriteLine(hFile, "<title>News Item</title>");
				WriteLine(hFile, "<style type=\"text/css\">");
				// WriteLine(hFile, "\thtml { border: 6px solid gray; }");
				WriteLine(hFile, "\tbody { background-color: white; color: black; font: 84% Verdana, Arial, sans-serif; margin: 12px 22px; }");
				WriteLine(hFile, "\ttable { font: 100% Verdana, Arial, sans-serif; }");
				WriteLine(hFile, "\ta { color: #0002CA; }");
				WriteLine(hFile, "\ta:hover { color: #6B8ADE; text-decoration: underline; }");
				WriteLine(hFile, "\tspan.nodescription { color: silver; font-size: smaller; }");
				WriteLine(hFile, "");
				WriteLine(hFile, "\th1,h2,h3,h4,h5,h6 { font-size: 80%; font-weight: bold; font-style: italic;	}");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.favchannel {");
				WriteLine(hFile, "\t\toverflow: hidden;");
				WriteLine(hFile, "\t\tborder: 1px solid #6B8ADE;");
				WriteLine(hFile, "\t\tbackground-color: #D6DFF7;");
				WriteLine(hFile, "\t\tfloat: left;");
				WriteLine(hFile, "\t\tmargin: 0 14px 10px 0;");
				WriteLine(hFile, "\t\tpadding: 16px 16px 0 16px; width: 40%;");
				WriteLine(hFile, "\t}");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.newspapertitle { font-weight: bold; font-size: 120%; text-align: center; padding-bottom: 12px; text-transform: uppercase; }");
				WriteLine(hFile, "\tdiv.channel { border-bottom: 1px dotted silver; margin-top: 14px}");
				WriteLine(hFile, "\tdiv.channeltitle { font-weight: bold; text-transform: uppercase; margin-top: 12px; margin-bottom: 18px;}");
				WriteLine(hFile, "\timg.channel { border: none; }");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.newsitemcontent { line-height: 140%; text-align: justify; }");
				WriteLine(hFile, "\tdiv.newsitemcontent ol, div.newsitemcontent ul { list-style-position: inside;}");
				WriteLine(hFile, "\tdiv.newsitemtitle { font-weight: bold; margin-bottom: 8px }");
				WriteLine(hFile, "\tdiv.newsitemfooter { color: gray; font-size: xx-small; text-align: left; margin-top: 6px; margin-bottom: 18px; }");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.newsitemtitle a, div.newsitemcontent a, div.newsitemfooter a { text-decoration: none; }");
				WriteLine(hFile, "</style><style>");
				WriteLine(hFile, "\tdiv.newspapertitle { margin-bottom: 12px; border-bottom: 1px dashed silver; }");
				WriteLine(hFile, "</style>");
				WriteLine(hFile, "</head>");
				WriteLine(hFile, "<body>");

				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued DESC";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued DESC";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued DESC";
				}

				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

				if(!recordset->EndOfFile)
				{
					recordset->MoveFirst();
					int x = 0;

					while(!recordset->EndOfFile)
					{
						if(x % 2 == 0)
							WriteLine(hFile, "<table width=\"100%\"><tr><td width=\"90%\">");
						else
							WriteLine(hFile, "<table width=\"100%\"><tr><td width=\"10%\">&nbsp;</td><td width=\"75%\">");

						CAtlString tmp;
						tmp.Format("\t<div class=\"newsitemtitle\"><a href=\"%s\">%s</a></div>\n", (const char*)(_bstr_t)recordset->Fields->GetItem("News.URL")->Value, (const char*)(_bstr_t)recordset->Fields->GetItem("Title")->Value);
						WriteLine(hFile, tmp);
						tmp.Format("\t<div class=\"newsitemcontent\">%s</div>\n", (const char*)(_bstr_t)recordset->Fields->GetItem("Description")->Value);
						WriteLine(hFile, tmp);
						tmp.Format("\t<div class=\"newsitemfooter\">Received %s</div>", (const char*)(_bstr_t)recordset->Fields->GetItem("Issued")->Value);
						WriteLine(hFile, tmp);

						if(x % 2 == 0)
							WriteLine(hFile, "</td><td width=\"10%\">&nbsp;</td></tr></table>");
						else
							WriteLine(hFile, "</td></tr></table>");

						++x;
						recordset->MoveNext();
					}
				}

				WriteLine(hFile, "</body>");
				WriteLine(hFile, "</html>");
				::CloseHandle(hFile);
				url = tmpPath;
			}
		}

		_variant_t v;
		m_htmlCtrl->Navigate2(&url, &v, &v, &v, &v);
		return 0;
	}

	LRESULT OnTreeBeginLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		NMTVDISPINFO* pTVDI = reinterpret_cast<NMTVDISPINFO*>(pnmh);
		TreeData* treedata = (TreeData*)m_treeView.GetItemData(pTVDI->item.hItem);
		bool isFeed = (dynamic_cast<FeedData*>(treedata) != NULL);
		bool isFolder = (dynamic_cast<FolderData*>(treedata) != NULL);

		if(isFolder || isFeed)
			return FALSE;
		else
			return TRUE;
	}

	LRESULT OnTreeEndLabelEdit(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		NMTVDISPINFO* pTVDI = reinterpret_cast<NMTVDISPINFO*>(pnmh);

		if(pTVDI->item.pszText != NULL)
		{
			TreeData* treedata = (TreeData*)m_treeView.GetItemData(pTVDI->item.hItem);
			bool isFeed = (dynamic_cast<FeedData*>(treedata) != NULL);
			bool isFolder = (dynamic_cast<FolderData*>(treedata) != NULL);

			if(isFeed)
			{
				FeedData* feeddata = dynamic_cast<FeedData*>(treedata);
				feeddata->m_name = pTVDI->item.pszText;
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Feeds SET Name=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, CComVariant(feeddata->m_name)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				return TRUE;
			}
			else if(isFolder)
			{
				FolderData* folderdata = dynamic_cast<FolderData*>(treedata);
				folderdata->m_name = pTVDI->item.pszText;
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Folders SET Name=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, CComVariant(folderdata->m_name)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(folderdata->m_id)));
				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				return TRUE;
			}

			m_treeView.SortChildren(m_feedsRoot, TRUE);
		}

		return FALSE;
	}

	void WriteLine(HANDLE hFile, const char* str)
	{
		DWORD written = 0;
		::WriteFile(hFile, str, strlen(str), &written, NULL);
		::WriteFile(hFile, "\r\n", 2, &written, NULL);
	}

	LRESULT OnListSelectionChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL)
		{
			FeedData* feeddata = dynamic_cast<FeedData*>((FeedData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));
			_variant_t url;

			if(feeddata->m_navigateURL == 1 || (feeddata->m_navigateURL == 0 && strlen(newsdata->m_description) < 60))
			{
				url = newsdata->m_url;
			}
			else
			{
				TCHAR tmpPath[MAX_PATH];
				::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, tmpPath);
				::PathAppend(tmpPath, "FeedIt");

				if(::GetFileAttributes(tmpPath) == INVALID_FILE_ATTRIBUTES)
				{
					::CreateDirectory(tmpPath, NULL);
				}

				::PathAppend(tmpPath, "Temp.htm");
				HANDLE hFile = ::CreateFile(tmpPath, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

				if(hFile != NULL)
				{
					CAtlString tmp;
					WriteLine(hFile, "<html>");
					WriteLine(hFile, "<head>");
					WriteLine(hFile, "<META http-equiv=\"Content-Type\" content=\"text/html\">");
					WriteLine(hFile, "<title>News Item</title>");
					WriteLine(hFile, "<style type=\"text/css\">");
					// WriteLine(hFile, "\thtml { border: 6px solid gray; }");
					WriteLine(hFile, "\tbody { background-color: white; color: black; font: 84% Verdana, Arial, sans-serif; margin: 12px 22px; }");
					WriteLine(hFile, "\ta { color: #0002CA; }");
					WriteLine(hFile, "\ta:hover { color: #6B8ADE; text-decoration: underline; }");
					WriteLine(hFile, "\tspan.nodescription {	color: silver;	font-size: smaller; }");
					WriteLine(hFile, "");
					WriteLine(hFile, "\th1,h2,h3,h4,h5,h6 { font-size: 80%; font-weight: bold; font-style: italic;	}");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.favchannel {");
					WriteLine(hFile, "\t\toverflow: hidden;");
					WriteLine(hFile, "\t\tborder: 1px solid #6B8ADE;");
					WriteLine(hFile, "\t\tbackground-color: #D6DFF7;");
					WriteLine(hFile, "\t\tfloat: left;");
					WriteLine(hFile, "\t\tmargin: 0 14px 10px 0;");
					WriteLine(hFile, "\t\tpadding: 16px 16px 0 16px; width: 40%;");
					WriteLine(hFile, "\t}");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.newspapertitle { font-weight: bold; font-size: 120%; text-align: center; padding-bottom: 12px; text-transform: uppercase; }");
					WriteLine(hFile, "\tdiv.channel { border-bottom: 1px dotted silver; margin-top: 14px}");
					WriteLine(hFile, "\tdiv.channeltitle { font-weight: bold; text-transform: uppercase; margin-top: 12px; margin-bottom: 18px;}");
					WriteLine(hFile, "\timg.channel { border: none; }");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.newsitemcontent { line-height: 140%; }");
					WriteLine(hFile, "\tdiv.newsitemcontent ol, div.newsitemcontent ul { list-style-position: inside;}");
					WriteLine(hFile, "\tdiv.newsitemtitle { font-weight: bold; margin-bottom: 8px }");
					WriteLine(hFile, "\tdiv.newsitemfooter { color: gray; font-size: xx-small; text-align: left; margin-top: 6px; margin-bottom: 18px; }");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.newsitemtitle a, div.newsitemcontent a, div.newsitemfooter a { text-decoration: none; }");
					WriteLine(hFile, "</style><style>");
					WriteLine(hFile, "\tdiv.newsitemtitle { border-bottom: 1px dotted silver; margin-bottom: 10px; padding-bottom: 10px;}");
					WriteLine(hFile, "\tdiv.newsitemfooter { text-align: right; margin-top: 14px; padding-top: 6px; border-top: 1px dashed #CBCBCB; }");
					WriteLine(hFile, "</style>");
					WriteLine(hFile, "</head>");
					WriteLine(hFile, "<body>");
					tmp.Format("\t<div class=\"newsitemtitle\"><a href=\"%s\">%s</a></div>\n", (const char*)newsdata->m_url, (const char*)newsdata->m_title);
					WriteLine(hFile, tmp);
					tmp.Format("\t<div class=\"newsitemcontent\">%s</div>\n", (const char*)newsdata->m_description);
					WriteLine(hFile, tmp);
					tmp.Format("\t<div class=\"newsitemfooter\">%s<br>Received %s</div>", (const char*)feeddata->m_name, (const char*)newsdata->m_issued);
					WriteLine(hFile, tmp);
					WriteLine(hFile, "</body>");
					WriteLine(hFile, "</html>");
					::CloseHandle(hFile);
					url = tmpPath;
				}
			}

			_variant_t v;
			m_htmlCtrl->Navigate2(&url, &v, &v, &v, &v);

			if(newsdata->m_unread)
			{
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE News SET Unread='0' WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(newsdata->m_id)));
				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				newsdata->m_unread = false;
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);
				feeddata->m_unread--;
				CAtlString txt;
				m_treeView.GetItemText(newsdata->m_feedTreeItem, txt);
				m_treeView.SetItemText(newsdata->m_feedTreeItem, txt);
				m_listView.Invalidate();
			}
		}

		return 0;
	}

	LRESULT OnShow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		WINDOWPLACEMENT plc;
		plc.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(&plc);

		if(plc.showCmd == SW_SHOWMINIMIZED)
		{
			LONG_PTR exstyle = GetWindowLongPtr(GWL_EXSTYLE);
			exstyle &= ~WS_EX_TOOLWINDOW;
			SetWindowLongPtr(GWL_EXSTYLE, exstyle);
			ShowWindow(SW_RESTORE);
		}
		else
		{
			ShowWindow(SW_SHOW);
		}

		return 0;
	}

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnFileNewFeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStringDlg dlg(_T("Enter feed URL:"));
		dlg.m_value = _T("http://");

		if(dlg.DoModal() == IDOK)
		{
			CAtlString name = SniffFeedName((BSTR)CComBSTR(dlg.m_value));
			CComPtr<ADODB::_Recordset> recordset;
			recordset.CoCreateInstance(CComBSTR("ADODB.Recordset"));
			ATLASSERT(recordset != NULL);
			recordset->CursorLocation = ADODB::adUseServer;
			recordset->Open(_bstr_t("Feeds"), _variant_t(m_connection), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
			recordset->AddNew();
			recordset->Fields->GetItem("FolderID")->Value = 0;
			recordset->Fields->GetItem("Name")->Value = _bstr_t(name);
			recordset->Fields->GetItem("URL")->Value = _bstr_t(dlg.m_value);
			recordset->Fields->GetItem("LastUpdate")->Value = _bstr_t("2000/01/01 00:00:00");
			recordset->Fields->GetItem("RefreshInterval")->Value = 60;
			recordset->Fields->GetItem("MaxAge")->Value = 0;
			recordset->Fields->GetItem("NavigateURL")->Value = _bstr_t("0");
			recordset->Update();
			FeedData* itemdata = new FeedData();
			itemdata->m_id = recordset->Fields->GetItem("ID")->Value;
			itemdata->m_name = name;
			itemdata->m_unread = 0;
			HTREEITEM item = m_treeView.InsertItem(name, m_feedsRoot, TVI_LAST);
			m_treeView.SetItemImage(item, 0, 0);
			m_treeView.SetItemData(item, (DWORD_PTR)itemdata);
			m_treeView.SortChildren(m_feedsRoot, TRUE);
			m_treeView.Expand(m_feedsRoot);
			LARGE_INTEGER liDueTime;
			liDueTime.QuadPart = -10000 * 10;
			::SetWaitableTimer(m_hTimer, &liDueTime, 0,  NULL, NULL, FALSE);
		}

		return 0;
	}

	LRESULT OnFileNewFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStringDlg dlg(_T("Enter folder name:"));
		dlg.m_value = _T("New");

		if(dlg.DoModal() == IDOK)
		{
			CComPtr<ADODB::_Recordset> recordset;
			recordset.CoCreateInstance(CComBSTR("ADODB.Recordset"));
			ATLASSERT(recordset != NULL);
			recordset->CursorLocation = ADODB::adUseServer;
			recordset->Open(_bstr_t("Folders"), _variant_t(m_connection), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
			recordset->AddNew();
			recordset->Fields->GetItem("Name")->Value = _bstr_t(dlg.m_value);
			recordset->Update();
			FolderData* itemdata = new FolderData();
			itemdata->m_id = recordset->Fields->GetItem("ID")->Value;
			itemdata->m_name = dlg.m_value;
			HTREEITEM item = m_treeView.InsertItem(dlg.m_value, m_feedsRoot, TVI_LAST);
			m_treeView.SetItemImage(item, 1, 1);
			m_treeView.SetItemData(item, (DWORD_PTR)itemdata);
			m_treeView.SortChildren(m_feedsRoot, TRUE);
			m_treeView.Expand(m_feedsRoot);
		}

		return 0;
	}

	LRESULT OnFileDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		HTREEITEM i = m_treeView.GetSelectedItem();

		if(i != NULL && i != m_feedsRoot && m_treeView.GetChildItem(i) == NULL)
		{
			if(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i)) != NULL)
			{
				{
					CComPtr<ADODB::_Command> command;
					command.CoCreateInstance(CComBSTR("ADODB.Command"));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "DELETE FROM News WHERE FeedID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
					CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				}

				{
					CComPtr<ADODB::_Command> command;
					command.CoCreateInstance(CComBSTR("ADODB.Command"));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "DELETE FROM Feeds WHERE ID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
					CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
				}
			}
			else if(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i)) != NULL)
			{
				CComPtr<ADODB::_Command> command;
				command.CoCreateInstance(CComBSTR("ADODB.Command"));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "DELETE FROM Folders WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
				CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
			}

			delete (TreeData*)m_treeView.GetItemData(i);
			m_treeView.DeleteItem(i);
		}

		return 0;
	}

	LRESULT OnViewProperties(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		HTREEITEM i = m_treeView.GetSelectedItem();
		FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));

		if(feeddata != NULL)
		{
			CComPtr<ADODB::_Command> command;
			command.CoCreateInstance(CComBSTR("ADODB.Command"));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "SELECT * FROM Feeds WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
			CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);

			if(!recordset->EndOfFile)
			{
				recordset->MoveFirst();
				CFeedPropertySheet sheet("Feed properties", 0);
				sheet.m_propertiesPage.m_name = recordset->Fields->GetItem("Name")->Value;
				sheet.m_propertiesPage.m_url = recordset->Fields->GetItem("URL")->Value;
				sheet.m_propertiesPage.m_update = recordset->Fields->GetItem("RefreshInterval")->Value;

				if(sheet.DoModal() == IDOK)
				{
					CComPtr<ADODB::_Command> subcommand;
					subcommand.CoCreateInstance(CComBSTR("ADODB.Command"));
					ATLASSERT(subcommand != NULL);
					subcommand->ActiveConnection = m_connection;
					subcommand->CommandText = "UPDATE Feeds SET Name=?, URL=?, RefreshInterval=? WHERE ID=?";
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, CComVariant(sheet.m_propertiesPage.m_name)));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, CComVariant(sheet.m_propertiesPage.m_url)));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(sheet.m_propertiesPage.m_update)));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(feeddata->m_id)));
					CComPtr<ADODB::_Recordset> subrecordset = subcommand->Execute(NULL, NULL, 0);
					feeddata->m_name = sheet.m_propertiesPage.m_name;
					m_treeView.SetItemText(i, feeddata->m_name);
					m_treeView.SortChildren(m_feedsRoot, TRUE);
				}
			}
		}

		return 0;
	}

	LRESULT OnActionsMarkRead(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		HTREEITEM i = m_treeView.GetSelectedItem();
		FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i));
		FolderData* folderdata = dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i));

		if(feeddata != NULL || folderdata != NULL || i == m_feedsRoot)
		{
			for(int idx = 0; idx < m_listView.GetItemCount(); ++idx)
			{
				NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

				if(newsdata != NULL && newsdata->m_unread)
				{
					CComPtr<ADODB::_Command> command;
					command.CoCreateInstance(CComBSTR("ADODB.Command"));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "UPDATE News SET Unread='0' WHERE ID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, CComVariant(newsdata->m_id)));
					CComPtr<ADODB::_Recordset> recordset = command->Execute(NULL, NULL, 0);
					newsdata->m_unread = false;
					m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);
				}
			}

			RefreshTree();
			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsBack(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_htmlCtrl->GoBack();
		return 0;
	}

	LRESULT OnActionsForward(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_htmlCtrl->GoForward();
		return 0;
	}

	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		static BOOL bVisible = TRUE;	// initially visible
		bVisible = !bVisible;
		CReBarCtrl rebar = m_hWndToolBar;
		int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
		rebar.ShowBand(nBandIndex, bVisible);
		UISetCheck(ID_VIEW_TOOLBAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
		::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
		UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
		UpdateLayout();
		return 0;
	}

	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	LRESULT OnNextPane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(::GetFocus() == m_treeView.m_hWnd)
		{
			m_listView.SetFocus();
		}
		else if(::GetFocus() == m_listView.m_hWnd)
		{
			m_htmlView.SetFocus();
			CComPtr<IOleObject> pIOO;
			m_htmlView.QueryControl(&pIOO);
			ATLASSERT(pIOO != NULL);
			CRect rc;
			m_htmlView.GetClientRect(&rc);
			pIOO->DoVerb(OLEIVERB_UIACTIVATE, (LPMSG)GetCurrentMessage(), NULL, 0, this->m_hWnd, &rc);
		}
		else
		{
			m_treeView.SetFocus();
		}

		return 0;
	}

	LRESULT OnPrevPane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		if(::GetFocus() == m_treeView.m_hWnd)
		{
			m_htmlView.SetFocus();
			CComPtr<IOleObject> pIOO;
			m_htmlView.QueryControl(&pIOO);
			ATLASSERT(pIOO != NULL);
			CRect rc;
			m_htmlView.GetClientRect(&rc);
			pIOO->DoVerb(OLEIVERB_UIACTIVATE, (LPMSG)GetCurrentMessage(), NULL, 0, this->m_hWnd, &rc);
		}
		else if(::GetFocus() == m_listView.m_hWnd)
		{
			m_treeView.SetFocus();
		}
		else
		{
			m_listView.SetFocus();
		}

		return 0;
	}

	HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, DWORD x, DWORD y, IUnknown *pcmdtReserved, IDispatch *pdispReserved, HRESULT *dwRetVal)
	{
		*dwRetVal = S_OK;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetHostInfo(DWORD *pdwFlags, DWORD *pdwDoubleClick)
	{
		*pdwFlags = DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_THEME | DOCHOSTUIFLAG_NO3DBORDER;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, IUnknown *pActiveObject, IUnknown *pCommandTarget, IUnknown *pFrame, IUnknown *pDoc, HRESULT *dwRetVal)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE HideUI()
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE UpdateUI()
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE EnableModeless(VARIANT_BOOL fEnable)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE OnDocWindowActivate(VARIANT_BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(VARIANT_BOOL fActivate)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE ResizeBorder(long left, long top, long right, long bottom, IUnknown *pUIWindow, VARIANT_BOOL fFrameWindow)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE TranslateAccelerator(DWORD_PTR hWnd, DWORD nMessage, DWORD_PTR wParam, DWORD_PTR lParam, BSTR bstrGuidCmdGroup, DWORD nCmdID, HRESULT *dwRetVal)
	{
		*dwRetVal = S_OK;

		if((nMessage == WM_KEYDOWN || nMessage == WM_KEYUP) && (wParam == VK_TAB || wParam == VK_BACK))
			*dwRetVal = S_FALSE;

		if(wParam == VK_RETURN)
			*dwRetVal = S_FALSE;

		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetOptionKeyPath(BSTR *pbstrKey, DWORD dw)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetDropTarget(IUnknown *pDropTarget, IUnknown **ppDropTarget)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, BSTR bstrURLIn, BSTR *pbstrURLOut)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE FilterDataObject(IUnknown *pDO, IUnknown **ppDORet)
	{
		return E_NOTIMPL;
	}

	void STDMETHODCALLTYPE OnDownloadBegin()
	{
		if(m_downloads++ == 0)
		{
			m_progressBar.ShowWindow(SW_SHOW);
		}
	}

	void STDMETHODCALLTYPE OnDownloadComplete()
	{
		if(--m_downloads == 0)
		{
			m_progressBar.ShowWindow(SW_HIDE);
		}
	}

	void STDMETHODCALLTYPE OnProgressChange(long progress, long progressMax)
	{
		m_progressBar.SetRange(0, progressMax);
		m_progressBar.SetPos(progress);
	}

	void STDMETHODCALLTYPE OnCommandStateChange(long command, bool enable)
	{
		if(command == CSC_NAVIGATEBACK)
			if(enable)
				UISetState(ID_ACTIONS_BACK, UPDUI_ENABLED);
			else
				UISetState(ID_ACTIONS_BACK, UPDUI_DISABLED);
		else if(command == CSC_NAVIGATEFORWARD)
			if(enable)
				UISetState(ID_ACTIONS_FORWARD, UPDUI_ENABLED);
			else
				UISetState(ID_ACTIONS_FORWARD, UPDUI_DISABLED);
	}

	void STDMETHODCALLTYPE OnStatusTextChange(BSTR text)
	{
		m_statusBar.SetPaneText(0, _bstr_t(text));
	}
};
