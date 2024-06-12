//////////////////////////////////////////////////////////////////////
//
// LocalVocabulary.cpp: vocabulary for local variables
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

#include "pch.h"
#include "LocalVocabulary.h"
#include "Engine.h"

//////////////////////////////////////////////////////////////////////
////
///     LocalVocabulary
//
//

LocalVocabulary::LocalVocabulary( 
                                    int           valueLongs,
                                    int           storageBytes )
: Vocabulary( "_locals", valueLongs, storageBytes)
, mDepth( 0 )
, mFrameCells( 0 )
, mpAllocOp( NULL )
{
    mType = VocabularyType::kLocalVariables;
}

LocalVocabulary::~LocalVocabulary()
{
}

const char* LocalVocabulary::GetDescription( void )
{
    return "local";
}

int
LocalVocabulary::GetFrameCells()
{
	return mFrameCells;
}

forthop*
LocalVocabulary::GetFrameAllocOpPointer()
{
	return mpAllocOp;
}

forthop*
LocalVocabulary::AddVariable( const char* pVarName, int32_t fieldType, int32_t varValue, int nCells)
{
    forthop op = COMPILED_OP(fieldType, varValue);
    forthop* pEntry = AddSymbol(pVarName, op);
    if (mFrameCells == 0)
    {
		mpAllocOp = mpEngine->GetDP();
	}
    mFrameCells += nCells;
	return pEntry;
}

void
LocalVocabulary::ClearFrame()
{
    mFrameCells = 0;
	mpAllocOp = NULL;
}

// 'hide' current local variables - used at start of anonymous function declaration
void
LocalVocabulary::Push()
{
	if ( mDepth < MAX_LOCAL_DEPTH )
	{
		mStack[mDepth++] = mNumSymbols;
		mStack[mDepth++] = mFrameCells;
		mStack[mDepth++] = (cell) mpAllocOp;
		mNumSymbols = 0;
        mFrameCells = 0;
		mpAllocOp = NULL;
	}
	else
	{
		// TODO: ERROR!
	}
}

void
LocalVocabulary::Pop()
{
	if ( mDepth > 0 )
	{
		while ( mNumSymbols > 0 )
		{
			mpStorageBottom = NextEntry( mpStorageBottom );
			mNumSymbols--;
		}
		mpAllocOp = (forthop *) (mStack[--mDepth]);
        mFrameCells = mStack[--mDepth];
		mNumSymbols = mStack[--mDepth];
	}
	else
	{
		// TBD: ERROR!
	}
}

