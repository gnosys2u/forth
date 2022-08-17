#pragma once
// pch.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#if defined(WIN32) || defined(WIN64)
#define WINDOWS_BUILD
#endif

#if defined(WINDOWS_BUILD)

#if defined(WIN32)
#ifdef MSDEV
// compile for WinVista or newer
#define WINVER 0x0600

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afx.h>
#include <afxwin.h>
#include <afxtempl.h>
#include <strsafe.h>
#else
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ASSERT(x, ...)
#endif

#elif defined(WIN64)
#include <SDKDDKVer.h>
#include <WinSock2.h>
#include <windows.h>
#include <winnt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>
#endif

#else  // not a WINDOWS_BUILD

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef RASPI
#define ASSERT(x, ...)
#endif

#endif

#include <assert.h>
#include <math.h>
#include <stdint.h>

#if defined(WINDOWS_BUILD)
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <ctype.h>
#elif defined(LINUX)
#include <unistd.h>
#endif

#include <cassert>

#define ASSERT assert

#ifndef ulong
#define ulong unsigned long
#endif

#if 0
#include "Forth.h"
#include "ForthEngine.h"
#include "ForthForgettable.h"
#include "ForthInner.h"
#include "ForthInput.h"
#include "ForthShell.h"
#include "ForthStructs.h"
#include "ForthThread.h"
#include "ForthVocabulary.h"
#endif
