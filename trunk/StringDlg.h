// stringdlg.h : interface of the CStringDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CStringDlg :
	public CDialogImpl<CStringDlg>,
	public CWinDataExchange<CStringDlg>
{
public:
	enum { IDD = IDD_STRING };

	CAtlString m_value;

	CStringDlg(LPCTSTR prompt)
	{
		m_prompt = prompt;
	}

private:
	CStatic m_promptCtrl;
	CEdit m_valueCtrl;
	CAtlString m_prompt;

	BEGIN_MSG_MAP(CStringDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
		COMMAND_ID_HANDLER(IDOK, OnOk)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CStringDlg)
		DDX_TEXT(IDC_PROMPT, m_prompt)
		DDX_TEXT(IDC_VALUE, m_value)
	END_DDX_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(false);
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CEdit ctrl;
		ctrl.Attach(GetDlgItem(IDC_VALUE));
		ctrl.SetSel(0, -1);
		ctrl.SetFocus();
		return TRUE;
	}

	LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(true);
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};
