// ForthMain.cpp : Defines the entry point for the console application.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include "pch.h"

#if defined(WIN32) && !defined(MINGW)
#define AFX_BUILD 1
#endif

#if AFX_BUILD
#include "ForthMain.h"
#endif
#include "Shell.h"
#include "Input.h"


#if AFX_BUILD

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;
#endif

using namespace std;


static HANDLE hLoggingPipe = INVALID_HANDLE_VALUE;

void OutputToLogger(const char* pBuffer)
{
    //OutputDebugString(buffer);

    DWORD dwWritten;
    int bufferLen = 1 + strlen(pBuffer);

    if (hLoggingPipe == INVALID_HANDLE_VALUE)
    {
        hLoggingPipe = CreateFile(TEXT("\\\\.\\pipe\\ForthLogPipe"),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    }
    if (hLoggingPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hLoggingPipe,
            pBuffer,
            bufferLen,   // = length of string + terminating '\0' !!!
            &dwWritten,
            NULL);

        //CloseHandle(hPipe);
    }

    return;
}

void myInvalidParameterHandler(const wchar_t* expression,
	const wchar_t* function,
	const wchar_t* file,
	uint32_t line,
	uintptr_t pReserved)
{
    wprintf(L"Invalid parameter detected in function %s.", function);
	wprintf(L"Expression: %s\n", expression);
	//abort();
}

static bool InitSystem()
{
#if defined(FORTH64)
    _set_invalid_parameter_handler(myInvalidParameterHandler);
    //set_constraint_handler(ignore_handler_s);
#endif

#if AFX_BUILD

    // initialize MFC and print an error on failure
    if ( !AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0) )
    {
        // TODO: change error code to suit your needs
        cerr << _T("Fatal Error: MFC initialization failed") << endl;
		return false;
	}

#if !defined(FORTH64)
	_set_invalid_parameter_handler(myInvalidParameterHandler);
#endif
	_CrtSetReportMode(_CRT_ASSERT, 0);
#if 0
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = 0;
		GetConsoleMode(hOut, &dwMode);
		dwMode |= 4;// ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode);
	}

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hIn != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = 0;
		GetConsoleMode(hIn, &dwMode);
		dwMode |= 0x200;// ENABLE_VIRTUAL_TERMINAL_INPUT;
		SetConsoleMode(hIn, dwMode);
	}
#endif

#endif
	return true;
}

#if AFX_BUILD
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
#else
int main( int argc, char* argv[], char* envp[] )
#endif
{
    int nRetCode = 0;
    Shell *pShell = NULL;
    InputStream *pInStream = NULL;

	if ( !InitSystem() )
	{
		nRetCode = 1;
	}
    else
    {
		nRetCode = 1;
        pShell = new Shell(argc, (const char **)(argv), (const char **)envp);
#if 0
        if ( argc > 1 )
        {

            //
            // interpret the forth file named on the command line
            //
            FILE *pInFile = fopen( argv[1], "r" );
            if ( pInFile != NULL )
            {
                pInStream = new FileInputStream(pInFile);
                nRetCode = pShell->Run( pInStream );
                fclose( pInFile );

            }
        }
        else
#endif
        {
            //
            // run forth in interactive mode
            //
            pInStream = new ConsoleInputStream;
            nRetCode = pShell->Run( pInStream );

        }
        delete pShell;
    }

	// NOTE: this will always report a 64 byte memory leak (a CDynLinkLibrary object) that it causes itself
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, hLoggingPipe);    // or _CRTDBG_FILE_STDERR?
	_CrtDumpMemoryLeaks();
    _CrtSetReportMode(_CRT_WARN, 0);

	return nRetCode;
}


