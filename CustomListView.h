#pragma once

class CCustomListViewCtrl :
	public CWindowImpl<CCustomListViewCtrl, CListViewCtrl>,
	public CCustomDraw<CCustomListViewCtrl>
{
public:
	BEGIN_MSG_MAP(CCustomListViewCtrl)    
		CHAIN_MSG_MAP(CCustomDraw<CCustomListViewCtrl>)
	END_MSG_MAP()

#if (_WTL_VER >= 0x0700)
	BOOL m_bHandledCD;
	BOOL IsMsgHandled() const
	{
		return m_bHandledCD;
	}
	
	void SetMsgHandled(BOOL bHandled)
	{
		m_bHandledCD = bHandled;
	}
#endif //(_WTL_VER >= 0x0700)

	DWORD OnPrePaint(int idCtrl, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
	{
		if(::IsWindow(m_hWnd) && idCtrl == GetDlgCtrlID())
		{
			return CDRF_NOTIFYITEMDRAW;
		}
		else
		{
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
	}

	DWORD OnItemPrePaint(int idCtrl, LPNMCUSTOMDRAW lpNMCustomDraw)
	{
		if(::IsWindow(m_hWnd) && idCtrl == GetDlgCtrlID())
		{
			NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(lpNMCustomDraw);
			LVITEM lvi = { 0 };
			lvi.iItem = pLVCD->nmcd.dwItemSpec;
			lvi.mask = LVIF_IMAGE;
			GetItem(&lvi);
			if(lvi.iImage == 1)
				::SelectObject(pLVCD->nmcd.hdc, AtlCreateBoldFont());
			return CDRF_NEWFONT;
		}
		else
		{
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
	}
};
