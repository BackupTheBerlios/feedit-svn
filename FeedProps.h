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
		DDX_INT_RANGE(IDC_FEED_UPDATE, m_update, 10, 1440)
	END_DDX_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		DoDataExchange(false);
		return 0;
	}

	int OnApply()
	{
		return DoDataExchange(true) ? PSNRET_NOERROR : PSNRET_INVALID;
	}

	CAtlString m_name;
	CAtlString m_url;
	int m_update;
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
