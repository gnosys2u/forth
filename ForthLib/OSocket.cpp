//////////////////////////////////////////////////////////////////////
//
// OSocket.cpp: builtin socket class
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <map>
#if defined(WIN32)
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

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"
#include "ForthPipe.h"

#include "OSocket.h"

namespace OSocket
{
    //////////////////////////////////////////////////////////////////////
    ///
    //                 OSocket
    //

    struct oSocketStruct
    {
        long*       pMethods;
        ulong       refCount;
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
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        MALLOCATE_OBJECT(oSocketStruct, pSocket, pClassVocab);
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
        FREE_OBJECT(pSocket);
        METHOD_RETURN;
    }

    FORTHOP(oSocketOpenMethod)
    {
        GET_THIS(oSocketStruct, pSocket);
        startupSockets();

        pSocket->protocol = SPOP;
        pSocket->type = SPOP;
        pSocket->domain = SPOP;
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

        socklen_t addrLen = SPOP;
        int addr = SPOP;
        int result = bind(pSocket->fd, (const struct sockaddr *)addr, addrLen);
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

        int backlog = SPOP;
        int result = listen(pSocket->fd, backlog);
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
        int result = accept(pSocket->fd, addr, addrLen);
        if (result != -1)
        {
            ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCISocket);
            MALLOCATE_OBJECT(oSocketStruct, pNewSocket, pClassVocab);
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

        socklen_t addrLen = SPOP;
        const struct sockaddr *addr = (const struct sockaddr *)(SPOP);
        int result = connect(pSocket->fd, addr, addrLen);
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

        int result = (int)inet_ntop(family, addr, dst, dstLen);
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

        METHOD_RET("open", oSocketOpenMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("close", oSocketCloseMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("bind", oSocketBindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("listen", oSocketListenMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("accept", oSocketAcceptMethod, RETURNS_OBJECT(kBCISocket)),
        METHOD_RET("connect", oSocketConnectMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("send", oSocketSendMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("sendTo", oSocketSendToMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("recv", oSocketRecvMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("recvFrom", oSocketRecvFromMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("read", oSocketReadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("write", oSocketWriteMethod, RETURNS_NATIVE(kBaseTypeInt)),

        CLASS_OP("inetPToN", inetPToNOp),
        CLASS_OP("inetNToP", inetNToPOp),
        CLASS_OP("htonl", htonlOp),
        CLASS_OP("htons", htonsOp),
        CLASS_OP("ntohl", ntohlOp),
        CLASS_OP("ntohs", ntohsOp),

        MEMBER_VAR("fd", NATIVE_TYPE_TO_CODE(0, kBaseTypeUInt)),
        MEMBER_VAR("domain", NATIVE_TYPE_TO_CODE(0, kBaseTypeUInt)),
        MEMBER_VAR("type", NATIVE_TYPE_TO_CODE(0, kBaseTypeUInt)),
        MEMBER_VAR("protocol", NATIVE_TYPE_TO_CODE(0, kBaseTypeUInt)),

        // following must be last in table
        END_MEMBERS
    };

    void AddClasses(ForthEngine* pEngine)
    {
        pEngine->AddBuiltinClass("Socket", kBCISocket, kBCIObject, oSocketMembers);
    }

} // namespace OSocket

