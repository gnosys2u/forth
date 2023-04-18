#pragma once
//////////////////////////////////////////////////////////////////////
//
// VocabularyStack.h: stack of vocabularies
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

