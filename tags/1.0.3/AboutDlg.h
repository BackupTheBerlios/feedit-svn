// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

	CStatic m_aboutText;
	CStatic m_email;

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_aboutText.Attach(GetDlgItem(IDC_ABOUTTEXT));

		TCHAR modulename[1024];
		::GetModuleFileName(NULL, modulename, sizeof(modulename));
		DWORD handle;
		DWORD versize = ::GetFileVersionInfoSize(modulename, &handle);

		if(versize > 0)
		{
			BYTE* verdata = new BYTE[versize];
			::GetFileVersionInfo(modulename, handle, versize, verdata);
			VS_FIXEDFILEINFO* buf;
			UINT len;
			::VerQueryValue(verdata, "\\", (LPVOID*)&buf, &len);
			CAtlString ver;

			if(len == sizeof(VS_FIXEDFILEINFO))
				ver.Format("%d.%d.%d.%d", HIWORD(buf->dwFileVersionMS), LOWORD(buf->dwFileVersionMS), HIWORD(buf->dwFileVersionLS), LOWORD(buf->dwFileVersionLS));

			CAtlString txt1;
			CAtlString txt2;
			m_aboutText.GetWindowText(txt1);
			txt2.Format(txt1, ver);
			m_aboutText.SetWindowText(txt2);
			delete[] verdata;
		}

		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
		EndDialog(wID);
		return 0;
	}
};
