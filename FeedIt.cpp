// FeedIt.cpp : main source file for FeedIt.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>

#include "resource.h"

#include "DataStructs.h"
#include "FeedParser.h"
#include "AboutDlg.h"
#include "StringDlg.h"
#include "CustomToolBar.h"
#include "CustomTreeView.h"
#include "CustomListView.h"
#include "TrayIcon.h"
#include "Settings.h"
#include "Options.h"
#include "FeedProps.h"
#include "SearchBand.h"
#include "MainFrm.h"

CAppModule _Module;

int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CComObjectGlobal<CMainFrame> wndMain;

	CAtlString cmdline(lpstrCmdLine);

	if(cmdline.Find("/background") >= 0)
		nCmdShow = SW_MINIMIZE;

	DWORD exstyle = 0;

	if(nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
		exstyle = WS_EX_TOOLWINDOW;

	if(wndMain.CreateEx(0, 0, 0, exstyle) == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	CWindowSettings ws;

	if(ws.Load("Software\\FeedIt", "MainFrame"))
		ws.ApplyTo(wndMain, nCmdShow);
	else
		wndMain.ShowWindow(nCmdShow);

	if(nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
		wndMain.ShowWindow(SW_HIDE);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	CAtlString mutexName;

	{
		TCHAR buf[1024];
		ULONG buflen = 1024;
		::GetUserName(buf, &buflen);
		mutexName += buf;
		mutexName += "@";
		buflen = 1024;
		::GetComputerName(buf, &buflen);
		mutexName += buf;
	}

	HANDLE hMutex = ::CreateMutex(NULL, FALSE, mutexName);

	if(::GetLastError() == ERROR_ALREADY_EXISTS)
	{
		::CloseHandle(hMutex);
		return 0;
	}

	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	AtlAxWinInit();

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();
	::CloseHandle(hMutex);
	return nRet;
}
