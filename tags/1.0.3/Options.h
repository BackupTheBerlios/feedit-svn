class COptionsPage :
	public CPropertyPageImpl<COptionsPage>,
	public CWinDataExchange<COptionsPage>
{
public:
	enum { IDD = IDD_OPTIONS_PAGE };

	COptionsPage()
	{
		m_psp.hInstance = _Module.GetResourceInstance();
	}

	~COptionsPage()
	{
	}

	BEGIN_MSG_MAP(COptionsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CPropertyPageImpl<COptionsPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(COptionsPage)
		DDX_CHECK(IDC_OPTIONS_AUTOSTART, m_autostart)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_updateCombo.Attach(GetDlgItem(IDC_OPTIONS_UPDATE));
		m_updateCombo.AddString("Never");
		m_updateCombo.AddString("Every 5 minutes");
		m_updateCombo.AddString("Every 15 minutes");
		m_updateCombo.AddString("Every 30 minutes");
		m_updateCombo.AddString("Every 60 minutes");

		if(m_update == 0)
			m_updateCombo.SetCurSel(0);
		else if(m_update > 0 && m_update <= 5)
			m_updateCombo.SetCurSel(1);
		else if(m_update > 5 && m_update <= 15)
			m_updateCombo.SetCurSel(2);
		else if(m_update > 15 && m_update <= 30)
			m_updateCombo.SetCurSel(3);
		else
			m_updateCombo.SetCurSel(4);

		m_retainCombo.Attach(GetDlgItem(IDC_OPTIONS_RETAIN));
		m_retainCombo.AddString("Forever");
		m_retainCombo.AddString("1 day");
		m_retainCombo.AddString("7 days");
		m_retainCombo.AddString("30 days");

		if(m_retain == 0)
			m_retainCombo.SetCurSel(0);
		else if(m_retain == 1)
			m_retainCombo.SetCurSel(1);
		else if(m_retain > 1 && m_retain <= 7)
			m_retainCombo.SetCurSel(2);
		else
			m_retainCombo.SetCurSel(3);

		DoDataExchange(false);
		CenterWindow(GetParent());
		return 0;
	}

	int OnApply()
	{
		switch(m_updateCombo.GetCurSel())
		{
		case 0:
			m_update = 0;
			break;
		case 1:
			m_update = 5;
			break;
		case 2:
			m_update = 15;
			break;
		case 3:
			m_update = 30;
			break;
		default:
			m_update = 60;
			break;
		}

		switch(m_retainCombo.GetCurSel())
		{
		case 0:
			m_retain = 0;
			break;
		case 1:
			m_retain = 1;
			break;
		case 2:
			m_retain = 7;
			break;
		default:
			m_retain = 30;
			break;
		}

		return DoDataExchange(true) ? PSNRET_NOERROR : PSNRET_INVALID;
	}

	bool m_autostart;
	int m_update;
	int m_retain;

protected:
	CComboBox m_updateCombo;
	CComboBox m_retainCombo;
};

class COptionsPropertySheet :
	public CPropertySheetImpl<COptionsPropertySheet>
{
public:
	COptionsPropertySheet(_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL) :
		CPropertySheetImpl<COptionsPropertySheet>(title, uStartPage, hWndParent)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
		AddPage(m_optionsPage);
	}

	BEGIN_MSG_MAP(COptionsPropertySheet)
		MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
		CHAIN_MSG_MAP(CPropertySheetImpl<COptionsPropertySheet>)
	END_MSG_MAP()

	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(m_psh.hwndParent);
		return 0;
	}

	COptionsPage m_optionsPage;
};
