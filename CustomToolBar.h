/////////////////////////////////////////////////////////////////////////////
// Customizable toolbar

// Simple implementation of ToolBar customization support.
// It handles only one ToolBar in total!
// Let the parent window derive from the class; chain it in the message map.
// Call InitToolbar() after creating the ToolBar control.
template< class T >
class CCustomizableToolBarCommands
{
public:
	typedef CCustomizableToolBarCommands<T> thisClass;

	CSimpleArray<TBBUTTON> m_aButtons;

	// Operations

	BOOL InitToolBar(HWND hWndToolBar, UINT nResource, BOOL bInitialSeparator = FALSE)
	{
		ATLASSERT(::IsWindow(hWndToolBar));

		// The ToolBar is adjustable!
		CToolBarCtrl tb = hWndToolBar;
		ATLASSERT(tb.GetStyle() & CCS_ADJUSTABLE); // Need this style to properly function on XP!

		// Gather information about toolbar buttons by building the toolbar.
		// Needed so we can reset it later on.
		// This code is almost identical to the CFrameWindowImplBase::CreateSimpleToolBarCtrl
		// code, but it also needs to build it the exact same way...
		HINSTANCE hInst = _Module.GetResourceInstance();
		HRSRC hRsrc = ::FindResource(hInst, (LPCTSTR) nResource, RT_TOOLBAR);
		if(hRsrc == NULL)
			return FALSE;
		HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
		if(hGlobal == NULL)
			return FALSE;

		struct _AtlToolBarData
		{
			WORD wVersion;
			WORD wWidth;
			WORD wHeight;
			WORD wItemCount;
			//WORD aItems[wItemCount]
			WORD* items() { return (WORD*)(this+1); }
		};
		_AtlToolBarData* pData = (_AtlToolBarData*)::LockResource(hGlobal);
		if(pData == NULL)
			return FALSE;
		ATLASSERT(pData->wVersion==1);

		WORD* pItems = pData->items();
		// Set initial separator (half width)
		if(bInitialSeparator)
		{
			TBBUTTON bt;
			bt.iBitmap = 4;
			bt.idCommand = 0;
			bt.fsState = 0;
			bt.fsStyle = TBSTYLE_SEP;
			bt.dwData = 0;
			bt.iString = 0;
			m_aButtons.Add(bt);
		}
		// Scan other buttons
		int nBmp = 0;
		for(int i=0, j= bInitialSeparator ? 1 : 0; i<pData->wItemCount; i++, j++)
		{
			if(pItems[i] != 0)
			{
				TBBUTTON bt;
				bt.iBitmap = nBmp++;
				bt.idCommand = pItems[i];
				bt.fsState = TBSTATE_ENABLED;
				bt.fsStyle = TBSTYLE_BUTTON;
				bt.dwData = 0;
				bt.iString = 0;
				m_aButtons.Add(bt);
			}
			else
			{
				TBBUTTON bt;
				bt.iBitmap = 8;
				bt.idCommand = 0;
				bt.fsState = 0;
				bt.fsStyle = TBSTYLE_SEP;
				bt.dwData = 0;
				bt.iString = 0;
				m_aButtons.Add(bt);
			}
		}
		return TRUE;
	}

	// Message map and handler

	BEGIN_MSG_MAP(thisClass)
		NOTIFY_CODE_HANDLER(TBN_BEGINADJUST, OnTbBeginAdjust)
		NOTIFY_CODE_HANDLER(TBN_INITCUSTOMIZE, OnTbInitCustomize)
		NOTIFY_CODE_HANDLER(TBN_ENDADJUST, OnTbEndAdjust)
		NOTIFY_CODE_HANDLER(TBN_RESET, OnTbReset)
		NOTIFY_CODE_HANDLER(TBN_TOOLBARCHANGE, OnTbToolBarChange)
		NOTIFY_CODE_HANDLER(TBN_QUERYINSERT, OnTbQueryInsert)
		NOTIFY_CODE_HANDLER(TBN_QUERYDELETE, OnTbQueryDelete)
		NOTIFY_CODE_HANDLER(TBN_GETBUTTONINFO, OnTbGetButtonInfo)      
	END_MSG_MAP()

	LRESULT OnTbBeginAdjust(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
	{
		ATLASSERT(m_aButtons.GetSize() > 0); // Remember to call InitToolBar()!
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTbInitCustomize(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		return TBNRF_HIDEHELP;
	}

	LRESULT OnTbEndAdjust(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		return TRUE;
	}

	LRESULT OnTbToolBarChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
	{
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnTbReset(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPTBNOTIFY lpTbNotify = (LPTBNOTIFY) pnmh;
		// Restore the old button-set
		CToolBarCtrl tb = lpTbNotify->hdr.hwndFrom;
		while(tb.GetButtonCount() > 0)
			tb.DeleteButton(0);
		tb.AddButtons(m_aButtons.GetSize(), m_aButtons.GetData());
		return TRUE;
	}

	LRESULT OnTbQueryInsert(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		// Allow all buttons!
		return TRUE;
	}

	LRESULT OnTbQueryDelete(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	{
		// All buttons can be deleted!
		return TRUE;
	}

	LRESULT OnTbGetButtonInfo(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		LPTBNOTIFY lpTbNotify = (LPTBNOTIFY) pnmh;
		CToolBarCtrl tb = lpTbNotify->hdr.hwndFrom;
		// The toolbar requests information about buttons that we don't know of...
		if(lpTbNotify->iItem >= m_aButtons.GetSize())
			return FALSE;
		// Locate tooltip text and copy second half of it.
		// This is the same code as CFrameWindowImplBase uses, despite how 
		// dangerous it may look...
		TCHAR szBuff[256] = { 0 };
		LPCTSTR pstr = szBuff;
#if (_ATL_VER < 0x0700)
		int nRet = ::LoadString(_Module.GetResourceInstance(), m_aButtons[lpTbNotify->iItem].idCommand, szBuff, 255);
#else
		int nRet = ATL::AtlLoadString(m_aButtons[lpTbNotify->iItem].idCommand, szBuff, 255);
#endif
		for(int i = 0; i < nRet; i++)
		{
			if(szBuff[i] == _T('\n'))
			{
				pstr = szBuff + i + 1;
				break;
			}
		}
		lpTbNotify->tbButton = m_aButtons[lpTbNotify->iItem];
		::lstrcpyn(lpTbNotify->pszText, pstr, lpTbNotify->cchText);
		lpTbNotify->cchText = ::lstrlen(pstr);
		return TRUE;
	}
};
