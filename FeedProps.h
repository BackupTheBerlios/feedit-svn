class CFeedPropertiesPage :
	public CPropertyPageImpl<CFeedPropertiesPage>,
	public CWinDataExchange<CFeedPropertiesPage>
{
public:
	enum { IDD = IDD_FEED_PROPERTIES_PAGE };

	CFeedPropertiesPage()
	{
		m_psp.hInstance = _Module.GetResourceInstance();
	}

	~CFeedPropertiesPage()
	{
	}

	BEGIN_MSG_MAP(CFeedPropertiesPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CPropertyPageImpl<CFeedPropertiesPage>)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CFeedPropertiesPage)
		DDX_TEXT(IDC_FEED_NAME, m_name)
		DDX_TEXT(IDC_FEED_URL, m_url)
		DDX_RADIO(IDC_FEED_BROWSE_DEFAULT, m_browse)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		m_updateCombo.Attach(GetDlgItem(IDC_FEED_UPDATE));
		m_updateCombo.AddString("Default");
		m_updateCombo.AddString("Never");
		m_updateCombo.AddString("Every 5 minutes");
		m_updateCombo.AddString("Every 15 minutes");
		m_updateCombo.AddString("Every 30 minutes");
		m_updateCombo.AddString("Every 60 minutes");

		if(m_update < 0)
			m_updateCombo.SetCurSel(0);
		else if(m_update == 0)
			m_updateCombo.SetCurSel(1);
		else if(m_update > 0 && m_update <= 5)
			m_updateCombo.SetCurSel(2);
		else if(m_update > 5 && m_update <= 15)
			m_updateCombo.SetCurSel(3);
		else if(m_update > 15 && m_update <= 30)
			m_updateCombo.SetCurSel(4);
		else
			m_updateCombo.SetCurSel(5);

		m_retainCombo.Attach(GetDlgItem(IDC_FEED_RETAIN));
		m_retainCombo.AddString("Default");
		m_retainCombo.AddString("Forever");
		m_retainCombo.AddString("1 day");
		m_retainCombo.AddString("7 days");
		m_retainCombo.AddString("30 days");

		if(m_retain < 0)
			m_retainCombo.SetCurSel(0);
		else if(m_retain == 0)
			m_retainCombo.SetCurSel(1);
		else if(m_retain == 1)
			m_retainCombo.SetCurSel(2);
		else if(m_retain > 1 && m_retain <= 7)
			m_retainCombo.SetCurSel(3);
		else
			m_retainCombo.SetCurSel(4);

		DoDataExchange(false);
		CenterWindow(GetParent());
		return 0;
	}

	int OnApply()
	{
		switch(m_updateCombo.GetCurSel())
		{
		case 0:
			m_update = -1;
			break;
		case 1:
			m_update = 0;
			break;
		case 2:
			m_update = 5;
			break;
		case 3:
			m_update = 15;
			break;
		case 4:
			m_update = 30;
			break;
		default:
			m_update = 60;
			break;
		}

		switch(m_retainCombo.GetCurSel())
		{
		case 0:
			m_retain = -1;
			break;
		case 1:
			m_retain = 0;
			break;
		case 2:
			m_retain = 1;
			break;
		case 3:
			m_retain = 7;
			break;
		default:
			m_retain = 30;
			break;
		}

		return DoDataExchange(true) ? PSNRET_NOERROR : PSNRET_INVALID;
	}

	CAtlString m_name;
	CAtlString m_url;
	int m_update;
	int m_retain;
	int m_browse;

protected:
	CComboBox m_updateCombo;
	CComboBox m_retainCombo;
};

class CFeedPropertySheet :
	public CPropertySheetImpl<CFeedPropertySheet>
{
public:
	CFeedPropertySheet(_U_STRINGorID title = (LPCTSTR)NULL, UINT uStartPage = 0, HWND hWndParent = NULL) :
		CPropertySheetImpl<CFeedPropertySheet>(title, uStartPage, hWndParent)
	{
		m_psh.dwFlags |= PSH_NOAPPLYNOW;
		AddPage(m_propertiesPage);
	}

	BEGIN_MSG_MAP(CFeedPropertySheet)
		MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
		CHAIN_MSG_MAP(CPropertySheetImpl<CFeedPropertySheet>)
	END_MSG_MAP()

	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		CenterWindow(m_psh.hwndParent);
		return 0;
	}

	CFeedPropertiesPage m_propertiesPage;
};
