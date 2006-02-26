#pragma once

class CNotifyIconData : public NOTIFYICONDATA
{
public:	
	CNotifyIconData()
	{
		::ZeroMemory(this, sizeof(NOTIFYICONDATA));
		cbSize = sizeof(NOTIFYICONDATA);
	}
};

template <class T>
class CTrayIconImpl
{
private:
	UINT WM_TRAYICON;
	CNotifyIconData m_nid;
	bool m_bInstalled;
	UINT m_nDefault;
public:	
	CTrayIconImpl() : m_bInstalled(false), m_nDefault(0)
	{
		WM_TRAYICON = ::RegisterWindowMessage(_T("WM_TRAYICON"));
	}
	
	~CTrayIconImpl()
	{
		RemoveIcon();
	}

	bool InstallIcon(LPCTSTR lpszToolTip, HICON hIcon, UINT nID)
	{
		T* pT = static_cast<T*>(this);
		m_nid.hWnd = pT->m_hWnd;
		m_nid.uID = nID;
		m_nid.hIcon = hIcon;
		m_nid.uCallbackMessage = WM_TRAYICON;
		m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		_tcsncpy(m_nid.szTip, lpszToolTip, sizeof(m_nid.szTip));
		m_bInstalled = Shell_NotifyIcon(NIM_ADD, &m_nid) ? true : false;

		if(m_bInstalled)
		{
			m_nid.uFlags = 0;
			m_nid.uVersion = NOTIFYICON_VERSION;
			Shell_NotifyIcon(NIM_SETVERSION, &m_nid);
		}

		return m_bInstalled;
	}

	bool RemoveIcon()
	{
		if (!m_bInstalled)
			return false;
		m_nid.uFlags = 0;
		return Shell_NotifyIcon(NIM_DELETE, &m_nid) ? true : false;
	}

	bool SetTooltipText(LPCTSTR pszTooltipText)
	{
		if (!m_bInstalled)
			return false;
		if (pszTooltipText == NULL)
			return FALSE;
		m_nid.uFlags = NIF_TIP;
		_tcsncpy(m_nid.szTip, pszTooltipText, sizeof(m_nid.szTip));
		return Shell_NotifyIcon(NIM_MODIFY, &m_nid) ? true : false;
	}

	bool Notify(const char* info, const char* infotitle, DWORD infoflags = NIIF_INFO, UINT timeout = 10000)
	{
		if (!m_bInstalled)
			return false;
		m_nid.uFlags = NIF_INFO;
		m_nid.dwInfoFlags = infoflags;
		m_nid.uTimeout = 10;
		_tcsncpy(m_nid.szInfo, info, sizeof(m_nid.szInfo));
		_tcsncpy(m_nid.szInfoTitle, infotitle, sizeof(m_nid.szInfoTitle));
		return Shell_NotifyIcon(NIM_MODIFY, &m_nid) ? true : false;
	}

	void SetDefaultItem(UINT nID)
	{
		m_nDefault = nID;
	}

	BEGIN_MSG_MAP(CTrayIcon)
		MESSAGE_HANDLER(WM_TRAYICON, OnTrayIcon)
	END_MSG_MAP()

	LRESULT OnTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		if (wParam != m_nid.uID)
			return 0;
		T* pT = static_cast<T*>(this);
		if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == NIN_KEYSELECT)
		{
			CMenu oMenu;
			if (!oMenu.LoadMenu(m_nid.uID))
				return 0;
			CMenuHandle oPopup(oMenu.GetSubMenu(0));
			pT->PrepareMenu(oPopup);
			CPoint pos;
			GetCursorPos(&pos);
			SetForegroundWindow(pT->m_hWnd);
			if (m_nDefault == 0)
				oPopup.SetMenuDefaultItem(0, TRUE);
			else
				oPopup.SetMenuDefaultItem(m_nDefault);
			oPopup.TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, pT->m_hWnd);
			pT->PostMessage(WM_NULL);
			oMenu.DestroyMenu();
		}
		else if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == NIN_BALLOONUSERCLICK)
		{
			SetForegroundWindow(pT->m_hWnd);
			CMenu oMenu;
			if (!oMenu.LoadMenu(m_nid.uID))
				return 0;
			CMenuHandle oPopup(oMenu.GetSubMenu(0));			
			if (m_nDefault)
			{
				pT->SendMessage(WM_COMMAND, m_nDefault, 0);
			}
			else
			{
				UINT nItem = oPopup.GetMenuItemID(0);
				pT->SendMessage(WM_COMMAND, nItem, 0);
			}
			oMenu.DestroyMenu();
		}
		return 0;
	}

	virtual void PrepareMenu(HMENU hMenu)
	{
	}
};
