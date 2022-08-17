// ForthDLLMain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "windows.h"
#include "..\ForthDLL\ForthDLL.h"


static bool InitSystem()
{
    // TODO!
    return true;
}

int main(int argc, char* argv[], char* envp[])
{
    int nRetCode = 0;
    ForthShell* pShell = NULL;
    ForthInputStream* pInStream = NULL;

    if (!InitSystem())
    {
        nRetCode = 1;
    }
    else
    {
        nRetCode = 1;
        pShell = CreateForthShell(argc, (const char**)(argv), (const char**)envp);

        if (argc > 1)
        {

            //
            // interpret the forth file named on the command line
            //
            FILE* pInFile;
            errno_t err = fopen_s(&pInFile, argv[1], "r");
            if (err == 0)
            {
                pInStream = CreateForthFileInputStream(pInFile, argv[1], 10000, true);
                nRetCode = pShell->Run(pInStream);
                fclose(pInFile);

            }
        }
        else
        {
            //
            // run forth in interactive mode
            //
            pInStream = CreateForthConsoleInputStream();
            nRetCode = pShell->Run(pInStream);

        }
        delete pShell;
    }

    // NOTE: this will always report a 64 byte memory leak (a CDynLinkLibrary object) that it causes itself
    /*
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, hLoggingPipe);    // or _CRTDBG_FILE_STDERR?
    _CrtDumpMemoryLeaks();
    _CrtSetReportMode(_CRT_WARN, 0);
    */
    return nRetCode;

    //int fooVal = foo();
    //std::cout << "foo say:" << fooVal;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
