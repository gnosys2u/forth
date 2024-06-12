#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthMessages.h: Message types sent between the Forth client and server
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


