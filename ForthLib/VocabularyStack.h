#pragma once
//////////////////////////////////////////////////////////////////////
//
// VocabularyStack.h: stack of vocabularies
//
//////////////////////////////////////////////////////////////////////


#include "Forth.h"
#include "ForthVocabulary.h"

class VocabularyStack
{
public:
    VocabularyStack( int maxDepth=256 );
    ~VocabularyStack();
    void                Initialize( void );
    void                DupTop( void );
    bool                DropTop( void );
    Vocabulary*    GetTop( void );
    void                SetTop( Vocabulary* pVocab );
    void                Clear();
    // GetElement(0) is the same as GetTop
    Vocabulary*    GetElement( int depth );
	inline int			GetDepth() { return mTop + 1; }

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
    Vocabulary**   mStack;
    Engine*        mpEngine;
    int                 mMaxDepth;
    int                 mTop;
    ucell               mSerial;
};

