// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#import "shdocvw.dll" named_guids

class CMainFrame : public CFrameWindowImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter, public CIdleHandler,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDocHostUIHandlerDispatch, &IID_IDocHostUIHandlerDispatch, &LIBID_ATLLib>,
	public IDispEventImpl<0, CMainFrame, &SHDocVw::DIID_DWebBrowserEvents2, &SHDocVw::LIBID_SHDocVw, 1, 0>
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CCommandBarCtrl m_CmdBar;
	CMultiPaneStatusBarCtrl m_statusBar;
	CProgressBarCtrl m_progressBar;
	CSplitterWindow m_vSplit;
	CHorSplitterWindow m_hSplit;
	CTreeViewCtrl m_treeView;
	CListViewCtrl m_listView;
	CAxWindow m_htmlView;
	CComPtr<IWebBrowser2> m_htmlCtrl;
	CImageList m_dragImage;
	int m_downloads;
	BOOL m_dragging;
	HTREEITEM m_feedsRoot;
	HTREEITEM m_itemDrag;
	HTREEITEM m_itemDrop;
	HCURSOR m_arrowCursor;
	HCURSOR m_noCursor;

	CMainFrame() : m_downloads(0), m_dragging(FALSE), m_feedsRoot(NULL), m_itemDrag(NULL), m_itemDrop(NULL),
		m_arrowCursor(LoadCursor(NULL, IDC_ARROW)), m_noCursor(LoadCursor(NULL, IDC_NO))
	{
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
			return TRUE;

		return FALSE;
	}

	virtual BOOL OnIdle()
	{
		UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
		NOTIFY_HANDLER(IDC_TREE, TVN_BEGINDRAG, OnBeginDrag)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
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
	END_SINK_MAP()

HTREEITEM MoveChildItem(HTREEITEM hItem, HTREEITEM htiNewParent, HTREEITEM htiAfter)
{
	TV_INSERTSTRUCT tvstruct;
	HTREEITEM hNewItem;
	ATL::CString sText;

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

		// client rect for horizontal splitter
		CRect rcHorz;
		GetClientRect(&rcHorz);

		// create the horizontal splitter. Note that vSplit is parent of hSplit
		m_hSplit.Create(m_vSplit.m_hWnd, rcHorz, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		// add the horizontal splitter to the right pane (1) of vertical splitter
		m_vSplit.SetSplitterPane(1, m_hSplit);

		m_treeView.Create(m_vSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT, WS_EX_CLIENTEDGE);
		m_treeView.SetDlgCtrlID(IDC_TREE);
		CImageList il;
		il.Create(IDB_TREE_IMAGELIST, 16, 2, RGB(255, 0 ,255));
		m_treeView.SetImageList(il, TVSIL_NORMAL);
		m_vSplit.SetSplitterPane(0, m_treeView);

		m_listView.Create(m_hSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SINGLESEL, WS_EX_CLIENTEDGE);
		m_hSplit.SetSplitterPane(0, m_listView);

		m_htmlView.Create(m_hSplit.m_hWnd, rcDefault, _T("about:blank"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL, WS_EX_CLIENTEDGE);
		m_hSplit.SetSplitterPane(1, m_htmlView);
		m_htmlView.SetExternalUIHandler(static_cast<IDocHostUIHandlerDispatch*>(this));
		m_htmlView.QueryControl(&m_htmlCtrl);
		DispEventAdvise(m_htmlCtrl);
		CComVariant u("http://www.microsoft.com");
		CComVariant v;
		m_htmlCtrl->Navigate2(&u, &v, &v, &v, &v);

		m_hWndClient = m_vSplit.m_hWnd;

		// set the vertical splitter parameters
		m_vSplit.m_cxyMin = 100; // minimum size
		m_vSplit.SetSplitterPos(150); // from left

		// set the horizontal splitter parameters
		m_hSplit.m_cxyMin = 100; // minimum size
		m_hSplit.SetSplitterPos(150); // from top

		m_feedsRoot = m_treeView.InsertItem(_T("Feeds"), TVI_ROOT, TVI_LAST);

		m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
		m_listView.AddColumn("Date", 0);
		m_listView.AddColumn("Headline", 1);
		m_listView.AddColumn("Topic", 2);
		m_listView.SetColumnWidth(0, 100);
		m_listView.SetColumnWidth(1, 300);
		m_listView.SetColumnWidth(2, 150);

		m_listView.InsertItem(0, "01/01/2004");
		m_listView.AddItem(0, 1, "FeedIt rulez!");
		m_listView.AddItem(0, 2, "development");

		// register object for message filtering and idle updates
		CMessageLoop* pLoop = _Module.GetMessageLoop();
		ATLASSERT(pLoop != NULL);
		pLoop->AddMessageFilter(this);
		pLoop->AddIdleHandler(this);

		return 0;
	}

	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
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
				m_treeView.Expand(m_itemDrop, TVE_EXPAND);
				HTREEITEM htiNew = MoveChildItem(m_itemDrag, m_itemDrop, TVI_LAST);
				m_treeView.DeleteItem(m_itemDrag);
				m_treeView.SelectItem(htiNew);
			}
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

	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStringDlg dlg(_T("Enter feed URL:"));
		dlg.m_value = _T("http://");

		if(dlg.DoModal() == IDOK)
		{
			FeedData* itemdata = new FeedData();
			itemdata->m_name = dlg.m_value;
			itemdata->m_url = dlg.m_value;
			HTREEITEM item = m_treeView.InsertItem(dlg.m_value, m_feedsRoot, TVI_LAST);
			m_treeView.SetItemImage(item, 1, 1);
			m_treeView.SetItemData(item, (DWORD_PTR)itemdata);
			m_treeView.SortChildren(m_feedsRoot, TRUE);
		}

		return 0;
	}

	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		CStringDlg dlg(_T("Enter folder name:"));
		dlg.m_value = _T("New");

		if(dlg.DoModal() == IDOK)
		{
			FolderData* itemdata = new FolderData();
			itemdata->m_name = dlg.m_value;
			HTREEITEM item = m_treeView.InsertItem(dlg.m_value, m_feedsRoot, TVI_LAST);
			m_treeView.SetItemImage(item, 0, 0);
			m_treeView.SetItemData(item, (DWORD_PTR)itemdata);
			m_treeView.SortChildren(m_feedsRoot, TRUE);
		}

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
		return E_NOTIMPL;
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
};
