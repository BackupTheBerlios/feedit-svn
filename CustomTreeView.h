#pragma once

class CCustomTreeViewCtrl :
	public CWindowImpl<CCustomTreeViewCtrl, CTreeViewCtrl>,
	public CCustomDraw<CCustomTreeViewCtrl>
{
public:
	BEGIN_MSG_MAP(CCustomTreeViewCtrl)    
		CHAIN_MSG_MAP(CCustomDraw<CCustomTreeViewCtrl>)
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
			return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
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
			NMTVCUSTOMDRAW* pTVCD = reinterpret_cast<NMTVCUSTOMDRAW*>(lpNMCustomDraw);
			FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)GetItemData((HTREEITEM)pTVCD->nmcd.dwItemSpec));
			if(feeddata != NULL && feeddata->m_unread > 0)
				::SelectObject(pTVCD->nmcd.hdc, AtlCreateBoldFont());
			return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
		}
		else
		{
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
	}

	DWORD OnItemPostPaint(int idCtrl, LPNMCUSTOMDRAW lpNMCustomDraw)
	{
		if(::IsWindow(m_hWnd) && idCtrl == GetDlgCtrlID())
		{
			NMTVCUSTOMDRAW* pTVCD = reinterpret_cast<NMTVCUSTOMDRAW*>(lpNMCustomDraw);
			FeedData* feeddata = dynamic_cast<FeedData*>((TreeData*)GetItemData((HTREEITEM)pTVCD->nmcd.dwItemSpec));
			if(feeddata != NULL && feeddata->m_unread > 0)
			{
				CAtlString txt;
				txt.Format("(%d)", feeddata->m_unread);
				CRect rc;
				GetItemRect((HTREEITEM)pTVCD->nmcd.dwItemSpec, &rc, TRUE);
				rc.OffsetRect(rc.right-rc.left, 0);
				COLORREF oldcolor = ::SetTextColor(pTVCD->nmcd.hdc, RGB(0, 0, 255));
				::DrawText(pTVCD->nmcd.hdc, txt, -1, &rc, DT_LEFT);
				::SetTextColor(pTVCD->nmcd.hdc, oldcolor);
			}
			return CDRF_DODEFAULT;
		}
		else
		{
			SetMsgHandled(FALSE);
			return CDRF_DODEFAULT;
		}
	}
};
