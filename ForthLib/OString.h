#pragma once
//////////////////////////////////////////////////////////////////////
//
// OString.h: builtin string related classes
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


// first time OString:printf fails due to overflow, it buffer is increased to this size
#define OSTRING_PRINTF_FIRST_OVERFLOW_SIZE 256
// this is size limit of buffer expansion upon OString:printf overflow
#define OSTRING_PRINTF_LAST_OVERFLOW_SIZE 0x2000000


namespace OString
{
    typedef std::map<std::string, ForthObject> oStringMap;
    struct oStringMapStruct
    {
        forthop*    pMethods;
        REFCOUNTER  refCount;
        oStringMap*	elements;
    };

    struct oStringMapIterStruct
    {
        forthop*                pMethods;
        REFCOUNTER              refCount;
        ForthObject			    parent;
        oStringMap::iterator	*cursor;
    };

    extern oString* createOString(int maxChars);
	extern oString* resizeOString(oStringStruct* pString, int newLen);
	extern void appendOString(oStringStruct* pString, const char* pSrc, int numNewBytes);
	extern void prependOString(oStringStruct* pString, const char* pSrc, int numNewBytes);

    // functions for string output streams
	extern void stringCharOut( CoreState* pCore, void *pData, char ch );
	extern void stringBlockOut( CoreState* pCore, void *pData, const char *pBuffer, int numChars );
	extern void stringStringOut( CoreState* pCore, void *pData, const char *pBuffer );
    
	void AddClasses(OuterInterpreter* pOuter);
    oStringMapStruct* createStringMapObject(ClassVocabulary *pClassVocab);


    extern int gDefaultOStringSize;
    extern ClassVocabulary* gpStringClassVocab;
    extern ClassVocabulary* gpStringMapClassVocab;

    extern baseMethodEntry oStringMembers[];
    extern baseMethodEntry oStringMapMembers[];
    extern baseMethodEntry oStringMapIterMembers[];
} // namespace oString
