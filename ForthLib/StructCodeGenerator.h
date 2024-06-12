#pragma once
//////////////////////////////////////////////////////////////////////
//
// StructCodeGenerator.h: support for user-defined structures
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

#include "Forth.h"

class TypesManager;
class StructVocabulary;
class ParseInfo;

class StructCodeGenerator {

public:
	StructCodeGenerator( TypesManager* pTypeManager );
	~StructCodeGenerator();
	
	bool Generate( ParseInfo *pInfo, forthop*& pDst, int dstLongs );
	bool UncompileLastOpcode() { return mCompileVarop != 0; }

protected:
	bool HandleFirst();
	bool HandleMiddle();
	bool HandleLast();
	bool IsLast();
	void HandlePreceedingVarop();
	
	uint32_t mTOSTypeCode;
	uint32_t mTypeCode;

	ParseInfo* mpParseInfo;
    StructVocabulary* mpStructVocab;
    StructVocabulary* mpContainedClassVocab;
    TypesManager* mpTypeManager;
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
    // these two bools are used to detect localObject.method and memberObject.method cases
    bool mbLocalObjectPrevious;
    bool mbMemberObjectPrevious;
};


