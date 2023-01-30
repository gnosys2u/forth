//////////////////////////////////////////////////////////////////////
//
// ForthMidiExtension.cpp: support for midi devices
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Engine.h"
#include "ForthMidiExtension.h"
#include <vector>

//////////////////////////////////////////////////////////////////////
////
///     midi device support ops
//
//

extern "C"
{

FORTHOP( enableMidiOp )
{
    ForthMidiExtension::GetInstance()->Enable( true );
}

FORTHOP( disableMidiOp )
{
    ForthMidiExtension::GetInstance()->Enable( false );
}

FORTHOP( midiInGetNumDevsOp )
{
    int32_t count = ForthMidiExtension::GetInstance()->InputDeviceCount();
    SPUSH( count );
}

FORTHOP( midiOutGetNumDevsOp )
{
    int32_t count = ForthMidiExtension::GetInstance()->OutputDeviceCount();
    SPUSH( count );
}

FORTHOP( midiInOpenOp )
{
    int32_t cbData = SPOP;
    int32_t cbOp = SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OpenInput( deviceId, cbOp, cbData );
    SPUSH( result );
}

FORTHOP( midiInCloseOp )
{
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->CloseInput( deviceId );
    SPUSH( result );
}

FORTHOP( midiInStartOp )
{
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->StartInput( deviceId );
    SPUSH( result );
}

FORTHOP( midiInStopOp )
{
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->StopInput( deviceId );
    SPUSH( result );
}

FORTHOP( midiInGetErrorTextOp )
{
	int32_t buffSize = SPOP;
	char* buff = (char *) SPOP;
	int32_t err = SPOP;
    ForthMidiExtension::GetInstance()->InGetErrorText( err, buff, buffSize );
}

FORTHOP( midiOutOpenOp )
{
    int32_t cbData = SPOP;
    int32_t cbOp = SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OpenOutput( deviceId, cbOp, cbData );
    SPUSH( result );
}

FORTHOP( midiOutCloseOp )
{
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->CloseOutput( deviceId );
    SPUSH( result );
}

FORTHOP( midiInGetDeviceNameOp )
{
    int32_t deviceNum = SPOP;
    MIDIINCAPS* pCaps = ForthMidiExtension::GetInstance()->GetInputDeviceCapabilities( (UINT_PTR) deviceNum );
    int32_t result = (pCaps == NULL) ? 0 : (int32_t) (&pCaps->szPname);
    SPUSH( result );
}

FORTHOP( midiInGetDeviceCapabilitiesOp )
{
    int32_t deviceNum = SPOP;
    MIDIINCAPS* pCaps = ForthMidiExtension::GetInstance()->GetInputDeviceCapabilities( (UINT_PTR) deviceNum );
    SPUSH( (int32_t) pCaps );
}

FORTHOP( midiOutGetDeviceNameOp )
{
    int32_t deviceNum = SPOP;
    MIDIOUTCAPS* pCaps = ForthMidiExtension::GetInstance()->GetOutputDeviceCapabilities( (UINT_PTR) deviceNum );
    int32_t result = (pCaps == NULL) ? 0 : (int32_t) (&pCaps->szPname);
    SPUSH( result );
}

FORTHOP( midiOutGetDeviceCapabilitiesOp )
{
    int32_t deviceNum = SPOP;
    MIDIOUTCAPS* pCaps = ForthMidiExtension::GetInstance()->GetOutputDeviceCapabilities( (UINT_PTR) deviceNum );
    SPUSH( (int32_t) pCaps );
}

FORTHOP( midiOutShortMsgOp )
{
    int32_t msg = SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OutShortMsg( deviceId, msg );
    SPUSH( result );
}

FORTHOP( midiHdrSizeOp )
{
    SPUSH( sizeof(MIDIHDR) );
}

FORTHOP( midiOutPrepareHeaderOp )
{
    MIDIHDR* pHdr = (MIDIHDR*) SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OutPrepareHeader( deviceId, pHdr );
    SPUSH( result );
}

FORTHOP( midiOutUnprepareHeaderOp )
{
    MIDIHDR* pHdr = (MIDIHDR*) SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OutUnprepareHeader( deviceId, pHdr );
    SPUSH( result );
}

FORTHOP( midiOutLongMsgOp )
{
    MIDIHDR* pHdr = (MIDIHDR*) SPOP;
    int32_t deviceId = SPOP;
    int32_t result = ForthMidiExtension::GetInstance()->OutLongMsg( deviceId, pHdr );
    SPUSH( result );
}

FORTHOP( midiOutGetErrorTextOp )
{
	int32_t buffSize = SPOP;
	char* buff = (char *) SPOP;
	int32_t err = SPOP;
    ForthMidiExtension::GetInstance()->OutGetErrorText( err, buff, buffSize );
}

baseDictionaryEntry midiDictionary[] =
{
    ///////////////////////////////////////////
    //  midi
    ///////////////////////////////////////////
    OP_DEF(    enableMidiOp,                    "enableMidi" ),
    OP_DEF(    disableMidiOp,                   "disableMidi" ),
    OP_DEF(    midiInGetNumDevsOp,              "midiInGetNumDevices" ),
    OP_DEF(    midiOutGetNumDevsOp,             "midiOutGetNumDevices" ),
    OP_DEF(    midiInGetDeviceNameOp,           "midiInGetDeviceName" ),
    OP_DEF(    midiOutGetDeviceNameOp,          "midiOutGetDeviceName" ),
    OP_DEF(    midiInGetDeviceCapabilitiesOp,   "midiInGetDeviceCapabilities" ),
    OP_DEF(    midiOutGetDeviceCapabilitiesOp,  "midiOutGetDeviceCapabilities" ),
    OP_DEF(    midiInOpenOp,                    "midiInOpen" ),
    OP_DEF(    midiInCloseOp,                   "midiInClose" ),
    OP_DEF(    midiInStartOp,                   "midiInStart" ),
    OP_DEF(    midiInStopOp,                    "midiInStop" ),
    OP_DEF(    midiInGetErrorTextOp,            "midiInGetErrorText" ),
    OP_DEF(    midiOutOpenOp,                   "midiOutOpen" ),
    OP_DEF(    midiOutCloseOp,                  "midiOutClose" ),
	OP_DEF(    midiHdrSizeOp,					"MIDIHDR_SIZE" ),
    OP_DEF(    midiOutShortMsgOp,               "midiOutShortMsg" ),
    OP_DEF(    midiOutPrepareHeaderOp,          "midiOutPrepareHeader" ),
    OP_DEF(    midiOutUnprepareHeaderOp,        "midiOutUnprepareHeader" ),
    OP_DEF(    midiOutGetErrorTextOp,           "midiOutGetErrorText" ),
    OP_DEF(    midiOutLongMsgOp,                "midiOutLongMsg" ),
    // following must be last in table
    OP_DEF(    NULL,                        "" )
};

};  // end extern "C"

//////////////////////////////////////////////////////////////////////
////
///     ForthMidiExtension
//
//

ForthMidiExtension *ForthMidiExtension::mpInstance = NULL;

ForthMidiExtension::ForthMidiExtension()
:   mbEnabled( false )
,   mpThread(nullptr)
,   mpFiber(nullptr)
{
    mpInstance = this;
}


ForthMidiExtension::~ForthMidiExtension()
{
	Shutdown();
}


void ForthMidiExtension::Initialize( Engine* pEngine )
{
    if (mpThread != NULL )
    {
        delete mpThread;
    }
    mpThread = Engine::GetInstance()->CreateThread();
    mpThread->SetName("MidiThread");
    mpFiber = mpThread->GetFiber(0);
    mpFiber->SetName("MidiThread");

    Extension::Initialize( pEngine );
    pEngine->AddBuiltinOps( midiDictionary );
}


void ForthMidiExtension::Reset()
{
    mInputDevices.resize( 0 );
    mOutputDevices.resize( 0 );
    mpFiber->Reset();
    mbEnabled = false;
}


void ForthMidiExtension::Shutdown()
{
    mbEnabled = false;
#if 0
	if (mpThread != NULL )
	{
		Engine::GetInstance()->DestroyThread(mpThread);
        mpThread = NULL;
	}
#endif
}


void ForthMidiExtension::ForgetOp( uint32_t opNumber )
{
    for ( uint32_t i = 0; i < mInputDevices.size(); i++ )
    {
        if ( mInputDevices[i].mCbOp >= opNumber )
        {
            mInputDevices[i].mCbOp = 0;
        }
    }
    for ( uint32_t i = 0; i < mOutputDevices.size(); i++ )
    {
        if ( mOutputDevices[i].mCbOp >= opNumber )
        {
            mOutputDevices[i].mCbOp = 0;
        }
    }
}


ForthMidiExtension* ForthMidiExtension::GetInstance( void )
{
    if ( mpInstance == NULL )
    {
        mpInstance = new ForthMidiExtension;
    }
    return mpInstance;
}


int32_t ForthMidiExtension::OpenInput( int32_t deviceId, int32_t cbOp, int32_t cbOpData )
{
    if ( (size_t) deviceId >= mInputDevices.size() )
    {
        mInputDevices.resize( deviceId + 1 );
    }
    InDeviceInfo* midiIn = &(mInputDevices[deviceId]);
    midiIn->mCbOp = cbOp;
    midiIn->mCbOpData = (void *) cbOpData;
    midiIn->mDeviceId = (UINT_PTR) deviceId;
    MMRESULT result = midiInOpen( &(midiIn->mHandle), (UINT_PTR) deviceId,
                                  (DWORD_PTR) MidiInCallback, (DWORD_PTR) deviceId, (CALLBACK_FUNCTION | MIDI_IO_STATUS) );
    return (int32_t) result;
}


void CALLBACK ForthMidiExtension::MidiInCallback( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
                                DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    ForthMidiExtension* pThis = ForthMidiExtension::GetInstance();
    pThis->HandleMidiIn( wMsg, dwInstance, dwParam1, dwParam2 );
}


void ForthMidiExtension::HandleMidiIn( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
#if 0
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
#endif

	if ( mbEnabled && (dwInstance < mInputDevices.size()) )
    {
        InDeviceInfo* midiIn = &(mInputDevices[dwInstance]);
        if ( midiIn->mCbOp != 0 )
        {
			mpFiber->Reset();
			mpFiber->SetOp( midiIn->mCbOp );
			mpFiber->Push( wMsg );
            mpFiber->Push( (int32_t) (midiIn->mCbOpData) );
			mpFiber->Push( dwParam1 );
			mpFiber->Push( dwParam2 );
			mpFiber->Run();
			cell* pStack = mpFiber->GetCore()->SP;
			TRACE( "MIDI:" );
			while ( pStack < mpFiber->GetCore()->ST )
			{
				TRACE( " %x", *pStack );
				pStack++;
			}
			TRACE( "\n" );
        }
        else
        {
            TRACE( "No registered op for channel %d\n", dwInstance );
        }
    }
    else
    {
        // TBD: Error
        int32_t cbOp = -1;
        if ( dwInstance < mInputDevices.size() )
        {
            cbOp = mInputDevices[dwInstance].mCbOp;
        }
        TRACE( "ForthMidiExtension::HandleMidiIn enable %d   nInputDevices %d   cbOp  %x\n",
               mbEnabled, mInputDevices.size(), cbOp );
    }
}


int32_t ForthMidiExtension::CloseInput( int32_t deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( ((size_t) deviceId < mInputDevices.size()) && (mInputDevices[deviceId].mCbOp != 0) )
    {
        result = midiInClose( mInputDevices[deviceId].mHandle );
        mInputDevices[deviceId].mCbOp = 0;
    }
	return (int32_t) result;
}


int32_t ForthMidiExtension::StartInput( int32_t deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( (size_t) deviceId < mInputDevices.size() )
    {
        result = midiInStart( mInputDevices[deviceId].mHandle );
    }
	return (int32_t) result;
}


int32_t ForthMidiExtension::StopInput( int32_t deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( (size_t) deviceId < mInputDevices.size() )
    {
        result = midiInStop( mInputDevices[deviceId].mHandle );
    }
	return (int32_t) result;
}


int32_t ForthMidiExtension::OpenOutput( int32_t deviceId, int32_t cbOp, int32_t cbOpData )
{
    if ( (size_t) deviceId >= mOutputDevices.size() )
    {
        mOutputDevices.resize( deviceId + 1 );
    }
    OutDeviceInfo* midiOut = &(mOutputDevices[deviceId]);
    midiOut->mCbOp = cbOp;
    midiOut->mCbOpData = (void *) cbOpData;
    midiOut->mDeviceId = (UINT_PTR) deviceId;
    MMRESULT result = midiOutOpen( &(midiOut->mHandle), (UINT_PTR) deviceId,
                                   (DWORD_PTR) MidiOutCallback, (DWORD_PTR) deviceId, CALLBACK_FUNCTION );
    return (int32_t) result;
}


int32_t ForthMidiExtension::CloseOutput( int32_t deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( ((size_t) deviceId < mOutputDevices.size()) && (mOutputDevices[deviceId].mCbOp != 0) )
    {
        result = midiOutClose( mOutputDevices[deviceId].mHandle );
        mOutputDevices[deviceId].mCbOp = 0;
    }
	return (int32_t) result;
}

void CALLBACK ForthMidiExtension::MidiOutCallback( HMIDIOUT hMidiOut, UINT wMsg, DWORD_PTR dwInstance,
                                 DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    ForthMidiExtension* pThis = ForthMidiExtension::GetInstance();
    pThis->HandleMidiOut( wMsg, dwInstance, dwParam1, dwParam2 );
}


void ForthMidiExtension::HandleMidiOut( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    if ( mbEnabled && (dwInstance < mOutputDevices.size()) )
    {
        OutDeviceInfo* midiOut = &(mOutputDevices[dwInstance]);
        if ( midiOut->mCbOp != 0 )
        {
			mpFiber->Reset();
			mpFiber->SetOp( midiOut->mCbOp );
			mpFiber->Push( wMsg );
            mpFiber->Push( (int32_t) (midiOut->mCbOpData) );
			mpFiber->Push( dwParam1 );
			mpFiber->Push( dwParam2 );
			mpFiber->Run();
        }
        else
        {
            TRACE( "No registered op for channel %d\n", dwInstance );
        }
    }
}

void ForthMidiExtension::Enable( bool enable )
{
    mbEnabled = enable;
}


int32_t ForthMidiExtension::InputDeviceCount()
{
    return (int32_t) midiInGetNumDevs();
}


int32_t ForthMidiExtension::OutputDeviceCount()
{
    return (int32_t) midiOutGetNumDevs();
}

MIDIOUTCAPS* ForthMidiExtension::GetOutputDeviceCapabilities( UINT_PTR deviceId )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( (size_t) deviceId >= mOutputDevices.size() )
    {
        mOutputDevices.resize( deviceId + 1 );
    }
	result = midiOutGetDevCaps( deviceId, &(mOutputDevices[deviceId].mCaps), sizeof(MIDIOUTCAPS) );
    return (result == MMSYSERR_NOERROR) ? &(mOutputDevices[deviceId].mCaps) : NULL;
}

MIDIINCAPS* ForthMidiExtension::GetInputDeviceCapabilities( UINT_PTR deviceId )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( (size_t) deviceId >= mInputDevices.size() )
    {
        mInputDevices.resize( deviceId + 1 );
    }
    result = midiInGetDevCaps( deviceId, &(mInputDevices[deviceId].mCaps), sizeof(MIDIINCAPS) );
    return (result == MMSYSERR_NOERROR) ? &(mInputDevices[deviceId].mCaps) : NULL;
}

int32_t ForthMidiExtension::OutShortMsg( UINT_PTR deviceId, DWORD msg )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( deviceId < mOutputDevices.size() )
    {
	    result = midiOutShortMsg( mOutputDevices[deviceId].mHandle, msg );
    }
	return (int32_t) result;
}

int32_t ForthMidiExtension::OutPrepareHeader( int32_t deviceId, MIDIHDR* pHdr )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( deviceId < mOutputDevices.size() )
    {
		result = midiOutPrepareHeader( mOutputDevices[deviceId].mHandle, pHdr, sizeof(MIDIHDR) );
    }
	return (int32_t) result;
}

int32_t ForthMidiExtension::OutUnprepareHeader( int32_t deviceId, MIDIHDR* pHdr )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( deviceId < mOutputDevices.size() )
    {
		result = midiOutUnprepareHeader( mOutputDevices[deviceId].mHandle, pHdr, sizeof(MIDIHDR) );
    }
	return (int32_t) result;
}

int32_t ForthMidiExtension::OutLongMsg( int32_t deviceId, MIDIHDR* pHdr )
{
	MMRESULT result = MMSYSERR_BADDEVICEID;
    if ( deviceId < mOutputDevices.size() )
    {
		result = midiOutLongMsg( mOutputDevices[deviceId].mHandle, pHdr, sizeof(MIDIHDR) );
    }
	return (int32_t) result;
}

void ForthMidiExtension::OutGetErrorText( int32_t err, char* buff, int32_t buffSize )
{
	midiOutGetErrorText( err, buff, buffSize );
}

void ForthMidiExtension::InGetErrorText( int32_t err, char* buff, int32_t buffSize )
{
	midiInGetErrorText( err, buff, buffSize );
}


#if 0
void ForthMidiExtension::
{
}


void ForthMidiExtension::
{
}


void ForthMidiExtension::
{
}
#endif


//////////////////////////////////////////////////////////////////////
////
///     InDeviceInfo
//
//

ForthMidiExtension::InDeviceInfo::InDeviceInfo()
:   mCbOp( 0 )
,   mCbOpData( NULL )
,   mHandle( (HMIDIIN) ~0 )
,   mDeviceId( (UINT_PTR) ~0 )
{
}

//////////////////////////////////////////////////////////////////////
////
///     OutDeviceInfo
//
//

ForthMidiExtension::OutDeviceInfo::OutDeviceInfo()
:   mCbOp( 0 )
,   mCbOpData( NULL )
,   mHandle( (HMIDIOUT) ~0 )
,   mDeviceId( (UINT_PTR) ~0 )
{
}



