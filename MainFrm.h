// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#define REFRESH_INTERVAL (5*60*1000)

class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CCustomizableToolBarCommands<CMainFrame>,
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
	CSearchBand m_SrcBar;
	CToolBarCtrl m_toolBar;
	CMultiPaneStatusBarCtrl m_statusBar;
	CProgressBarCtrl m_progressBar;
	CSplitterWindow m_vSplit;
	CHorSplitterWindow m_hSplit;
	CCustomTreeViewCtrl m_treeView;
	CCustomListViewCtrl m_listView;
	CAxWindow m_htmlView;
	IWebBrowser2Ptr m_htmlCtrl;
	ADODB::_ConnectionPtr m_connection;
	CImageList m_dragImage;
	int m_downloads;
	BOOL m_dragging;
	BOOL m_mustRefresh;
	BOOL m_forceUpdate;
	int m_newItems;
	HTREEITEM m_feedsRoot;
	HTREEITEM m_searchRoot;
	HTREEITEM m_itemDrag;
	HTREEITEM m_itemDrop;
	HCURSOR m_arrowCursor;
	HCURSOR m_noCursor;
	CWorkerThread<> m_updateThread;
	HANDLE m_hTimer;

	CMainFrame() : m_downloads(0), m_dragging(FALSE), m_mustRefresh(FALSE), m_forceUpdate(FALSE), m_newItems(0),
		m_feedsRoot(NULL), m_searchRoot(NULL), m_itemDrag(NULL), m_itemDrop(NULL), m_arrowCursor(LoadCursor(NULL, IDC_ARROW)),
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
			ADOX::_CatalogPtr catalog;
			catalog.CreateInstance(__uuidof(ADOX::Catalog));
			ATLASSERT(catalog != NULL);
			catalog->Create(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath);
			ADODB::_ConnectionPtr connection;
			connection.CreateInstance(__uuidof(ADODB::Connection));
			ATLASSERT(connection != NULL);
			connection->Open(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath, _bstr_t(), _bstr_t(), 0);
			ADODB::_RecordsetPtr recordset;
			recordset = connection->Execute(_bstr_t("CREATE TABLE Configuration (Name VARCHAR(255) UNIQUE NOT NULL, CurrentValue VARCHAR(255) NOT NULL)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Configuration (Name, CurrentValue) VALUES ('DBSchemaVersion', '1')"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Configuration (Name, CurrentValue) VALUES ('DefaultRefreshInterval', '60')"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Configuration (Name, CurrentValue) VALUES ('DefaultMaxAge', '30')"), NULL, 0);
			recordset = connection->Execute(_bstr_t("CREATE TABLE Folders (ID AUTOINCREMENT UNIQUE NOT NULL, Name VARCHAR(255) NOT NULL)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Folders (Name) VALUES ('Sample feeds')"), NULL, 0);
			recordset = connection->Execute(_bstr_t("CREATE TABLE Feeds (ID AUTOINCREMENT UNIQUE NOT NULL, FolderID INTEGER NOT NULL, Title VARCHAR(255) NOT NULL, URL VARCHAR(255) NOT NULL, LastError VARCHAR(255) NOT NULL DEFAULT '', Link VARCHAR(255) NOT NULL DEFAULT '', ImageLink VARCHAR(255) NOT NULL DEFAULT '', Description VARCHAR(255) NOT NULL DEFAULT '', LastUpdate DATETIME NOT NULL, RefreshInterval INTEGER NOT NULL, MaxAge INTEGER NOT NULL, NavigateURL VARCHAR(1) NOT NULL)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("CREATE INDEX FeedsI1 ON Feeds (FolderID)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Feeds (FolderID, Title, URL, LastUpdate, RefreshInterval, MaxAge, NavigateURL) VALUES (1, 'Slashdot', 'http://slashdot.org/index.rss', '2000/01/01', -1, -1, 0)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Feeds (FolderID, Title, URL, LastUpdate, RefreshInterval, MaxAge, NavigateURL) VALUES (1, 'OSNews', 'http://www.osnews.com/files/recent.rdf', '2000/01/01', -1, -1, 0)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("INSERT INTO Feeds (FolderID, Title, URL, LastUpdate, RefreshInterval, MaxAge, NavigateURL) VALUES (1, 'The Register', 'http://www.theregister.com/headlines.rss', '2000/01/01', -1, -1, 0)"), NULL, 0);
			recordset = connection->Execute(_bstr_t("CREATE TABLE News (ID AUTOINCREMENT UNIQUE NOT NULL, FeedID INTEGER NOT NULL, Title VARCHAR(255) NOT NULL, URL VARCHAR(255) NOT NULL, Issued DATETIME NOT NULL, Description MEMO, Unread VARCHAR(1) NOT NULL, Flagged VARCHAR(1) NOT NULL, CONSTRAINT NewsC1 UNIQUE (FeedID, URL))"), NULL, 0);
			recordset = connection->Execute(_bstr_t("CREATE INDEX NewsI1 ON News (FeedID)"), NULL, 0);
		}
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(::IsWindow(m_htmlView.m_hWnd) && (pMsg->hwnd == m_htmlView.m_hWnd || m_htmlView.IsChild(pMsg->hwnd)))
		{
			CComPtr<IOleInPlaceActiveObject> pIOAO;
			m_htmlView.QueryControl(&pIOAO);
			ATLASSERT(pIOAO != NULL);

			if(pIOAO->TranslateAccelerator(pMsg) == S_OK)
				return TRUE;
		}

		if(::IsWindow(m_SrcBar.m_hWnd) && m_SrcBar.IsDialogMessage(pMsg))
			return TRUE;

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
				UpdateFeedProperties(feeddata);
				feeddata->m_unread = GetUnreadItemCount(feeddata->m_id);

				if(feeddata->m_error.GetLength() == 0)
					m_treeView.SetItemImage(i, 0, 0);
				else
					m_treeView.SetItemImage(i, 2, 2);

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
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued";
				}

				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

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
							newsdata->m_title = recordset->Fields->GetItem("News.Title")->Value;
							newsdata->m_description = recordset->Fields->GetItem("News.Description")->Value;
							newsdata->m_issued = recordset->Fields->GetItem("Issued")->Value;
							newsdata->m_feedTreeItem = GetFeedTreeItem(recordset->Fields->GetItem("Feeds.ID")->Value);
							feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));

							if((_bstr_t)recordset->Fields->GetItem("Unread")->Value == _bstr_t("0"))
								newsdata->m_unread = false;
							else
								newsdata->m_unread = true;

							if((_bstr_t)recordset->Fields->GetItem("Flagged")->Value == _bstr_t("0"))
								newsdata->m_flagged = false;
							else
								newsdata->m_flagged = true;

							if(newsdata->m_flagged)
								m_listView.InsertItem(0, newsdata->m_issued, 2);
							else if(newsdata->m_unread)
								m_listView.InsertItem(0, newsdata->m_issued, 1);
							else
								m_listView.InsertItem(0, newsdata->m_issued, 0);

							m_listView.AddItem(0, 1, CAtlString(recordset->Fields->GetItem("News.Title")->Value));
							m_listView.AddItem(0, 2, feeddata->m_title);
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
			else
			{
				RefreshTree();
			}

			m_mustRefresh = FALSE;
			m_forceUpdate = FALSE;
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
			UISetState(ID_ACTIONS_MARKALLREAD, UPDUI_ENABLED);
		else
			UISetState(ID_ACTIONS_MARKALLREAD, UPDUI_DISABLED);

		int j = m_listView.GetSelectedIndex();

		if(j >= 0)
		{
			UISetState(ID_ACTIONS_SENDMAIL, UPDUI_ENABLED);
			NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(j));

			if(newsdata->m_unread)
			{
				UISetState(ID_ACTIONS_MARKREAD, UPDUI_ENABLED);
				UISetState(ID_ACTIONS_MARKUNREAD, UPDUI_DISABLED);
			}
			else
			{
				UISetState(ID_ACTIONS_MARKREAD, UPDUI_DISABLED);
				UISetState(ID_ACTIONS_MARKUNREAD, UPDUI_ENABLED);
			}

			if(newsdata->m_flagged)
			{
				UISetState(ID_ACTIONS_MARKFLAGGED, UPDUI_DISABLED);
				UISetState(ID_ACTIONS_MARKUNFLAGGED, UPDUI_ENABLED);
			}
			else
			{
				UISetState(ID_ACTIONS_MARKFLAGGED, UPDUI_ENABLED);
				UISetState(ID_ACTIONS_MARKUNFLAGGED, UPDUI_DISABLED);
			}
		}
		else
		{
			UISetState(ID_ACTIONS_SENDMAIL, UPDUI_DISABLED);
			UISetState(ID_ACTIONS_MARKREAD, UPDUI_DISABLED);
			UISetState(ID_ACTIONS_MARKUNREAD, UPDUI_DISABLED);
			UISetState(ID_ACTIONS_MARKFLAGGED, UPDUI_DISABLED);
			UISetState(ID_ACTIONS_MARKUNFLAGGED, UPDUI_DISABLED);
		}

		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_FILE_DELETE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_PROPERTIES, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_ACTIONS_SENDMAIL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_ACTIONS_MARKREAD, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTIONS_MARKUNREAD, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTIONS_MARKFLAGGED, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTIONS_MARKUNFLAGGED, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_ACTIONS_MARKALLREAD, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
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
		COMMAND_ID_HANDLER(ID_VIEW_OPTIONS, OnViewOptions)
		COMMAND_ID_HANDLER(ID_VIEW_CUSTOMIZETOOLBAR, OnViewCustomizeToolbar)
		COMMAND_ID_HANDLER(ID_ACTIONS_SENDMAIL, OnActionsSendMail)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKREAD, OnActionsMarkRead)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKUNREAD, OnActionsMarkUnread)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKFLAGGED, OnActionsMarkFlagged)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKUNFLAGGED, OnActionsMarkUnflagged)
		COMMAND_ID_HANDLER(ID_ACTIONS_MARKALLREAD, OnActionsMarkAllRead)
		COMMAND_ID_HANDLER(ID_ACTIONS_UPDATEFEEDS, OnActionsUpdateFeeds)
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
		CHAIN_MSG_MAP(CCustomizableToolBarCommands<CMainFrame>)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
		COMMAND_ID_HANDLER(IDC_SEARCH, OnSearch)
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
		ADODB::_RecordsetPtr recordset;
		recordset.CreateInstance(__uuidof(ADODB::Recordset));
		ATLASSERT(recordset != NULL);
		recordset->CursorLocation = ADODB::adUseServer;
		SYSTEMTIME t;
		::GetSystemTime(&t);
		COleDateTime now(t);
		recordset->Open(_bstr_t("Feeds"), _variant_t(m_connection.GetInterfacePtr()), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();

			while(!recordset->EndOfFile)
			{
				COleDateTime t1;
				t1.ParseDateTime((_bstr_t)recordset->Fields->GetItem("LastUpdate")->Value);
				int refresh = recordset->Fields->GetItem("RefreshInterval")->Value;

				if(refresh == -1)
					refresh = GetConfiguration("DefaultRefreshInterval", 60);

				if(refresh > 0)
				{
					COleDateTimeSpan s1(0, 0, refresh, 0);

					if(m_forceUpdate || t1+s1 < now)
					{
						int feedid = recordset->Fields->GetItem("ID")->Value;
						_bstr_t url = recordset->Fields->GetItem("URL")->Value;
						CFeedParser fp(url);

						for(size_t i = 0; i < fp.m_items.GetCount(); ++i)
							AddNewsToFeed(feedid, fp.m_items[i]);

						recordset->Fields->GetItem("LastError")->Value = (_bstr_t)fp.m_error;
						recordset->Fields->GetItem("Link")->Value = (_bstr_t)fp.m_link;
						recordset->Fields->GetItem("ImageLink")->Value = (_bstr_t)fp.m_image;
						recordset->Fields->GetItem("Description")->Value = (_bstr_t)fp.m_description;
						recordset->Fields->GetItem("LastUpdate")->Value = (_bstr_t)now.Format("%Y/%m/%d %H:%M:%S");
						recordset->Update();
					}
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

	void AddNewsToFeed(int feedid, const CFeedParser::FeedItem& item)
	{
		try
		{
			COleDateTime dt;
			dt.SetStatus(COleDateTime::invalid);
			CAtlString t1((const char*)item.m_date);

			if(t1.Mid(4, 1) == "-" && t1.Mid(7, 1) == "-")
			{
				dt.SetDateTime(atoi(t1.Mid(0, 4)), atoi(t1.Mid(5, 2)), atoi(t1.Mid(8, 2)), atoi(t1.Mid(11, 2)), atoi(t1.Mid(14, 2)), atoi(t1.Mid(17, 2)));

				if(t1.Mid(19, 1) == "+")
					dt -= COleDateTimeSpan(0, atoi(t1.Mid(20, 2)), atoi(t1.Mid(23, 2)), 0);

				if(t1.Mid(19, 1) == "-")
					dt += COleDateTimeSpan(0, atoi(t1.Mid(20, 2)), atoi(t1.Mid(23, 2)), 0);
			}
			else if(t1.Mid(3, 1) == "," && t1.Mid(4, 1) == " ")
			{
				int pos = 0;
				CAtlString tok = t1.Tokenize(" ,:", pos);
				int dbuf[6] = { 0 };
				int dpos = 0;

				while(tok != "" && dpos < 6)
				{
					int v = atoi(tok);

					if(v == 0)
					{
						if(tok == "Jan")
							v = 1;
						else if(tok == "Feb")
							v = 2;
						else if(tok == "Mar")
							v = 3;
						else if(tok == "Apr")
							v = 4;
						else if(tok == "May")
							v = 5;
						else if(tok == "Jun")
							v = 6;
						else if(tok == "Jul")
							v = 7;
						else if(tok == "Aug")
							v = 8;
						else if(tok == "Sep")
							v = 9;
						else if(tok == "Oct")
							v = 10;
						else if(tok == "Nov")
							v = 11;
						else if(tok == "Dec")
							v = 12;
					}

					if(v > 0 || (dpos >= 3 && v >= 0))
						dbuf[dpos++] = v;

					tok = t1.Tokenize(" ,:", pos);
				}

				dt.SetDateTime(dbuf[2], dbuf[1], dbuf[0], dbuf[3], dbuf[4], dbuf[5]);

				if(tok.Left(4) == "GMT+")
				{
					dt -= COleDateTimeSpan(0, atoi(tok.Mid(4)), 0, 0);
				}
				else if(tok.Left(4) == "GMT-")
				{
					dt += COleDateTimeSpan(0, atoi(tok.Mid(4)), 0, 0);
				}
			}

			if(dt.GetStatus() == COleDateTime::invalid)
				dt = COleDateTime::GetCurrentTime();

			CAtlString t2 = dt.Format("%Y/%m/%d %H:%M:%S");

			ADODB::_RecordsetPtr recordset;
			recordset.CreateInstance(__uuidof(ADODB::Recordset));
			ATLASSERT(recordset != NULL);
			recordset->CursorLocation = ADODB::adUseServer;
			recordset->Open(_bstr_t("News"), _variant_t(m_connection.GetInterfacePtr()), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
			recordset->AddNew();
			recordset->Fields->GetItem("FeedID")->Value = feedid;
			recordset->Fields->GetItem("Title")->Value = (_bstr_t)item.m_title;
			recordset->Fields->GetItem("URL")->Value = (_bstr_t)item.m_url;
			recordset->Fields->GetItem("Description")->Value = (_bstr_t)item.m_description;
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

	void UpdateFeedProperties(FeedData* feeddata)
	{
		ADODB::_CommandPtr command;
		command.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT * FROM Feeds WHERE ID=?";
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feeddata->m_id)));
		ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();
			feeddata->m_error = recordset->Fields->GetItem("LastError")->Value;
			feeddata->m_link = recordset->Fields->GetItem("Link")->Value;
			feeddata->m_image = recordset->Fields->GetItem("ImageLink")->Value;
			feeddata->m_description = recordset->Fields->GetItem("Description")->Value;
		}
	}

	int GetConfiguration(const char* name, int def)
	{
		ADODB::_CommandPtr command;
		command.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT * FROM Configuration WHERE Name=?";
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, _variant_t(name)));
		ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

		if(!recordset->EndOfFile)
		{
			recordset->MoveFirst();
			return atoi((_bstr_t)recordset->Fields->GetItem("CurrentValue")->Value);
		}

		return def;
	}

	void SetConfiguration(const char* name, int val)
	{
		ADODB::_CommandPtr command;
		command.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "UPDATE Configuration SET CurrentValue=? WHERE Name=?";
		CAtlString valstr;
		valstr.Format("%d", val);
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, _variant_t(valstr)));
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, _variant_t(name)));
		ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
	}

	int GetUnreadItemCount(int feedid)
	{
		ADODB::_CommandPtr command;
		command.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT COUNT(*) AS ItemCount FROM News WHERE FeedID=? AND Unread='1'";
		command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feedid)));
		ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

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

		HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE | CCS_ADJUSTABLE);
		InitToolBar(hWndToolBar, IDR_MAINFRAME);
		m_toolBar = hWndToolBar;
		m_toolBar.RestoreState(HKEY_CURRENT_USER, "Software\\FeedIt", "ToolBar");

		HWND hWndSrcBar = m_SrcBar.Create(m_hWnd);

		CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
		AddSimpleReBarBand(hWndCmdBar);
		AddSimpleReBarBand(hWndToolBar, NULL, TRUE, 0, TRUE);
		AddSimpleReBarBand(hWndSrcBar, "Search", FALSE, 0, TRUE);

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
		tvil.Create(IDB_TREE_IMAGELIST, 16, 3, RGB(192, 192, 192));
		m_treeView.SetImageList(tvil.Detach(), TVSIL_NORMAL);
		m_vSplit.SetSplitterPane(0, m_treeView);

		m_listView.Create(m_hSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE);
		::SendMessage(m_listView.m_hWnd, CCM_SETVERSION, 5, 0);
		m_listView.SetDlgCtrlID(IDC_LIST);
		CImageList lvil;
		lvil.Create(IDB_LIST_IMAGELIST, 16, 3, RGB(192, 192, 192));
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
		m_searchRoot = m_treeView.InsertItem(_T("Search results"), TVI_ROOT, TVI_LAST);

		m_connection.CreateInstance(__uuidof(ADODB::Connection));
		ATLASSERT(m_connection != NULL);
		m_connection->Open(_bstr_t("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=")+m_dbPath, _bstr_t(), _bstr_t(), 0);
		ADODB::_CommandPtr command;
		command.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(command != NULL);
		command->ActiveConnection = m_connection;
		command->CommandText = "SELECT * FROM Folders";
		ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

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
				ADODB::_CommandPtr subcommand;
				subcommand.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(subcommand != NULL);
				subcommand->ActiveConnection = m_connection;
				subcommand->CommandText = "SELECT * FROM Feeds WHERE FolderID=?";
				subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(folderitemdata->m_id)));
				ADODB::_RecordsetPtr subrecordset = subcommand->Execute(NULL, NULL, 0);

				if(!subrecordset->EndOfFile)
				{
					subrecordset->MoveFirst();

					while(!subrecordset->EndOfFile)
					{
						FeedData* feeditemdata = new FeedData();
						feeditemdata->m_id = subrecordset->Fields->GetItem("ID")->Value;
						feeditemdata->m_error = subrecordset->Fields->GetItem("LastError")->Value;
						feeditemdata->m_title = subrecordset->Fields->GetItem("Title")->Value;
						feeditemdata->m_link = subrecordset->Fields->GetItem("Link")->Value;
						feeditemdata->m_image = subrecordset->Fields->GetItem("ImageLink")->Value;
						feeditemdata->m_description = subrecordset->Fields->GetItem("Description")->Value;
						feeditemdata->m_unread = GetUnreadItemCount(feeditemdata->m_id);
						feeditemdata->m_navigateURL = atoi(_bstr_t(subrecordset->Fields->GetItem("NavigateURL")->Value));
						HTREEITEM feeditem = m_treeView.InsertItem(feeditemdata->m_title, folderitem, TVI_LAST);

						if(feeditemdata->m_error.GetLength() == 0)
							m_treeView.SetItemImage(feeditem, 0, 0);
						else
							m_treeView.SetItemImage(feeditem, 2, 2);

						m_treeView.SetItemData(feeditem, (DWORD_PTR)feeditemdata);
						subrecordset->MoveNext();
					}
				}

				m_treeView.Expand(folderitem);
				recordset->MoveNext();
			}
		}

		ADODB::_CommandPtr subcommand;
		subcommand.CreateInstance(__uuidof(ADODB::Command));
		ATLASSERT(subcommand != NULL);
		subcommand->ActiveConnection = m_connection;
		subcommand->CommandText = "SELECT * FROM Feeds WHERE FolderID=0";
		ADODB::_RecordsetPtr subrecordset = subcommand->Execute(NULL, NULL, 0);

		if(!subrecordset->EndOfFile)
		{
			subrecordset->MoveFirst();

			while(!subrecordset->EndOfFile)
			{
				int id = subrecordset->Fields->GetItem("ID")->Value;
				FeedData* feeditemdata = new FeedData();
				feeditemdata->m_id = subrecordset->Fields->GetItem("ID")->Value;
				feeditemdata->m_error = subrecordset->Fields->GetItem("LastError")->Value;
				feeditemdata->m_title = subrecordset->Fields->GetItem("Title")->Value;
				feeditemdata->m_link = subrecordset->Fields->GetItem("Link")->Value;
				feeditemdata->m_image = subrecordset->Fields->GetItem("ImageLink")->Value;
				feeditemdata->m_description = subrecordset->Fields->GetItem("Description")->Value;
				feeditemdata->m_unread = GetUnreadItemCount(feeditemdata->m_id);
				feeditemdata->m_navigateURL = atoi(_bstr_t(subrecordset->Fields->GetItem("NavigateURL")->Value));
				HTREEITEM feeditem = m_treeView.InsertItem(feeditemdata->m_title, m_feedsRoot, TVI_LAST);

				if(feeditemdata->m_error.GetLength() == 0)
					m_treeView.SetItemImage(feeditem, 0, 0);
				else
					m_treeView.SetItemImage(feeditem, 2, 2);

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
		DispEventUnadvise(m_htmlCtrl);

		CWindowSettings ws;
		ws.GetFrom(*this);
		ws.Save("Software\\FeedIt", "MainFrame");

		CReBarSettings rs;
		CReBarCtrl rbc = m_hWndToolBar;
		rs.GetFrom(rbc);
		rs.Save("Software\\FeedIt", "ReBar");

		m_toolBar.SaveState(HKEY_CURRENT_USER, "Software\\FeedIt", "ToolBar");

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
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Feeds SET FolderID=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(m_itemDrop))->m_id)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(m_itemDrag))->m_id)));
				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
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
		CRect trc;
		CRect lrc;
		m_treeView.GetWindowRect(&trc);
		m_listView.GetWindowRect(&lrc);

		if((pt == npt && (m_treeView.m_hWnd == ::GetFocus() || ::IsChild(m_treeView.m_hWnd, ::GetFocus()))) || trc.PtInRect(pt))
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
					pt = trc.TopLeft();
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
		else if((pt == npt && (m_listView.m_hWnd == ::GetFocus() || ::IsChild(m_listView.m_hWnd, ::GetFocus()))) || lrc.PtInRect(pt))
		{
			if(pt == npt)
			{
				CRect r;
				int i = m_listView.GetSelectedIndex();

				if(i >= 0)
				{
					m_listView.GetItemRect(i, &r, TRUE);
					pt = r.TopLeft();
				}
				else
				{
					pt = lrc.TopLeft();
				}

				m_listView.ClientToScreen(&pt);
			}

			CMenu menu;
			if(!menu.LoadMenu(IDR_LIST_POPUP))
				return 0;
			CMenuHandle popup(menu.GetSubMenu(0));
			PrepareMenu(popup);
			SetForegroundWindow(m_hWnd);
			popup.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
			menu.DestroyMenu();
		}

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

		if(feeddata != NULL || folderdata != NULL || i == m_feedsRoot || i == m_searchRoot)
		{
			CAtlString search;

			if(i == m_searchRoot)
				m_SrcBar.m_search.GetWindowText(search);

			{
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued";
				}

				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

				if(!recordset->EndOfFile)
				{
					recordset->MoveFirst();

					while(!recordset->EndOfFile)
					{
						if(i != m_searchRoot || CheckSearchResult(search, CAtlString((char*)(_bstr_t)recordset->Fields->GetItem("News.Title")->Value), CAtlString((char*)(_bstr_t)recordset->Fields->GetItem("News.Description")->Value)))
						{
							NewsData* newsdata = new NewsData();
							newsdata->m_id = recordset->Fields->GetItem("News.ID")->Value;
							newsdata->m_url = recordset->Fields->GetItem("News.URL")->Value;
							newsdata->m_title = recordset->Fields->GetItem("News.Title")->Value;
							newsdata->m_description = recordset->Fields->GetItem("News.Description")->Value;
							newsdata->m_issued = recordset->Fields->GetItem("Issued")->Value;
							newsdata->m_feedTreeItem = GetFeedTreeItem(recordset->Fields->GetItem("Feeds.ID")->Value);
							FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));

							if((_bstr_t)recordset->Fields->GetItem("Unread")->Value == _bstr_t("0"))
								newsdata->m_unread = false;
							else
								newsdata->m_unread = true;

							if((_bstr_t)recordset->Fields->GetItem("Flagged")->Value == _bstr_t("0"))
								newsdata->m_flagged = false;
							else
								newsdata->m_flagged = true;

							if(newsdata->m_flagged)
								m_listView.InsertItem(0, newsdata->m_issued, 2);
							else if(newsdata->m_unread)
								m_listView.InsertItem(0, newsdata->m_issued, 1);
							else
								m_listView.InsertItem(0, newsdata->m_issued, 0);

							m_listView.AddItem(0, 1, CAtlString(recordset->Fields->GetItem("News.Title")->Value));
							m_listView.AddItem(0, 2, feeddata->m_title);
							m_listView.SetItemData(0, (DWORD_PTR)newsdata);
						}

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
				WriteLine(hFile, "<title>FeedIt</title>");
				WriteLine(hFile, "<style type=\"text/css\">");
				WriteLine(hFile, "\tbody { background-color: white; color: black; font: 84% Verdana, Arial, sans-serif; margin: 12px 22px; }");
				WriteLine(hFile, "\ttable { font: 100% Verdana, Arial, sans-serif; }");
				WriteLine(hFile, "\ta { color: #0002CA; }");
				WriteLine(hFile, "\ta:hover { color: #6B8ADE; text-decoration: underline; }");
				WriteLine(hFile, "");
				WriteLine(hFile, "\th1,h2,h3,h4,h5,h6 { font-size: 80%; font-weight: bold; font-style: italic;	}");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.newspapertitle { font-weight: bold; font-size: 120%; text-align: center; padding-bottom: 12px; margin-bottom: 12px; border-bottom: 1px dashed silver; }");
				WriteLine(hFile, "");
				WriteLine(hFile, "\tdiv.newsitemcontent { line-height: 140%; text-align: justify; }");
				WriteLine(hFile, "\tdiv.newsitemcontent ol, div.newsitemcontent ul { list-style-position: inside; }");
				WriteLine(hFile, "\tdiv.newsitemtitle { font-weight: bold; margin-bottom: 8px }");
				WriteLine(hFile, "\tdiv.newsitemtitle a, div.newsitemcontent a, div.newsitemfooter a { text-decoration: none; }");
				WriteLine(hFile, "\tdiv.newsitemfooter { color: gray; font-size: xx-small; text-align: left; margin-top: 6px; margin-bottom: 18px; }");
				WriteLine(hFile, "</style>");
				WriteLine(hFile, "</head>");
				WriteLine(hFile, "<body>");

				if(feeddata != NULL)
				{
					CAtlString tmp;

					if(feeddata->m_image.GetLength() > 0)
						tmp.Format("\t<div class=\"newspapertitle\"><img src=\"%s\"><br><a href=\"%s\">%s</a> - %s</div>", feeddata->m_image, feeddata->m_link, feeddata->m_title, feeddata->m_description);
					else
						tmp.Format("\t<div class=\"newspapertitle\"><a href=\"%s\">%s</a> - %s</div>", feeddata->m_link, feeddata->m_title, feeddata->m_description);

					WriteLine(hFile, tmp);
				}
				else if(folderdata != NULL)
				{
					CAtlString tmp;
					tmp.Format("\t<div class=\"newspapertitle\">%s</div>", folderdata->m_name);
					WriteLine(hFile, tmp);
				}
				else if(i == m_searchRoot)
				{
					WriteLine(hFile, "\t<div class=\"newspapertitle\">Search results</div>");
				}
				else
				{
					WriteLine(hFile, "\t<div class=\"newspapertitle\">Headlines</div>");
				}

				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;

				if(feeddata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE FeedID=? ORDER BY Issued DESC";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feeddata->m_id)));
				}
				else if(folderdata != NULL)
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID WHERE Feeds.FolderID=? ORDER BY Issued DESC";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(folderdata->m_id)));
				}
				else
				{
					command->CommandText = "SELECT Feeds.*, News.* FROM Feeds INNER JOIN News ON Feeds.ID = News.FeedID ORDER BY Issued DESC";
				}

				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

				if(!recordset->EndOfFile)
				{
					recordset->MoveFirst();
					int x = 0;

					while(!recordset->EndOfFile)
					{
						if(i != m_searchRoot || CheckSearchResult(search, CAtlString((char*)(_bstr_t)recordset->Fields->GetItem("News.Title")->Value), CAtlString((char*)(_bstr_t)recordset->Fields->GetItem("News.Description")->Value)))
						{
							if(x % 2 == 0)
								WriteLine(hFile, "<table width=\"100%\"><tr><td width=\"90%\">");
							else
								WriteLine(hFile, "<table width=\"100%\"><tr><td width=\"10%\">&nbsp;</td><td width=\"75%\">");

							CAtlString tmp;
							tmp.Format("\t<div class=\"newsitemtitle\"><a href=\"%s\">%s</a></div>", (const char*)(_bstr_t)recordset->Fields->GetItem("News.URL")->Value, (const char*)(_bstr_t)recordset->Fields->GetItem("News.Title")->Value);
							WriteLine(hFile, tmp);
							tmp.Format("\t<div class=\"newsitemcontent\">%s</div>", (const char*)(_bstr_t)recordset->Fields->GetItem("News.Description")->Value);
							WriteLine(hFile, tmp);
							tmp.Format("\t<div class=\"newsitemfooter\">%s - Received %s</div>", (const char*)(_bstr_t)recordset->Fields->GetItem("Feeds.Title")->Value, (const char*)(_bstr_t)recordset->Fields->GetItem("Issued")->Value);
							WriteLine(hFile, tmp);

							if(x % 2 == 0)
								WriteLine(hFile, "</td><td width=\"10%\">&nbsp;</td></tr></table>");
							else
								WriteLine(hFile, "</td></tr></table>");

							++x;
						}

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

	bool CheckSearchResult(CAtlString& search, CAtlString& title, CAtlString& description)
	{
		CAtlString s = search.MakeLower();
		CAtlString t = title.MakeLower()+description.MakeLower();

		if(s.GetLength() > 0 && t.Find(s) >= 0)
			return true;
		else
			return false;
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
				feeddata->m_title = pTVDI->item.pszText;
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Feeds SET Name=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, _variant_t(feeddata->m_title)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(feeddata->m_id)));
				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
				return TRUE;
			}
			else if(isFolder)
			{
				FolderData* folderdata = dynamic_cast<FolderData*>(treedata);
				folderdata->m_name = pTVDI->item.pszText;
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE Folders SET Name=? WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, _variant_t(folderdata->m_name)));
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(folderdata->m_id)));
				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
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

	LRESULT OnListSelectionChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;

		if((pnmv->uChanged & LVIF_STATE) == 0 || (pnmv->uNewState & LVIS_SELECTED) == 0)
			return 0;

		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL)
		{
			FeedData* feeddata = dynamic_cast<FeedData*>((FeedData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));
			_variant_t url;

			if(feeddata->m_navigateURL == 2 || (feeddata->m_navigateURL == 0 && strlen(newsdata->m_description) < 60))
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
					WriteLine(hFile, "<title>FeedIt</title>");
					WriteLine(hFile, "<style type=\"text/css\">");
					WriteLine(hFile, "\tbody { background-color: white; color: black; font: 84% Verdana, Arial, sans-serif; margin: 12px 22px; }");
					WriteLine(hFile, "\ttable { font: 100% Verdana, Arial, sans-serif; }");
					WriteLine(hFile, "\ta { color: #0002CA; }");
					WriteLine(hFile, "\ta:hover { color: #6B8ADE; text-decoration: underline; }");
					WriteLine(hFile, "");
					WriteLine(hFile, "\th1,h2,h3,h4,h5,h6 { font-size: 80%; font-weight: bold; font-style: italic;	}");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.newspapertitle { font-weight: bold; font-size: 120%; text-align: center; padding-bottom: 12px; margin-bottom: 12px; border-bottom: 1px dashed silver; }");
					WriteLine(hFile, "");
					WriteLine(hFile, "\tdiv.newsitemcontent { line-height: 140%; text-align: justify; }");
					WriteLine(hFile, "\tdiv.newsitemcontent ol, div.newsitemcontent ul { list-style-position: inside; }");
					WriteLine(hFile, "\tdiv.newsitemtitle { font-weight: bold; border-bottom: 1px dotted silver; margin-bottom: 10px; padding-bottom: 10px; }");
					WriteLine(hFile, "\tdiv.newsitemtitle a, div.newsitemcontent a, div.newsitemfooter a { text-decoration: none; }");
					WriteLine(hFile, "\tdiv.newsitemfooter { color: gray; font-size: xx-small; text-align: right; margin-top: 14px; padding-top: 6px; border-top: 1px dashed #CBCBCB; }");
					WriteLine(hFile, "</style>");
					WriteLine(hFile, "</head>");
					WriteLine(hFile, "<body>");
					tmp.Format("\t<div class=\"newsitemtitle\"><a href=\"%s\">%s</a></div>", (const char*)newsdata->m_url, (const char*)newsdata->m_title);
					WriteLine(hFile, tmp);
					tmp.Format("\t<div class=\"newsitemcontent\">%s</div>", (const char*)newsdata->m_description);
					WriteLine(hFile, tmp);
					tmp.Format("\t<div class=\"newsitemfooter\">%s - Received %s</div>", (const char*)feeddata->m_title, (const char*)newsdata->m_issued);
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
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "UPDATE News SET Unread='0' WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
				newsdata->m_unread = false;

				if(newsdata->m_flagged)
					m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
				else if(newsdata->m_unread)
					m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
				else
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

	LRESULT OnSearch(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_treeView.SelectItem(NULL);
		m_treeView.SelectItem(m_searchRoot);
		return 0;
	}

	LRESULT OnShow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		WINDOWPLACEMENT plc;
		plc.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(&plc);

		if(plc.showCmd == SW_SHOWMINIMIZED || plc.showCmd == SW_MINIMIZE)
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
			CFeedParser fp(dlg.m_value);

			if(fp.m_type != CFeedParser::FPFT_UNKNOWN)
			{
				ADODB::_RecordsetPtr recordset;
				recordset.CreateInstance(__uuidof(ADODB::Recordset));
				ATLASSERT(recordset != NULL);
				recordset->CursorLocation = ADODB::adUseServer;
				recordset->Open(_bstr_t("Feeds"), _variant_t(m_connection.GetInterfacePtr()), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
				recordset->AddNew();
				recordset->Fields->GetItem("FolderID")->Value = 0;
				recordset->Fields->GetItem("Title")->Value = _bstr_t(fp.m_title);
				recordset->Fields->GetItem("URL")->Value = _bstr_t(dlg.m_value);
				recordset->Fields->GetItem("LastUpdate")->Value = _bstr_t("2000/01/01 00:00:00");
				recordset->Fields->GetItem("RefreshInterval")->Value = 60;
				recordset->Fields->GetItem("MaxAge")->Value = 0;
				recordset->Fields->GetItem("NavigateURL")->Value = _bstr_t("0");
				recordset->Update();
				FeedData* itemdata = new FeedData();
				itemdata->m_id = recordset->Fields->GetItem("ID")->Value;
				itemdata->m_title = fp.m_title;
				itemdata->m_unread = 0;
				HTREEITEM item = m_treeView.InsertItem(fp.m_title, m_feedsRoot, TVI_LAST);
				m_treeView.SetItemImage(item, 0, 0);
				m_treeView.SetItemData(item, (DWORD_PTR)itemdata);
				m_treeView.SortChildren(m_feedsRoot, TRUE);
				m_treeView.Expand(m_feedsRoot);
				LARGE_INTEGER liDueTime;
				liDueTime.QuadPart = -10000 * 10;
				::SetWaitableTimer(m_hTimer, &liDueTime, 0,  NULL, NULL, FALSE);
			}
			else
			{
				CAtlString tmp;
				tmp.Format("The URL you entered doesn't point to a good feed.\nThe error is: %s", fp.m_error);
				AtlMessageBox(m_hWnd, tmp.GetBuffer(), "Error", MB_OK | MB_ICONERROR);
			}
		}

		return 0;
	}

	LRESULT OnFileNewFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStringDlg dlg(_T("Enter folder name:"));
		dlg.m_value = _T("New");

		if(dlg.DoModal() == IDOK)
		{
			ADODB::_RecordsetPtr recordset;
			recordset.CreateInstance(__uuidof(ADODB::Recordset));
			ATLASSERT(recordset != NULL);
			recordset->CursorLocation = ADODB::adUseServer;
			recordset->Open(_bstr_t("Folders"), _variant_t(m_connection.GetInterfacePtr()), ADODB::adOpenStatic, ADODB::adLockOptimistic, 0);
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
					ADODB::_CommandPtr command;
					command.CreateInstance(__uuidof(ADODB::Command));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "DELETE FROM News WHERE FeedID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
					ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
				}

				{
					ADODB::_CommandPtr command;
					command.CreateInstance(__uuidof(ADODB::Command));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "DELETE FROM Feeds WHERE ID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(dynamic_cast<FeedData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
					ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
				}
			}
			else if(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i)) != NULL)
			{
				ADODB::_CommandPtr command;
				command.CreateInstance(__uuidof(ADODB::Command));
				ATLASSERT(command != NULL);
				command->ActiveConnection = m_connection;
				command->CommandText = "DELETE FROM Folders WHERE ID=?";
				command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(dynamic_cast<FolderData*>((TreeData*)m_treeView.GetItemData(i))->m_id)));
				ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
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
			ADODB::_CommandPtr command;
			command.CreateInstance(__uuidof(ADODB::Command));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "SELECT * FROM Feeds WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, feeddata->m_id));
			ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);

			if(!recordset->EndOfFile)
			{
				recordset->MoveFirst();
				CFeedPropertySheet sheet("Feed properties", 0);
				sheet.m_propertiesPage.m_title = recordset->Fields->GetItem("Title")->Value;
				sheet.m_propertiesPage.m_url = recordset->Fields->GetItem("URL")->Value;
				sheet.m_propertiesPage.m_update = recordset->Fields->GetItem("RefreshInterval")->Value;
				sheet.m_propertiesPage.m_retain = recordset->Fields->GetItem("MaxAge")->Value;
				sheet.m_propertiesPage.m_browse = recordset->Fields->GetItem("NavigateURL")->Value;

				if(sheet.DoModal() == IDOK)
				{
					ADODB::_CommandPtr subcommand;
					subcommand.CreateInstance(__uuidof(ADODB::Command));
					ATLASSERT(subcommand != NULL);
					subcommand->ActiveConnection = m_connection;
					subcommand->CommandText = "UPDATE Feeds SET Title=?, URL=?, RefreshInterval=?, MaxAge=?, NavigateURL=? WHERE ID=?";
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, (_bstr_t)sheet.m_propertiesPage.m_title));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adBSTR, ADODB::adParamInput, NULL, (_bstr_t)sheet.m_propertiesPage.m_url));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, sheet.m_propertiesPage.m_update));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, sheet.m_propertiesPage.m_retain));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, sheet.m_propertiesPage.m_browse));
					subcommand->GetParameters()->Append(subcommand->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, feeddata->m_id));
					ADODB::_RecordsetPtr subrecordset = subcommand->Execute(NULL, NULL, 0);
					feeddata->m_title = sheet.m_propertiesPage.m_title;
					feeddata->m_navigateURL = sheet.m_propertiesPage.m_browse;
					m_treeView.SetItemText(i, feeddata->m_title);
					m_treeView.SortChildren(m_feedsRoot, TRUE);
				}
			}
		}

		return 0;
	}

	LRESULT OnViewOptions(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CRegKey reg;
		DWORD err = reg.Create(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run");

		if(err == ERROR_SUCCESS)
		{
			COptionsPropertySheet sheet("Options", 0);
			TCHAR buf[1024];
			ULONG len = 1024;
			sheet.m_optionsPage.m_autostart = (reg.QueryStringValue("FeedIt", buf, &len) == ERROR_SUCCESS);
			sheet.m_optionsPage.m_update = GetConfiguration("DefaultRefreshInterval", 60);
			sheet.m_optionsPage.m_retain = GetConfiguration("DefaultMaxAge", 30);

			if(sheet.DoModal() == IDOK)
			{
				::GetModuleFileName(NULL, buf, 1024);
				CAtlString tmp;
				tmp.Format("\"%s\" /background", buf);

				if(sheet.m_optionsPage.m_autostart)
					reg.SetStringValue("FeedIt", tmp);
				else
					reg.DeleteValue("FeedIt");

				SetConfiguration("DefaultRefreshInterval", sheet.m_optionsPage.m_update);
				SetConfiguration("DefaultMaxAge", sheet.m_optionsPage.m_retain);
			}
		}

		return 0;
	}

	LRESULT OnViewCustomizeToolbar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_toolBar.Customize();
		return 0;
	}

	LRESULT OnActionsMarkRead(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL && newsdata->m_unread)
		{
			ADODB::_CommandPtr command;
			command.CreateInstance(__uuidof(ADODB::Command));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "UPDATE News SET Unread='0' WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
			ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
			newsdata->m_unread = false;

			if(newsdata->m_flagged)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
			else if(newsdata->m_unread)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
			else
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);

			FeedData* feeddata = dynamic_cast<FeedData*>((FeedData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));
			feeddata->m_unread--;
			CAtlString txt;
			m_treeView.GetItemText(newsdata->m_feedTreeItem, txt);
			m_treeView.SetItemText(newsdata->m_feedTreeItem, txt);
			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsMarkUnread(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL && !newsdata->m_unread)
		{
			ADODB::_CommandPtr command;
			command.CreateInstance(__uuidof(ADODB::Command));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "UPDATE News SET Unread='1' WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
			ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
			newsdata->m_unread = true;

			if(newsdata->m_flagged)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
			else if(newsdata->m_unread)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
			else
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);

			FeedData* feeddata = dynamic_cast<FeedData*>((FeedData*)m_treeView.GetItemData(newsdata->m_feedTreeItem));
			feeddata->m_unread++;
			CAtlString txt;
			m_treeView.GetItemText(newsdata->m_feedTreeItem, txt);
			m_treeView.SetItemText(newsdata->m_feedTreeItem, txt);
			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsMarkFlagged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL && !newsdata->m_flagged)
		{
			ADODB::_CommandPtr command;
			command.CreateInstance(__uuidof(ADODB::Command));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "UPDATE News SET Flagged='1' WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
			ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
			newsdata->m_flagged = true;

			if(newsdata->m_flagged)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
			else if(newsdata->m_unread)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
			else
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);

			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsMarkUnflagged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL && newsdata->m_flagged)
		{
			ADODB::_CommandPtr command;
			command.CreateInstance(__uuidof(ADODB::Command));
			ATLASSERT(command != NULL);
			command->ActiveConnection = m_connection;
			command->CommandText = "UPDATE News SET Flagged='0' WHERE ID=?";
			command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
			ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
			newsdata->m_flagged = false;

			if(newsdata->m_flagged)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
			else if(newsdata->m_unread)
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
			else
				m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);

			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsSendMail(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		int idx = m_listView.GetSelectedIndex();
		NewsData* newsdata = dynamic_cast<NewsData*>((ListData*)m_listView.GetItemData(idx));

		if(newsdata != NULL)
		{
			MapiMessage mm;
			::ZeroMemory(&mm, sizeof(MapiMessage));
			CAtlString note;
			note.Format("%s\n\n%s\n\nSent by FeedIt!", newsdata->m_description, newsdata->m_url);
			mm.lpszSubject = newsdata->m_title.GetBuffer();
			mm.lpszNoteText = note.GetBuffer();
			const HINSTANCE hMAPILib = ::LoadLibrary("MAPI32.DLL");

			if(hMAPILib)
			{
				LPMAPISENDMAIL lpMAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hMAPILib, "MAPISendMail");

				if(lpMAPISendMail != NULL)
				{
					lpMAPISendMail(NULL, (ULONG_PTR)m_hWnd, &mm, MAPI_LOGON_UI | MAPI_DIALOG, 0);
				}

				::FreeLibrary(hMAPILib);
			}
		}

		return 0;
	}

	LRESULT OnActionsMarkAllRead(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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
					ADODB::_CommandPtr command;
					command.CreateInstance(__uuidof(ADODB::Command));
					ATLASSERT(command != NULL);
					command->ActiveConnection = m_connection;
					command->CommandText = "UPDATE News SET Unread='0' WHERE ID=?";
					command->GetParameters()->Append(command->CreateParameter(_bstr_t(), ADODB::adInteger, ADODB::adParamInput, NULL, _variant_t(newsdata->m_id)));
					ADODB::_RecordsetPtr recordset = command->Execute(NULL, NULL, 0);
					newsdata->m_unread = false;

					if(newsdata->m_flagged)
						m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 2, 0, 0, 0);
					else if(newsdata->m_unread)
						m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 1, 0, 0, 0);
					else
						m_listView.SetItem(idx, 0, LVIF_IMAGE, NULL, 0, 0, 0, 0);
				}
			}

			RefreshTree();
			m_listView.Invalidate();
		}

		return 0;
	}

	LRESULT OnActionsUpdateFeeds(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_forceUpdate = TRUE;
		LARGE_INTEGER liDueTime;
		liDueTime.QuadPart = -10000;
		::SetWaitableTimer(m_hTimer, &liDueTime, 0,  NULL, NULL, FALSE);
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
		HWND focus = ::GetFocus();

		if(focus == m_SrcBar.m_hWnd || m_SrcBar.IsChild(focus))
		{
			m_treeView.SetFocus();
		}
		else if(focus == m_treeView.m_hWnd || m_treeView.IsChild(focus))
		{
			m_listView.SetFocus();
		}
		else if(focus == m_listView.m_hWnd || m_listView.IsChild(focus))
		{
			m_htmlView.SetFocus();
		}
		else
		{
			m_SrcBar.SetFocus();
		}

		return 0;
	}

	LRESULT OnPrevPane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		HWND focus = ::GetFocus();

		if(focus == m_SrcBar.m_hWnd || m_SrcBar.IsChild(focus))
		{
			m_htmlView.SetFocus();
		}
		else if(focus == m_treeView.m_hWnd || m_treeView.IsChild(focus))
		{
			m_SrcBar.SetFocus();
		}
		else if(focus == m_listView.m_hWnd || m_listView.IsChild(focus))
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

		if(nMessage >= WM_KEYFIRST && nMessage <= WM_KEYLAST &&
			(wParam == VK_TAB || wParam == VK_BACK || wParam == VK_RETURN || wParam == VK_UP || wParam == VK_DOWN || wParam == VK_LEFT || wParam == VK_RIGHT))
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
