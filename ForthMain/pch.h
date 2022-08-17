#pragma once
// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#if defined(WIN64)

#include "targetver.h"

// TODO: reference additional headers your program requires here

#include <windows.h>
#include <winnt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>

#include <math.h>
#include <stdint.h>

#include <conio.h>
#include <direct.h>
#include <io.h>
#include <ctype.h>

#include <cassert>

#define ASSERT assert
#else

// compile for WinVista or newer
#define WINVER 0x0600

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>

#endif