// ForthMidiMain.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "ForthMidiMain.h"
#include "ForthShell.h"
#include "ForthEngine.h"
#include "ForthInput.h"
#include "ForthMidiExtension.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//#define TEST_MIDI
#ifdef TEST_MIDI

HMIDIIN MidiInHandle = 0;
#define MIDI_IN_DEVICENUM 8

/* ****************************** closeMidiIn() *****************************
 * Close MIDI In Device if it's open.
 ************************************************************************** */

DWORD closeMidiIn(void)
{
   DWORD   err;

   /* Is the device open? */
   if ((err = (DWORD)MidiInHandle))
   {
      /* Unqueue any buffers we added. If you don't
      input System Exclusive, you won't need this */
      midiInReset(MidiInHandle);

      /* Close device */
      if (!(err = midiInClose(MidiInHandle)))
      {
         /* Clear handle so that it's safe to call closeMidiIn() anytime */
         MidiInHandle = 0;
      }
   }

   /* Return the error */
   return(err);
}

void CALLBACK midiInputEvt( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    TRACE( "ForthMidiExtension::HandleMidiIn   msg %x   cbData %x   p1 %x   p2 %x\n",
            wMsg, dwInstance, dwParam1, dwParam2 );
    switch ( wMsg )
    {
    case MIM_DATA:      // Short message received
        TRACE( "Data\n" );
        break;

    case MIM_ERROR:     // Invalid short message received
        TRACE( "Error\n" );
        break;

    case MIM_LONGDATA:  // System exclusive message received
        TRACE( "LongData\n" );
        break;

    case MIM_LONGERROR: // Invalid system exclusive message received
        TRACE( "LongError\n" );
        break;

    case MIM_OPEN:
        TRACE( "Input Open\n" );
        break;

    case MIM_CLOSE:
        TRACE( "Input Close\n" );
        break;

    case MOM_OPEN:
        TRACE( "Output Open\n" );
        break;

    case MOM_CLOSE:
        TRACE( "Output Close\n" );
        break;

    case MOM_DONE:
        TRACE( "Output Done\n" );
        break;
    }
}

/* *************************** openMidiIn() *****************************
 * Opens MIDI In Device #0. Stores handle in MidiInHandle. Starts
 * recording. (midiInputEvt is my callback to process input).
 * Returns 0 if success. Otherwise, an error number.
 * Use midiInGetErrorText to retrieve an error message.
 ************************************************************************ */

DWORD openMidiIn( UINT deviceId )
{
   DWORD   err;

   /* Is it not yet open? */
   if (!MidiInHandle)
   {
      if (!(err = midiInOpen(&MidiInHandle, deviceId, (DWORD)midiInputEvt, 0, CALLBACK_FUNCTION)))
      {
         /* Start recording Midi and return if SUCCESS */
         if (!(err = midiInStart(MidiInHandle)))
         {
            return(0);
         }
      }

      /* ============== ERROR ============== */

      /* Close MIDI In and zero handle */
      closeMidiIn();

      /* Return the error */
      return(err);
   }

   return(0);
}


#endif

// The one and only application object

CWinApp theApp;

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


int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
    ForthShell *pShell = NULL;
    ForthInputStream *pInStream = NULL;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
#ifdef TEST_MIDI
        /* Open MIDI In */
		MIDIINCAPS deviceCaps;
		UINT deviceId = MIDI_IN_DEVICENUM;
		MMRESULT result = midiInGetDevCaps( deviceId, &deviceCaps, sizeof(MIDIINCAPS) );
        if ( openMidiIn( deviceId ) == 0 )
        {

            char buffy[256];
            gets( &(buffy[0]) );

            /* Close any Midi Input device */
            closeMidiIn();
        }
#else
        ForthMidiExtension* pMidiExtension = new ForthMidiExtension;
        pShell = new ForthShell( argc, (const char **)(argv), (const char **)envp,
            nullptr, pMidiExtension);
#if 0
        if ( argc > 1 )
        {

            //
            // interpret the forth file named on the command line
            //
            FILE *pInFile = fopen( argv[1], "r" );
            if ( pInFile != NULL )
            {
                pInStream = new ForthFileInputStream(pInFile);
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
            pInStream = new ForthConsoleInputStream;
            nRetCode = pShell->Run( pInStream );

        }
        delete pShell;
        delete pMidiExtension;
#endif
    }

	return nRetCode;
}
