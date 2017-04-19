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
	extern int gDefaultOStringSize;

	extern oString* createOString(int maxChars);
	extern oString* resizeOString(oStringStruct* pString, int newLen);
	extern void appendOString(oStringStruct* pString, const char* pSrc, int numNewBytes);
	extern void prependOString(oStringStruct* pString, const char* pSrc, int numNewBytes);

    extern baseMethodEntry oStringMembers[];
    extern baseMethodEntry oStringMapMembers[];
    extern baseMethodEntry oStringMapIterMembers[];
  
    // functions for string output streams
	extern void stringCharOut( ForthCoreState* pCore, void *pData, char ch );
	extern void stringBlockOut( ForthCoreState* pCore, void *pData, const char *pBuffer, int numChars );
	extern void stringStringOut( ForthCoreState* pCore, void *pData, const char *pBuffer );
    
	void AddClasses(ForthEngine* pEngine);
} // namespace oString
