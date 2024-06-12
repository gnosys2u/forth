#pragma once
//////////////////////////////////////////////////////////////////////
//
// LocalVocabulary.h: vocabulary for local variables
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

#include "Vocabulary.h"

#define MAX_LOCAL_DEPTH 16
#define LOCAL_STACK_STRIDE 3

class LocalVocabulary : public Vocabulary
{
public:
    LocalVocabulary( int valueLongs=NUM_LOCALS_VOCAB_VALUE_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~LocalVocabulary();

    // return a string telling the type of library
    virtual const char* GetDescription( void );

	void				Push();
	void				Pop();

	int					GetFrameCells();
    forthop*            GetFrameAllocOpPointer();
    forthop*			AddVariable( const char* pVarName, int32_t fieldType, int32_t varValue, int nCells );
	void				ClearFrame();

protected:
	int					mDepth;
	cell				mStack[ MAX_LOCAL_DEPTH * LOCAL_STACK_STRIDE ];
    forthop*            mpAllocOp;
	int					mFrameCells;
};

