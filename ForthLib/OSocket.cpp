//////////////////////////////////////////////////////////////////////
//
// OSocket.cpp: builtin socket class
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <map>
#include "Forth.h"
#if defined(WINDOWS_BUILD)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#if defined(MACOSX)
#include <unistd.h>
#endif

#include "Engine.h"
#include "OuterInterpreter.h"
#include "Vocabulary.h"
#include "Object.h"
#include "BuiltinClasses.h"
#include "ShowContext.h"
#include "Pipe.h"

#include "TypesManager.h"
#include "ClassVocabulary.h"

#include "OSocket.h"

namespace OSocket
{
    //////////////////////////////////////////////////////////////////////
    ///
    //                 OSocket
    //

    struct oSocketStruct
    {
        forthop*    pMethods;
        REFCOUNTER  refCount;
#ifdef WIN32
        SOCKET      fd;
#else
        int         fd;
#endif
        int         domain;
        int         type;
        int         protocol;
    };


    FORTHOP(oSocketNew)
    {
        ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
        ALLOCATE_OBJECT(oSocketStruct, pSocket, pClassVocab);
        pSocket->pMethods = pClassVocab->GetMethods();
        pSocket->refCount = 0;
        pSocket->fd = -1;
        pSocket->domain = 0;
        pSocket->type = 0;
        pSocket->protocol = 0;
        PUSH_OBJECT(pSocket);
    }

    FORTHOP(oSocketDeleteMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        if (pSocket->fd != -1)
        {
#if defined(WIN32)
            closesocket(pSocket->fd);
#else
            close(pSocket->fd);
#endif
        }
        METHOD_RETURN;
    }

    FORTHOP(oSocketOpenMethod)
    {
        GET_THIS(oSocketStruct, pSocket);
        startupSockets();

        pSocket->protocol = (int)SPOP;
        pSocket->type = (int)SPOP;
        pSocket->domain = (int)SPOP;
        pSocket->fd = socket(pSocket->domain, pSocket->type, pSocket->protocol);
#ifdef WIN32
        if (pSocket->fd == -1)
        {
            printf("Error at socket(): %d\n", WSAGetLastError());
        }
#endif
        SPUSH(pSocket->fd);
        METHOD_RETURN;
    }

    FORTHOP(oSocketCloseMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int result;
#if defined(WIN32)
        result = closesocket(pSocket->fd);
#else
        result = close(pSocket->fd);
#endif
        pSocket->fd = -1;
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketBindMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        socklen_t addrLen = (socklen_t)SPOP;
        cell addr = SPOP;
        cell result = (cell)bind(pSocket->fd, (const struct sockaddr *)addr, addrLen);
#ifdef WIN32
        if (result < 0)
        {
            printf("Error at socket(): %d\n", WSAGetLastError());
        }
#endif
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketListenMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int backlog = (int)SPOP;
        cell result = (cell)listen(pSocket->fd, backlog);
#ifdef WIN32
        if (result < 0)
        {
            printf("Error at socket(): %d\n", WSAGetLastError());
        }
#endif
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketAcceptMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        //int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags);
        socklen_t *addrLen = (socklen_t *)(SPOP);
        struct sockaddr *addr = (struct sockaddr *)(SPOP);
        cell result = (cell)accept(pSocket->fd, addr, addrLen);
        if (result != -1)
        {
            ClassVocabulary *pClassVocab = TypesManager::GetInstance()->GetClassVocabulary(kBCISocket);
            ALLOCATE_OBJECT(oSocketStruct, pNewSocket, pClassVocab);
            pNewSocket->pMethods = pClassVocab->GetMethods();
            pNewSocket->refCount = 0;
            pNewSocket->fd = result;
            pNewSocket->domain = pSocket->domain;
            pNewSocket->type = pSocket->type;
            pNewSocket->protocol = pSocket->protocol;
            PUSH_OBJECT(pNewSocket);
        }
        else
        {
            PUSH_OBJECT(nullptr);
        }
        METHOD_RETURN;
    }

    FORTHOP(oSocketConnectMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        socklen_t addrLen = (socklen_t)SPOP;
        const struct sockaddr *addr = (const struct sockaddr *)(SPOP);
        cell result = (cell)connect(pSocket->fd, addr, addrLen);
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketSendMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int flags = SPOP;
        size_t len = SPOP;
        const char *buf = (const char *)(SPOP);
        int result = send(pSocket->fd, buf, len, flags);
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketSendToMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int addrLen = SPOP;
        const struct sockaddr *destAddr = (const struct sockaddr *)(SPOP);
        int flags = SPOP;
        size_t len = SPOP;
        const char *buf = (const char *)(SPOP);
        int result = sendto(pSocket->fd, buf, len, flags, destAddr, addrLen);
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketRecvMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int flags = SPOP;
        size_t len = SPOP;
        char *buf = (char *)(SPOP);
        int result = recv(pSocket->fd, buf, len, flags);
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketRecvFromMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        socklen_t *pAddrLen = (socklen_t *)(SPOP);
        struct sockaddr *destAddr = (struct sockaddr *)(SPOP);
        int flags = SPOP;
        size_t len = SPOP;
        char *buf = (char *)(SPOP);
        //int socketFD = SPOP;  huh? what was this supposed to be?
        int result = recvfrom(pSocket->fd, buf, len, flags, destAddr, pAddrLen);
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketReadMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int numBytes = SPOP;
        char* buffer = (char *)(SPOP);

#ifdef WIN32
        int result = recv(pSocket->fd, buffer, numBytes, 0);
#else
        int result = read(pSocket->fd, buffer, numBytes);
#endif
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(oSocketWriteMethod)
    {
        GET_THIS(oSocketStruct, pSocket);

        int numBytes = SPOP;
        char* buffer = (char *)(SPOP);

#ifdef WIN32
        int result = send(pSocket->fd, buffer, numBytes, 0);
#else
        int result = write(pSocket->fd, buffer, numBytes);
#endif
        SPUSH(result);
        METHOD_RETURN;
    }

    FORTHOP(inetPToNOp)
    {
        NEEDS(3);
        int* dst = (int *)(SPOP);
        char* addrStr = (char *)(SPOP);
        int family = SPOP;

        int result = inet_pton(family, addrStr, dst);
        SPUSH(result);
    }

    FORTHOP(inetNToPOp)
    {
        NEEDS(4);
        int dstLen = SPOP;
        char* dst = (char *)(SPOP);
        const void* addr = (const void *)(SPOP);
        int family = SPOP;

        cell result = (cell)inet_ntop(family, addr, dst, dstLen);
        SPUSH(result);
    }

    FORTHOP(htonsOp)
    {
        NEEDS(1);
        int shortVal = SPOP;

        int result = htons(shortVal);
        SPUSH(result);
    }

    FORTHOP(htonlOp)
    {
        NEEDS(1);
        int val = SPOP;

        int result = htonl(val);
        SPUSH(result);
    }

    FORTHOP(ntohsOp)
    {
        NEEDS(1);
        int shorty = SPOP;

        int result = ntohs(shorty);
        SPUSH(result);
    }

    FORTHOP(ntohlOp)
    {
        NEEDS(1);
        int shorty = SPOP;

        int result = ntohl(shorty);
        SPUSH(result);
    }

    baseMethodEntry oSocketMembers[] =
    {
        METHOD("__newOp", oSocketNew),
        METHOD("delete", oSocketDeleteMethod),

        METHOD_RET("open", oSocketOpenMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("close", oSocketCloseMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("bind", oSocketBindMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("listen", oSocketListenMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("accept", oSocketAcceptMethod, RETURNS_OBJECT(kBCISocket)),
        METHOD_RET("connect", oSocketConnectMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("send", oSocketSendMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("sendTo", oSocketSendToMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("recv", oSocketRecvMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("recvFrom", oSocketRecvFromMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("read", oSocketReadMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD_RET("write", oSocketWriteMethod, RETURNS_NATIVE(BaseType::kInt)),

        CLASS_OP("inetPToN", inetPToNOp),
        CLASS_OP("inetNToP", inetNToPOp),
        CLASS_OP("htonl", htonlOp),
        CLASS_OP("htons", htonsOp),
        CLASS_OP("ntohl", ntohlOp),
        CLASS_OP("ntohs", ntohsOp),

        MEMBER_VAR("fd", NATIVE_TYPE_TO_CODE(0, BaseType::kUInt)),
        MEMBER_VAR("domain", NATIVE_TYPE_TO_CODE(0, BaseType::kUInt)),
        MEMBER_VAR("type", NATIVE_TYPE_TO_CODE(0, BaseType::kUInt)),
        MEMBER_VAR("protocol", NATIVE_TYPE_TO_CODE(0, BaseType::kUInt)),

        // following must be last in table
        END_MEMBERS
    };

    void AddClasses(OuterInterpreter* pOuter)
    {
        pOuter->AddBuiltinClass("Socket", kBCISocket, kBCIObject, oSocketMembers);
    }

} // namespace OSocket

