// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER		0x0400
//#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100

#include <atlstr.h>
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <atlctl.h>
#include <atlsplit.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlmisc.h>
#include <exdispid.h>

#import "C:\WINDOWS\System32\SHDOCVW.DLL" named_guids
#import "C:\WINDOWS\System32\MSXML2.DLL"
#import "C:\Program Files\Common Files\System\ADO\MSADO15.DLL" rename("EOF","EndOfFile")
#import "C:\Program Files\Common Files\System\ADO\MSADOX.DLL"
