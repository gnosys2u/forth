#pragma once
//////////////////////////////////////////////////////////////////////
//
// VocabularyStack.h: stack of vocabularies
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
#include "Vocabulary.h"

class VocabularyStack
{
public:
    VocabularyStack( int maxDepth=256 );
    ~VocabularyStack();
    void                Initialize( Vocabulary* pRootVocab );
    void                DupTop( void );
    bool                DropTop( void );
    Vocabulary*         GetTop( void );
    void                SetTop(Vocabulary* pVocab);
    void                Push(Vocabulary* pVocab);
    void                Clear(Vocabulary* pOnlyVocab = nullptr);
    // GetElement(0) is the same as GetTop
    Vocabulary*         GetElement( ucell depth );
	inline ucell		GetDepth() { return mDepth; }

    // return pointer to symbol entry, NULL if not found
    // ppFoundVocab will be set to the vocabulary the symbol was actually found in
    // set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
    forthop*    FindSymbol( const char *pSymName, Vocabulary** ppFoundVocab=NULL );

    // return pointer to symbol entry, NULL if not found, given its value
    forthop*    FindSymbolByValue( int32_t val, Vocabulary** ppFoundVocab=NULL );

    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    forthop*    FindSymbol( ParseInfo *pInfo, Vocabulary** ppFoundVocab=NULL );

private:
    forthop* FindSymbolInner(const char* pSymName, Vocabulary** ppFoundVocab);
    forthop* FindSymbolInner(ParseInfo* pInfo, Vocabulary** ppFoundVocab);

    Vocabulary**   mStack;
    Engine*        mpEngine;
    Vocabulary*    mpRootVocab;                // root vocabulary
    ucell          mMaxDepth;
    ucell          mDepth;
    ucell          mSerial;
};

