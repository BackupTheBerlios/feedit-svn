// stringdlg.h : interface of the CStringDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CStringDlg : public CDialogImpl<CStringDlg>
{
public:
	enum { IDD = IDD_STRING };

	ATL::CString m_value;

	CStringDlg(LPCTSTR prompt)
	{
		m_prompt = prompt;
	}

private:
	CStatic m_promptCtrl;
	CEdit m_valueCtrl;
	ATL::CString m_prompt;

	BEGIN_MSG_MAP(CStringDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
		COMMAND_ID_HANDLER(IDOK, OnOk)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_promptCtrl.Attach(GetDlgItem(IDC_PROMPT));
		m_promptCtrl.SetWindowText(m_prompt);
		m_valueCtrl.Attach(GetDlgItem(IDC_VALUE));
		m_valueCtrl.SetWindowText(m_value);
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_valueCtrl.SetSel(0, -1);
		m_valueCtrl.SetFocus();
		return TRUE;
	}

	LRESULT OnOk(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		m_valueCtrl.GetWindowText(m_value);
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};
