#pragma once

#include <MMSystem.h>

//////////////////////////////////////////////////////////////////////
//
// ForthMidiExtension.h: interface for the ForthMidiExtension class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "Forth.h"
#include "ForthEngine.h"
#include "ForthExtension.h"

class ForthThread;

class ForthMidiExtension : public ForthExtension
{
public:
    ForthMidiExtension();
    virtual ~ForthMidiExtension();

    virtual void Initialize( ForthEngine* pEngine );
    virtual void Reset();
    virtual void Shutdown();
    virtual void ForgetOp( uint32_t opNumber );

    static ForthMidiExtension*     GetInstance( void );

    int32_t OpenInput( int32_t deviceId, int32_t cbOp, int32_t cbOpData );
    int32_t CloseInput( int32_t deviceId );
    int32_t StartInput( int32_t deviceId );
    int32_t StopInput( int32_t deviceId );
	void InGetErrorText( int32_t err, char* buff, int32_t buffsize );

    int32_t OpenOutput( int32_t deviceId, int32_t cbOp, int32_t cbOpData );
    int32_t CloseOutput( int32_t deviceId );

    void Enable( bool enable );

    int32_t InputDeviceCount();
    int32_t OutputDeviceCount();

    MIDIOUTCAPS* GetOutputDeviceCapabilities( UINT_PTR deviceId );
    MIDIINCAPS* GetInputDeviceCapabilities( UINT_PTR deviceId );

	int32_t ForthMidiExtension::OutShortMsg( UINT_PTR deviceId, DWORD msg );

	int32_t OutPrepareHeader( int32_t deviceId, MIDIHDR* pHdr );
	int32_t OutUnprepareHeader( int32_t deviceId, MIDIHDR* pHdr );
	int32_t OutLongMsg( int32_t deviceId, MIDIHDR* pHdr );
	void OutGetErrorText( int32_t err, char* buff, int32_t buffsize );

protected:
    static ForthMidiExtension* mpInstance;

    static void CALLBACK MidiInCallback( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
                                DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
    static void CALLBACK MidiOutCallback( HMIDIOUT hMidiOut, UINT wMsg, DWORD_PTR dwInstance,
                                 DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    void HandleMidiIn( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
    void HandleMidiOut( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    class InDeviceInfo
    {
    public:
        InDeviceInfo();

        uint32_t       mCbOp;
        void*       mCbOpData;
        HMIDIIN     mHandle;
        UINT_PTR    mDeviceId;
        MIDIINCAPS  mCaps;
    };

    class OutDeviceInfo
    {
    public:
        OutDeviceInfo();

        uint32_t       mCbOp;
        void*       mCbOpData;
        HMIDIOUT    mHandle;
        UINT_PTR    mDeviceId;
        MIDIOUTCAPS mCaps;
    };

    std::vector<InDeviceInfo>    mInputDevices;
    std::vector<OutDeviceInfo>   mOutputDevices;

    ForthThread*             mpThread;
    ForthFiber*             mpFiber;
    bool                    mbEnabled;
};

