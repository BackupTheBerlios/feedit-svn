#pragma once

class CSearchBand :
	public CDialogImpl<CSearchBand>,
	public CDialogResize<CSearchBand>
{
public:
	enum { IDD = IDD_SEARCH_BAND };

	CEdit m_search;

	BEGIN_MSG_MAP(CSearchBand)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CDialogResize<CSearchBand>)
		FORWARD_NOTIFICATIONS()
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CSearchBand)
		DLGRESIZE_CONTROL(IDC_SEARCHTEXT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_SEARCH, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

	BOOL IsDialogMessage(LPMSG pMsg)
	{
		if(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST && pMsg->wParam == VK_TAB)
			return FALSE;

		return CDialogImpl<CSearchBand>::IsDialogMessage(pMsg);
	}

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DlgResize_Init(false, false);
		m_search.Attach(GetDlgItem(IDC_SEARCHTEXT));
		return TRUE;
	}
};
