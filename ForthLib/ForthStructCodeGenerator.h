#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthStructs.h: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class ForthTypesManager;
class ForthStructVocabulary;
class ForthParseInfo;

class ForthStructCodeGenerator {

public:
	ForthStructCodeGenerator( ForthTypesManager* pTypeManager );
	~ForthStructCodeGenerator();
	
	bool Generate( ForthParseInfo *pInfo, forthop*& pDst, int dstLongs );
	bool UncompileLastOpcode() { return mCompileVarop != 0; }

protected:
	bool HandleFirst();
	bool HandleMiddle();
	bool HandleLast();
	bool IsLast();
	void HandlePreceedingVarop();
	
	uint32_t mTOSTypeCode;
	uint32_t mTypeCode;

	ForthParseInfo* mpParseInfo;
    ForthStructVocabulary* mpStructVocab;
    ForthStructVocabulary* mpContainedClassVocab;
    ForthTypesManager* mpTypeManager;
    forthop* mpDst;
    forthop* mpDstBase;
	int	mDstLongs;
	char* mpBuffer;
	int mBufferBytes;
	char* mpToken;
	char* mpNextToken;
	uint32_t mCompileVarop;
	uint32_t mOffset;
	char mErrorMsg[ 512 ];
    bool mUsesSuper;
    VarOperation mSuffixVarop;
};


