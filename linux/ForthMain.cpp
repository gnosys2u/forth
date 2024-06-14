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
//  main.cpp
//  Forth
//
//  Created by Pat McElhatton on 3/28/17.
//  Copyright © 2017 Pat McElhatton. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "Forth.h"
#include "Shell.h"

static int loggerFD = -1;

void OutputToLogger(const char* pBuffer)
{
    if (loggerFD < 0)
    {
        const char* myfifo = "/tmp/forthLoggerFIFO";
        
        loggerFD = open(myfifo, O_WRONLY | O_NONBLOCK);
    }
    
    if (loggerFD >= 0)
    {
        write(loggerFD, pBuffer, strlen(pBuffer) + 1);
    }
    //close(loggerFD);
    
    /* remove the FIFO */
    //unlink(myfifo);
}

int main(int argc, const char * argv[], char * envp[])
{
    int nRetCode = 0;
    Shell *pShell = NULL;
    InputStream *pInStream = NULL;
    
    /*
     if ( !InitSystem() )
    {
        nRetCode = 1;
    }
    else*/
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
    
    return nRetCode;
}



