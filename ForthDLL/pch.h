// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strsafe.h>
#include <stdint.h>
#include <tchar.h>
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <ctype.h>

#include <cassert>

#define ASSERT assert

#ifndef ulong
#define ulong unsigned long
#endif

// these had to be added to get ForthDLL DbgAsm and RelAsm configurations to compile
// I don't understand how these are different from Debug and Release for this case
# ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS 1
# endif
# ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
# define _WINSOCK_DEPRECATED_NO_WARNINGS 1
# endif
# if defined(_MSC_VER)
# ifndef _CRT_SECURE_NO_DEPRECATE
# define _CRT_SECURE_NO_DEPRECATE 1
# endif
# pragma warning(disable : 4996)
# endif

#endif
