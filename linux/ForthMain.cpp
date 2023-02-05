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



