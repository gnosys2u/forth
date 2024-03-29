#pragma once
//////////////////////////////////////////////////////////////////////
//
// OString.h: builtin string related classes
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
