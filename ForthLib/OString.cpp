//////////////////////////////////////////////////////////////////////
//
// OString.cpp: builtin string related classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"
#include "ForthObjectReader.h"

#include "OString.h"
#include "OArray.h"

extern "C"
{
	unsigned long SuperFastHash (const char * data, int len, unsigned long hash);
	extern int oStringFormatSub( ForthCoreState* pCore, char* pBuffer, int bufferSize );
};

namespace OString
{

	//////////////////////////////////////////////////////////////////////
    ///
    //                 String
    //

	int gDefaultOStringSize = DEFAULT_STRING_DATA_BYTES - 1;
    ForthClassVocabulary* gpStringClassVocab = nullptr;
    ForthClassVocabulary* gpStringMapClassVocab = nullptr;

// temp hackaround for a heap corruption when expanding a string
//#define RCSTRING_SLOP 16
#define RCSTRING_SLOP 0
	oString* createOString( int maxChars )
	{
		int dataBytes = ((maxChars  + 4) & ~3);
        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
		oString* str = (oString *) __MALLOC(nBytes + RCSTRING_SLOP);
		str->maxLen = dataBytes - 1;
		str->curLen = 0;
		str->data[0] = '\0';
		return str;
	}

	oString* resizeOString(oStringStruct* pString, int newLen)
    {
        int dataBytes = ((newLen + 4) & ~3);
        size_t nBytes = sizeof(oString) + (dataBytes - DEFAULT_STRING_DATA_BYTES);
		oString* dstString = (oString *)__REALLOC(pString->str, nBytes);
		pString->str = dstString;
		dstString->maxLen = dataBytes - 1;
        dstString->data[newLen] = '\0';
		pString->hash = 0;
        return dstString;
    }

    void appendOString(oStringStruct* pString, const char* pSrc, int numNewBytes)
    {
        oString* dst = pString->str;
        long newLen = dst->curLen + numNewBytes;
        if (newLen > dst->maxLen)
        {
            // enlarge string
			dst = resizeOString(pString, newLen);
        }
        memmove(&(dst->data[dst->curLen]), pSrc, numNewBytes);
        dst->data[newLen] = '\0';
        dst->curLen = newLen;
        pString->hash = 0;
    }

    void prependOString(oStringStruct* pString, const char* pSrc, int numNewBytes)
    {
        oString* dst = pString->str;
        long newLen = dst->curLen + numNewBytes;
        if (newLen > dst->maxLen)
        {
            // enlarge string
			dst = resizeOString(pString, newLen);
        }
        char* pDst = &(dst->data[0]);
        memmove((void *)(pDst + numNewBytes), pDst, dst->curLen);
        memmove(pDst, pSrc, numNewBytes);
        dst->data[newLen] = '\0';
        dst->curLen = newLen;
        pString->hash = 0;
    }


    FORTHOP( oStringNew )
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *) (SPOP);
        MALLOCATE_OBJECT( oStringStruct, pString, pClassVocab );
        pString->pMethods = pClassVocab->GetMethods();
        pString->refCount = 0;
        pString->hash = 0;
		pString->str = createOString( gDefaultOStringSize );
        PUSH_OBJECT(pString);
    }

    FORTHOP( oStringDeleteMethod )
    {
        // go through all elements and release any which are not null
        GET_THIS( oStringStruct, pString );
		free( pString->str );
        FREE_OBJECT( pString );
        METHOD_RETURN;
    }

    FORTHOP( oStringShowInnerMethod )
    {
		char buffer[16];
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();

		pShowContext->BeginElement("value");
		GET_THIS(oStringStruct, pString);
        pShowContext->ShowQuotedText(&(pString->str->data[0]));
        pShowContext->EndElement();

        pShowContext->BeginElement("curLen");
		sprintf(buffer, "%d", pString->str->curLen);
        pShowContext->EndElement(buffer);

        pShowContext->BeginElement("maxLen");
		sprintf(buffer, "%d", pString->str->maxLen);
        pShowContext->EndElement(buffer);

        pShowContext->BeginElement("hashVal");
        sprintf(buffer, "%d", pString->hash);
        pShowContext->EndElement(buffer);

		METHOD_RETURN;
    }

    FORTHOP( oStringCompareMethod )
    {
        GET_THIS( oStringStruct, pString );
        ForthObject compObj;
        POP_OBJECT( compObj );
		oStringStruct* pComp = (oStringStruct *) compObj;
		int retVal = strcmp( &(pString->str->data[0]), &(pComp->str->data[0]) );
		SPUSH( retVal );
        METHOD_RETURN;
    }

    FORTHOP( oStringSizeMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( pString->str->maxLen );
        METHOD_RETURN;
    }

    FORTHOP( oStringLengthMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( pString->str->curLen );
        METHOD_RETURN;
    }

    FORTHOP( oStringGetMethod )
    {
        GET_THIS( oStringStruct, pString );
		SPUSH( (long) &(pString->str->data[0]) );
        METHOD_RETURN;
    }

    FORTHOP(oStringGet4CMethod)
    {
        GET_THIS(oStringStruct, pString);
        int val = 0;
        int len = pString->str->curLen;
        if (len > 4)
        {
            len = 4;
        }
        char* pSrc = &(pString->str->data[0]);
        memcpy(&val, pSrc, len);
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oStringGet8CMethod)
    {
        GET_THIS(oStringStruct, pString);
        stackInt64 val;
        val.s64 = 0L;
        int len = pString->str->curLen;
        if (len > 8)
        {
            len = 8;
        }
        char* pSrc = &(pString->str->data[0]);
        memcpy(&val, pSrc, len);
        LPUSH(val);
        METHOD_RETURN;
    }

    void setString(oStringStruct* pString, const char* srcStr)
    {
        long len = 0;
        if (srcStr != NULL)
        {
            len = (long)strlen(srcStr);
        }
        else
        {
            // treat null input as an empty string
            srcStr = "";
        }
        oString* dst = pString->str;
        if (len > dst->maxLen)
        {
            // enlarge string
            free(dst);
            dst = createOString(len);
            pString->str = dst;
        }
        dst->curLen = len;
        memmove(&(dst->data[0]), srcStr, len + 1);
        pString->hash = 0;
    }

    FORTHOP( oStringSetMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
        setString(pString, srcStr);
        METHOD_RETURN;
    }

    FORTHOP(oStringSet4CMethod)
    {
        GET_THIS(oStringStruct, pString);
        int buff[2];
        buff[1] = 0;
        buff[0] = SPOP;
        const char* pChars = (const char*)&buff[0];
        setString(pString, pChars);
        METHOD_RETURN;
    }

    FORTHOP(oStringSet8CMethod)
    {
        GET_THIS(oStringStruct, pString);
        stackInt64 buff[2];
        buff[1].s64 = 0L;
        LPOP(buff[0]);
        const char* pChars = (const char*)&buff[0];
        setString(pString, pChars);
        METHOD_RETURN;
    }

    FORTHOP(oStringCopyMethod)
	{
		GET_THIS(oStringStruct, pString);
		ForthObject srcObj;
		POP_OBJECT(srcObj);
		long srcLen = 0;
		oStringStruct* srcStr = nullptr;
		if (srcObj != nullptr)
		{
			srcStr = (oStringStruct *)srcObj;
			srcLen = srcStr->str->curLen;
		}
		
		oString* dst = pString->str;
		if (srcLen == 0)
		{
			dst->data[0] = '\0';
		}
		else
		{
			if (srcLen > dst->maxLen)
			{
				// enlarge string
				free(dst);
				dst = createOString(srcLen);
				pString->str = dst;
			}
			memmove(&(dst->data[0]), &(srcStr->str->data[0]), srcLen + 1);
		}
		dst->curLen = srcLen;
		pString->hash = 0;
		METHOD_RETURN;
	}

	FORTHOP(oStringAppendMethod)
    {
        GET_THIS(oStringStruct, pString);
        const char* srcStr = (const char *)SPOP;
        int len = strlen(srcStr);
        appendOString(pString, srcStr, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringPrependMethod)
    {
        GET_THIS(oStringStruct, pString);
        const char* srcStr = (const char *)SPOP;
        int len = strlen(srcStr);
        prependOString(pString, srcStr, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringGetBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        SPUSH((long)&(pString->str->data[0]));
        SPUSH(pString->str->curLen);
        METHOD_RETURN;
    }

    FORTHOP(oStringSetBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        int len = SPOP;
        const char* srcStr = (const char *)SPOP;
        oString* dst = pString->str;
        if (len > dst->maxLen)
        {
            // enlarge string
            free(dst);
            dst = createOString(len);
            pString->str = dst;
        }
        dst->curLen = len;
        memmove(&(dst->data[0]), srcStr, len);
        dst->data[len] = '\0';
        pString->hash = 0;
        METHOD_RETURN;
    }

    FORTHOP(oStringAppendBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        int len = SPOP;
        const char* srcStr = (const char *)SPOP;
        appendOString(pString, srcStr, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringPrependBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        int len = SPOP;
        const char* srcStr = (const char *)SPOP;
        prependOString(pString, srcStr, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringResizeMethod)
    {
        GET_THIS( oStringStruct, pString );
		long newLen = SPOP;
		oString* dst = resizeOString(pString, newLen);
		dst->data[newLen] = '\0';
        pString->hash = 0;
        dst->curLen = strlen(&(dst->data[0]));
        METHOD_RETURN;
    }

    FORTHOP(oStringKeepLeftMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        if (newLen < 0)
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError(kForthErrorBadParameter, " String.keepLeft negative length");
            newLen = 0;
        }
        else if (newLen > str->curLen)
        {
            newLen = str->curLen;
        }
        str->curLen = newLen;
        str->data[newLen] = '\0';
        METHOD_RETURN;
    }

    FORTHOP(oStringKeepRightMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        char* data = &(str->data[0]);
        if (newLen < 0)
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError(kForthErrorBadParameter, " String.keepRight negative length");
            newLen = 0;
        }
        else if (newLen < str->curLen)
        {
            memmove(data, data + (str->curLen - newLen), newLen);
        }
        else
        {
            // trying to keep more bytes than are in string, do nothing
            newLen = str->curLen;
        }
        str->curLen = newLen;
        str->data[newLen] = '\0';
        METHOD_RETURN;
    }

    FORTHOP(oStringKeepMiddleMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        long firstChar = SPOP;
        ForthEngine *pEngine = ForthEngine::GetInstance();
        if (firstChar < 0)
        {
            pEngine->SetError(kForthErrorBadParameter, " String.keepMiddle negative first character");
        }
        else if (newLen < 0)
        {
            pEngine->SetError(kForthErrorBadParameter, " String.keepMiddle negative length");
        }
        else
        {
            if (firstChar >= str->curLen)
            {
                // starting char is beyond end of string, string is empty
                str->data[0] = '\0';
                str->curLen = 0;
            }
            else
            {
                long charsLeft = str->curLen - firstChar;
                if (newLen > charsLeft)
                {
                    newLen = charsLeft;
                }
                if (newLen > 0)
                {

                    char* data = &(str->data[0]);
                    memmove(data, data + firstChar, newLen);
                }
                else
                {
                    newLen = 0;
                }
                str->curLen = newLen;
                str->data[newLen] = '\0';
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringLeftBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        if (newLen < 0)
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError(kForthErrorBadParameter, " String.leftBytes negative length");
            newLen = 0;
        }
        else if (newLen > str->curLen)
        {
            newLen = str->curLen;
        }
        SPUSH((long)(&(str->data[0])));
        SPUSH(newLen);
        METHOD_RETURN;
    }

    FORTHOP(oStringRightBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        char* data = &(str->data[0]);
        if (newLen < 0)
        {
            ForthEngine *pEngine = ForthEngine::GetInstance();
            pEngine->SetError(kForthErrorBadParameter, " String.rightBytes negative length");
            newLen = 0;
        }
        else if (newLen >= str->curLen)
        {
            newLen = str->curLen;
        }
        else
        {
            data += (str->curLen - newLen);
        }
        SPUSH((long)data);
        SPUSH(newLen);
        METHOD_RETURN;
    }

    FORTHOP(oStringMiddleBytesMethod)
    {
        GET_THIS(oStringStruct, pString);
        oString* str = pString->str;
        long newLen = SPOP;
        long firstChar = SPOP;
        ForthEngine *pEngine = ForthEngine::GetInstance();
        char* pBytes = &(str->data[0]);
        if (firstChar < 0)
        {
            newLen = 0;
            pEngine->SetError(kForthErrorBadParameter, " String.middleBytes negative first character");
        }
        else if (newLen < 0)
        {
            newLen = 0;
            pEngine->SetError(kForthErrorBadParameter, " String.middleBytes negative length");
        }
        else
        {
            if (firstChar >= str->curLen)
            {
                // starting char is beyond end of string, string is empty
                newLen = 0;
                pBytes += str->curLen;
            }
            else
            {
                // starting char is within string
                pBytes += firstChar;
                long charsLeft = str->curLen - firstChar;
                if (newLen > charsLeft)
                {
                    newLen = charsLeft;
                }
            }
        }

        SPUSH((long)(pBytes));
        SPUSH(newLen);
        METHOD_RETURN;
    }

	FORTHOP(oStringEqualsMethod)
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( srcStr != NULL )
		{
			long len = (long) strlen( srcStr );
			if ( (len == pString->str->curLen)
				&& (strncmp( pString->str->data, srcStr, len ) == 0 ) )
			{
				result = ~0;
			}
		}
		SPUSH( result );
        METHOD_RETURN;
    }

	FORTHOP(oStringStartsWithMethod)
	{
		GET_THIS(oStringStruct, pString);
		const char* srcStr = (const char *)SPOP;
		long result = 0;
		if (srcStr != NULL)
		{
			long len = (long)strlen(srcStr);
			if ((len <= pString->str->curLen)
				&& (strncmp(pString->str->data, srcStr, len) == 0))
			{
				result = ~0;
			}
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oStringEndsWithMethod)
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( srcStr != NULL )
		{
			long len = (long) strlen( srcStr );
			if ( len <= pString->str->curLen )
			{
				const char* strEnd = pString->str->data + (pString->str->curLen - len);
				if ( strcmp( strEnd, srcStr ) == 0 )
				{
					result = ~0;
				}
			}
		}
		SPUSH( result );
        METHOD_RETURN;
    }

    FORTHOP( oStringContainsMethod )
    {
        GET_THIS( oStringStruct, pString );
		const char* srcStr = (const char *) SPOP;
		long result = 0;
		if ( (srcStr != NULL)
			&& ( (long) strlen( srcStr ) <= pString->str->curLen )
			&& (strstr( pString->str->data, srcStr ) != NULL ) )
		{
			result = ~0;
		}
		SPUSH( result );
        METHOD_RETURN;
    }

    FORTHOP( oStringClearMethod )
    {
        GET_THIS( oStringStruct, pString );
		pString->hash = 0;
		oString* dst = pString->str;
		dst->curLen = 0;
		dst->data[0] = '\0';
        METHOD_RETURN;
    }

    FORTHOP( oStringHashMethod )
    {
        GET_THIS( oStringStruct, pString );
		pString->hash = 0;
		oString* dst = pString->str;
		if ( dst->curLen != 0 )
		{
			pString->hash = SuperFastHash( &(dst->data[0]), dst->curLen, 0 );
		}
		SPUSH( (long)(pString->hash) );
        METHOD_RETURN;
    }

    FORTHOP( oStringAppendCharMethod )
    {
        GET_THIS( oStringStruct, pString );
		char c = (char) SPOP;
		appendOString( pString, &c, 1 );
        METHOD_RETURN;
    }

    FORTHOP(oStringAppend4CMethod)
    {
        GET_THIS(oStringStruct, pString);
        int buff[2];
        buff[1] = 0;
        buff[0] = SPOP;
        const char* pChars = (const char*)&buff[0];
        int len = strlen(pChars);
        appendOString(pString, pChars, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringAppend8CMethod)
    {
        GET_THIS(oStringStruct, pString);
        stackInt64 buff[2];
        buff[1].s64 = 0L;
        LPOP(buff[0]);
        const char* pChars = (const char*)&buff[0];
        int len = strlen(pChars);
        appendOString(pString, pChars, len);
        METHOD_RETURN;
    }

    FORTHOP(oStringLoadMethod)
	{
		GET_THIS(oStringStruct, pString);
		int numStrings = SPOP;
		const char** pStrings = (const char**)(GET_SP);
		for (int i = numStrings - 1; i >= 0; i--)
		{
			const char* srcStr = pStrings[i];
			int len = strlen(srcStr);
			appendOString(pString, srcStr, len);
		}
        pCore->SP += numStrings;
		METHOD_RETURN;
	}

	FORTHOP(oStringSplitMethod)
	{
		GET_THIS(oStringStruct, pString);
		
        int delimiter = SPOP;

        ForthObject dstArrayObj;
		POP_OBJECT(dstArrayObj);
		oArrayStruct* pArray = (oArrayStruct *)(dstArrayObj);

		if ((pArray != nullptr) && (pString->str->curLen != 0))
		{
			ForthObject obj;

			const char* pSrc = pString->str->data;
			int substringSize;
			bool notDone = true;

			ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIString);
			while (notDone)
			{
				substringSize = 0;
				const char* pEnd = strchr(pSrc, delimiter);
				if (pEnd != NULL)
				{
					substringSize = pEnd - pSrc;
				}
				else
				{
					substringSize = strlen(pSrc);
					notDone = false;
				}
				oString* str = createOString(substringSize);
				MALLOCATE_OBJECT(oStringStruct, pSubString, pClassVocab);
                pSubString->pMethods = pClassVocab->GetMethods();
				pSubString->refCount = 1;
				pSubString->hash = 0;
				pSubString->str = str;
				memcpy(str->data, pSrc, substringSize);
				str->data[substringSize] = '\0';
				str->curLen = substringSize;
				obj = (ForthObject) pSubString;
				pArray->elements->push_back(obj);
				pSrc = pEnd + 1;
			}
		}

		METHOD_RETURN;
	}

	FORTHOP(oStringJoinMethod)
	{
		GET_THIS(oStringStruct, pString);
		const char* delimStr = (const char*)(SPOP);
		int delimLen = (delimStr == nullptr) ? 0 : strlen(delimStr);

		ForthObject obj;
		
		POP_OBJECT(obj);
		
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(obj);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		bool firstTime = true;
        pString->str->curLen = 0;
        pString->str->data[0] = '\0';
        for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			oStringStruct* pStr = (oStringStruct *)o;

			if (!firstTime && (delimLen != 0))
			{
				appendOString(pString, delimStr, delimLen);
			}

			appendOString(pString, pStr->str->data, pStr->str->curLen);
			firstTime = false;
		}

		METHOD_RETURN;
	}

	FORTHOP(oStringAppendFormattedMethod)
	{
		// TOS: N argN ... arg1 formatStr     (arg1 to argN are optional)
        GET_THIS(oStringStruct, pString);
        oString* pOStr = pString->str;
        bool tryAgain = true;
        int maxLen = pOStr->maxLen;
		int curLen = pOStr->curLen;
        long* oldSP = pCore->SP;
        while (tryAgain)
        {
			int roomLeft = maxLen - curLen;
			int numChars = oStringFormatSub(pCore, &(pOStr->data[curLen]), roomLeft + 1);
			if ((numChars >= 0) && (numChars <= roomLeft))
            {
                tryAgain = false;
				curLen += numChars;
            }
            else
            {
                if (maxLen < OSTRING_PRINTF_FIRST_OVERFLOW_SIZE)
                {
                    maxLen = OSTRING_PRINTF_FIRST_OVERFLOW_SIZE;
                }
                else
                {
                    maxLen <<= 1;
                    if (maxLen >= OSTRING_PRINTF_LAST_OVERFLOW_SIZE)
                    {
                        tryAgain = false;
                    }
                }
				pOStr = resizeOString(pString, maxLen);
                pCore->SP = oldSP;
            }
        }

		pOStr->curLen = curLen;
		pString->hash = 0;
        METHOD_RETURN;
    }
    
	FORTHOP(oStringFormatMethod)
	{
		GET_THIS(oStringStruct, pString);
		pString->str->curLen = 0;
		pString->str->data[0] = '\0';
		oStringAppendFormattedMethod(pCore);
	}

	FORTHOP(oStringFixupMethod)
	{
		GET_THIS(oStringStruct, pString);
		pString->hash = 0;
		oString* dst = pString->str;
		dst->curLen = strlen(&(dst->data[0]));
		METHOD_RETURN;
	}

	FORTHOP(oStringToUpperMethod)
	{
		GET_THIS(oStringStruct, pString);
		pString->hash = 0;
		oString* dst = pString->str;
		char* pDst = &(dst->data[0]);
		for (int i = dst->curLen; i >0; --i)
		{
			*pDst = toupper(*pDst);
			pDst++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringToLowerMethod)
	{
		GET_THIS(oStringStruct, pString);
		pString->hash = 0;
		oString* dst = pString->str;
		char* pDst = &(dst->data[0]);
		for (int i = dst->curLen; i >0; --i)
		{
			*pDst = tolower(*pDst);
			pDst++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringReplaceCharMethod)
	{
		GET_THIS(oStringStruct, pString);
		char newChar = (char)(SPOP);
		char oldChar = (char)(SPOP);
		pString->hash = 0;
		oString* dst = pString->str;
		char* pDst = &(dst->data[0]);
		for (int i = dst->curLen; i >0; --i)
		{
			if (*pDst == oldChar)
			{
				*pDst = newChar;
			}
			pDst++;
		}
		METHOD_RETURN;
	}


    baseMethodEntry oStringMembers[] =
    {
        METHOD(     "__newOp",              oStringNew ),
        METHOD(     "delete",               oStringDeleteMethod ),
        METHOD(     "showInner",			oStringShowInnerMethod ),
        METHOD_RET( "compare",              oStringCompareMethod, RETURNS_NATIVE(kBaseTypeInt) ),

        METHOD(     "size",                 oStringSizeMethod ),
        METHOD(     "length",               oStringLengthMethod ),
        METHOD(     "get",                  oStringGetMethod ),
        METHOD(     "get4c",                oStringGet4CMethod ),
        METHOD(     "get8c",                oStringGet8CMethod ),
        METHOD(     "set",                  oStringSetMethod ),
        METHOD(     "set4c",                oStringSet4CMethod ),
        METHOD(     "set8c",                oStringSet8CMethod ),
        METHOD(     "copy",                 oStringCopyMethod ),
        METHOD(     "append",               oStringAppendMethod ),
        METHOD(     "prepend",              oStringPrependMethod ),
        METHOD(     "getBytes",             oStringGetBytesMethod ),
        METHOD(     "setBytes",             oStringSetBytesMethod ),
        METHOD(     "appendBytes",          oStringAppendBytesMethod ),
        METHOD(     "prependBytes",         oStringPrependBytesMethod ),
        METHOD(     "resize",               oStringResizeMethod ),
        METHOD(     "keepLeft",             oStringKeepLeftMethod ),
        METHOD(     "keepRight",            oStringKeepRightMethod ),
        METHOD(     "keepMiddle",           oStringKeepMiddleMethod ),
        METHOD(     "leftBytes",            oStringLeftBytesMethod ),
        METHOD(     "rightBytes",           oStringRightBytesMethod ),
        METHOD(     "middleBytes",          oStringMiddleBytesMethod ),
        METHOD(     "equals",				oStringEqualsMethod ),
        METHOD(     "startsWith",           oStringStartsWithMethod ),
        METHOD(     "endsWith",             oStringEndsWithMethod ),
        METHOD(     "contains",             oStringContainsMethod ),
        METHOD(     "clear",                oStringClearMethod ),
        METHOD(     "hash",                 oStringHashMethod ),
        METHOD(     "appendChar",           oStringAppendCharMethod ),
        METHOD(     "append4c",             oStringAppend4CMethod ),
        METHOD(     "append8c",             oStringAppend8CMethod ),
        METHOD(     "load",                 oStringLoadMethod ),
        METHOD(		"split",                oStringSplitMethod ),
        METHOD(		"join",					oStringJoinMethod ),
        METHOD(		"format",				oStringFormatMethod ),
        METHOD(     "appendFormatted",      oStringAppendFormattedMethod ),
        METHOD(		"fixup",				oStringFixupMethod ),
        METHOD(		"toLower",				oStringToLowerMethod ),
        METHOD(		"toUpper",				oStringToUpperMethod ),
		METHOD(     "replaceChar",          oStringReplaceCharMethod),
		
        MEMBER_VAR( "__hash",				NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
        MEMBER_VAR( "__str",				NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),

        // following must be last in table
        END_MEMBERS
    };

	//////////////////////////////////////////////////////////////////////
	///
	//                 StringMap
	//


    void setStringMap(oStringMapStruct* pMap, std::string& key, ForthObject& obj, ForthCoreState* pCore)
    {
        oStringMap& a = *(pMap->elements);
        oStringMap::iterator iter = a.find(key);
        if (obj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, obj))
                {
                    SAFE_KEEP(obj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(obj);
            }
            a[key] = obj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                ForthObject& oldObj = iter->second;
                SAFE_RELEASE(pCore, oldObj);
                a.erase(iter);
            }
        }

    }

    oStringMapStruct* createStringMapObject(ForthClassVocabulary *pClassVocab)
    {

        MALLOCATE_OBJECT(oStringMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
        pMap->elements = new oStringMap;
        return pMap;
    }

    FORTHOP(oStringMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject newMap = (ForthObject) createStringMapObject(pClassVocab);
        PUSH_OBJECT(newMap);
	}

	FORTHOP(oStringMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapShowInnerMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
		{
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
                pShowContext->BeginElement(iter->first.c_str());
				ForthShowObject(iter->second, pCore);
                pShowContext->EndElement();
            }
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
	}


    FORTHOP(oStringMapHeadIterMethod)
    {
        GET_THIS(oStringMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oStringMapIterStruct* pIter = new oStringMapIterStruct;
        TRACK_ITER_NEW;
        ForthClassVocabulary *pClassVocab = GET_CLASS_VOCABULARY(kBCIStringMapIter);
        pIter->pMethods = pClassVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oStringMap::iterator;
        *(pIter->cursor) = pMap->elements->begin();
        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringMapTailIterMethod)
    {
        GET_THIS(oStringMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oStringMapIterStruct* pIter = new oStringMapIterStruct;
        TRACK_ITER_NEW;
        ForthClassVocabulary *pClassVocab = GET_CLASS_VOCABULARY(kBCIStringMapIter);
        pIter->pMethods = pClassVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oStringMap::iterator;
        *(pIter->cursor) = pMap->elements->end();
        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringMapFindMethod)
    {
        GET_THIS(oStringMapStruct, pMap);
        long found = 0;
        ForthObject soughtObj;
        POP_OBJECT(soughtObj);
        oStringMap::iterator iter;
        oStringMap& a = *(pMap->elements);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            if (OBJECTS_SAME(o, soughtObj))
            {
                found = ~0;
                pMap->refCount++;
                TRACK_KEEP;
                // needed to use new instead of malloc otherwise the iterator isn't setup right and
                //   a crash happens when you assign to it
                oStringMapIterStruct* pIter = new oStringMapIterStruct;
                TRACK_ITER_NEW;
                ForthClassVocabulary *pClassVocab = GET_CLASS_VOCABULARY(kBCIStringMapIter);
                pIter->pMethods = pClassVocab->GetMethods();
                pIter->refCount = 0;
                pIter->parent = reinterpret_cast<ForthObject>(pMap);
                pIter->cursor = new oStringMap::iterator;
                *(pIter->cursor) = iter;
                PUSH_OBJECT(pIter);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oStringMapCountMethod)
    {
        GET_THIS(oStringMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oStringMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringMapStruct, pMap);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringMapGrabMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
        long found = 0;
        oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

	FORTHOP(oStringMapSetMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
        std::string key;
        key = (const char*)(SPOP);
        ForthObject newObj;
        POP_OBJECT(newObj);

        setStringMap(pMap, key, newObj, pCore);

        METHOD_RETURN;
	}

    FORTHOP(oStringMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oStringMapStruct, pMap);
        oStringMap::iterator iter;
        oStringMap& a = *(pMap->elements);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            std::string key;
            key = (const char*)(SPOP);
            ForthObject newObj;
            POP_OBJECT(newObj);
            SAFE_KEEP(newObj);
            a[key] = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringMapFindValueMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oStringMap::iterator iter;
		oStringMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = ~0;
                SPUSH(((long)(iter->first.c_str())));
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapRemoveMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapUnrefMethod)
	{
		GET_THIS(oStringMapStruct, pMap);
		oStringMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringMapMembers[] =
	{
		METHOD("__newOp", oStringMapNew),
		METHOD("delete", oStringMapDeleteMethod),
		METHOD("showInner", oStringMapShowInnerMethod),

		METHOD_RET("headIter", oStringMapHeadIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("tailIter", oStringMapTailIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("find", oStringMapFindMethod, RETURNS_OBJECT(kBCIMapIter)),
		//METHOD_RET( "clone",                oStringMapCloneMethod, RETURNS_OBJECT(kBCIMap) ),
		METHOD_RET("count", oStringMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oStringMapClearMethod),

        METHOD_RET("grab", oStringMapGrabMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("set", oStringMapSetMethod),
        METHOD("load", oStringMapLoadMethod),
        METHOD_RET("findValue", oStringMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringMapRemoveMethod),
		METHOD("unref", oStringMapUnrefMethod),

        MEMBER_VAR( "__elements",       NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringMapIter
	//

	FORTHOP(oStringMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a oStringMapIter object");
	}

	FORTHOP(oStringMapIterDeleteMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekNextMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekPrevMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekHeadMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterSeekTailMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oStringMapIterAtHeadMethod)
    {
        GET_THIS(oStringMapIterStruct, pIter);
        oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStringMapIterAtTailMethod)
    {
        GET_THIS(oStringMapIterStruct, pIter);
        oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStringMapIterNextMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterPrevMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterCurrentMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterRemoveMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		if (*(pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*(pIter->cursor))->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterFindNextMethod)
	{
		SPUSH(0);
		METHOD_RETURN;
	}

	FORTHOP(oStringMapIterCurrentPairMethod)
	{
		GET_THIS(oStringMapIterStruct, pIter);
		oStringMapStruct* pMap = reinterpret_cast<oStringMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH((long)(*(pIter->cursor))->first.c_str());
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oStringMapIterMembers[] =
	{
		METHOD("__newOp", oStringMapIterNew),
		METHOD("delete", oStringMapIterDeleteMethod),

		METHOD("seekNext", oStringMapIterSeekNextMethod),
		METHOD("seekPrev", oStringMapIterSeekPrevMethod),
		METHOD("seekHead", oStringMapIterSeekHeadMethod),
		METHOD("seekTail", oStringMapIterSeekTailMethod),
        METHOD_RET("atHead", oStringMapIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oStringMapIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oStringMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oStringMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oStringMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringMapIterRemoveMethod),
		METHOD_RET("findNext", oStringMapIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD_RET( "clone",                oStringMapIterCloneMethod, RETURNS_OBJECT(kBCIMapIter) ),

		METHOD_RET("currentPair", oStringMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR( "__cursor",			NATIVE_TYPE_TO_CODE(0, kBaseTypeInt) ),
        
		// following must be last in table
		END_MEMBERS
	};

    // functions for string output streams
	void stringCharOut( ForthCoreState* pCore, void *pData, char ch )
	{
        oStringStruct* pString = reinterpret_cast<oStringStruct*>(static_cast<oStringOutStreamStruct*>(pData)->outString);
        appendOString( pString, &ch, 1 );
	}

	void stringBlockOut( ForthCoreState* pCore, void *pData, const char *pBuffer, int numChars )
	{
        oStringStruct* pString = reinterpret_cast<oStringStruct*>(static_cast<oStringOutStreamStruct*>(pData)->outString);
        appendOString( pString, pBuffer, numChars );
	}

	void stringStringOut( ForthCoreState* pCore, void *pData, const char *pBuffer )
	{
		oStringStruct* pString = reinterpret_cast<oStringStruct*>(static_cast<oStringOutStreamStruct*>(pData)->outString);
		int numChars = strlen( pBuffer );
		appendOString( pString, pBuffer, numChars );
	}

    bool customStringReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "value")
        {
            oStringStruct *dstString = (oStringStruct *)(reader->getCustomReaderContext().pData);
            std::string value;
            reader->getString(value);
            setString(dstString, value.c_str());
            return true;
        }
        else
        {
            if ((elementName == "curLen") || (elementName == "maxLen") || (elementName == "hashVal"))
            {
                std::string ignoredValue;
                reader->getNumber(ignoredValue);
                return true;
            }
        }
        return false;
    }

    bool customStringMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oStringMapStruct *dstMap = (oStringMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string name;
            ForthObject obj;
            while (true)
            {
                char ch = reader->getChar();
                if (ch == '}')
                {
                    break;
                }
                if (ch != ',')
                {
                    reader->ungetChar(ch);
                }
                reader->getString(name);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&obj);
                setStringMap(dstMap, name, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    void AddClasses(ForthEngine* pEngine)
	{
        gpStringClassVocab = pEngine->AddBuiltinClass("String", kBCIString, kBCIObject, oStringMembers);
        gpStringClassVocab->SetCustomObjectReader(customStringReader);

        gpStringMapClassVocab = pEngine->AddBuiltinClass("StringMap", kBCIStringMap, kBCIIterable, oStringMapMembers);
        gpStringMapClassVocab->SetCustomObjectReader(customStringMapReader);

        pEngine->AddBuiltinClass("StringMapIter", kBCIStringMapIter, kBCIIter, oStringMapIterMembers);
	}

} // namespace oString
