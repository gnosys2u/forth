#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthMessages.h: Message types sent between the Forth client and server
//
//////////////////////////////////////////////////////////////////////

#define FORTH_SERVER_PORT 1742
//#define FORTH_SERVER_PORT 4
//#define FORTH_SERVER_PORT 27015

enum
{
    // server -> client
    kClientMsgDisplayText = 10000,
    kClientMsgSendLine,
    kClientMsgStartLoad,
    kClientMsgPopStream,    // sent when "loaddone" is executed
    kClientMsgGetChar,
    kClientMsgGoAway,
    kClientMsgFileOpen,
    kClientMsgFileClose,
    kClientMsgFileSetPosition,
    kClientMsgFileRead,
    kClientMsgFileWrite,
    kClientMsgFileGetChar,
    kClientMsgFilePutChar,
    kClientMsgFileCheckEOF,
    kClientMsgFileGetLength,
    kClientMsgFileCheckExists,
    kClientMsgFileGetPosition,
    kClientMsgFileGetString,
    kClientMsgFilePutString,
    kClientMsgRemoveFile,
    kClientMsgDupHandle,
    kClientMsgDupHandle2,
    kClientMsgFileToHandle,
    kClientMsgFileFlush,
    kClientMsgGetTempFilename,
    kClientMsgRenameFile,
    kClientMsgRunSystem,
    kClientMsgSetWorkDir,
    kClientMsgGetWorkDir,
    kClientMsgMakeDir,
    kClientMsgRemoveDir,
    kClientMsgGetStdIn,
    kClientMsgGetStdOut,
    kClientMsgGetStdErr,
    kClientMsgOpenDir,
    kClientMsgReadDir,
    kClientMsgCloseDir,
    kClientMsgRewindDir,
    kClientMsgOpen,
    kClientMsgRead,
    kClientMsgWrite,
    kClientMsgClose,
    kClientMsgStat,
    kClientMsgFstat,

    kClientMsgLimit,

    // client -> server    
    kServerMsgProcessLine = 20000,
    kServerMsgProcessChar,
    kServerMsgPopStream,         // sent when file is empty
    kServerMsgHandleResult,      // used for things which return a handle/pointer (fileOpen, dirOpen, etc.)
    kServerMsgFileOpResult,      // used for things which return an int status
    kServerMsgFileReadResult,
    kServerMsgFileGetStringResult,
    kServerMsgFileStatResult,
    kServerMsgReadDirResult,
    kServerMsgStartLoadResult,      // sent in response to kClientMsgStartLoad
    kServerMsgLimit
};


