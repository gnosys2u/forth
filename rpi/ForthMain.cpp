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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
 
#include "Shell.h"
#include "ConsoleInputStream.h"

//#define LOG_TO_FILE 1

#ifdef LOG_TO_FILE
static FILE* loggerFile = nullptr;
#else
static int loggerFD = -1;
const char* loggerFifo = "/tmp/forthLoggerFIFO";
#endif

void OutputToLogger(const char* pBuffer)
{
#ifdef LOG_TO_FILE
    if (loggerFile == nullptr)
    {
	loggerFile = fopen("logfile.txt", "a");
    }
    
    int n = fprintf(loggerFile, "%s", pBuffer);
#else


    if (loggerFD < 0)
    {
	// create the FIFO (named pipe)
	unlink(loggerFifo);
	if (mkfifo(loggerFifo, 0666) < 0)
	{
		perror("error making logger fifo");
	}
	
	// ugh, it seems like O_NONBLOCK doesn't work on rpi, so forth will hang at startup if logger isn't running
	//loggerFD = open(loggerFifo, O_WRONLY | O_NONBLOCK);
	loggerFD = open(loggerFifo, O_WRONLY);
    }
    
    write(loggerFD, pBuffer, strlen(pBuffer) + 1);
#endif
}

void CloseLogger()
{
#ifdef LOG_TO_FILE
    if (loggerFile != nullptr)
    {
	fclose(loggerFile);
	loggerFile = nullptr;
    }
#else

    if (loggerFD >= 0)
    {
	close(loggerFD);
    
	/* remove the FIFO */
	unlink(loggerFifo);
    }
#endif
}

static struct termios oldTermSettings;

// kbhit from http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
static void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    newt = oldTermSettings;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
  {
    tcsetattr( STDIN_FILENO, TCSANOW, &oldTermSettings);
  }
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  changemode( 1 ); 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  int result = FD_ISSET(STDIN_FILENO, &rdfs);
  changemode( 0 ); 
  return result;
}

// getch from http://stackoverflow.com/questions/7469139/what-is-equivalent-to-getch-getche-in-linux
int getch()
{
    char buf = 0;
    changemode( 0 ); 
    struct termios oldSettings = {0};
    struct termios newSettings = {0};
    fflush(stdout);
    
    if ( tcgetattr(0, &oldSettings) < 0 )
	{
        perror("tcgetattr()");
    }
    if ( tcgetattr(0, &newSettings) < 0 )
	{
        perror("tcgetattr()");
    }
    newSettings.c_lflag &= ~(ICANON | ECHO);
    newSettings.c_cc[VMIN] = 1;
    newSettings.c_cc[VTIME] = 0;
    if ( tcsetattr(0, TCSANOW, &newSettings) < 0 )
    {
        perror("tcsetattr ICANON");
    }
    if(read(0,&buf,1)<0)
    {
        perror("read()");
    }
    //old.c_lflag |= (ICANON | ECHO);
    if ( tcsetattr(0, TCSADRAIN, &oldSettings) < 0 )
	{
        perror ("tcsetattr ~ICANON");
    }
    changemode( 0 );
    return (int) buf;
}


using namespace std;

static bool InitSystem()
{
	return true;
}
	
int main(int argc, const char* argv[], const char* envp[] )
{
    int nRetCode = 0;
    Shell *pShell = NULL;
    InputStream *pInStream = NULL;

    tcgetattr( STDIN_FILENO, &oldTermSettings);

	// uncomment this getchar if you want to attach gdb before forth initialization has occured
	//getchar();

	if ( !InitSystem() )
	{
		nRetCode = 1;
	}
    else
    {
		nRetCode = 1;
//return 0;
        pShell = new Shell( argc, (const char **) argv, (const char **) envp);
		OutputToLogger("created shell\n");
        //pShell->SetEnvironmentVars( (const char **) envp );
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
			OutputToLogger("running shell\n");
			pShell->GetEngine()->SetTraceFlags(0);
            nRetCode = pShell->Run( pInStream );

        }
        delete pShell;
    }

    CloseLogger();
    
    return nRetCode;
}

