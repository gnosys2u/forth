//////////////////////////////////////////////////////////////////////
//
// VocabularyStack.cpp: stack of vocabularies
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "VocabularyStack.h"
#include "ForthEngine.h"
#include "OuterInterpreter.h"
#include "ForthParseInfo.h"

//////////////////////////////////////////////////////////////////////
////
///     VocabularyStack
//
//

VocabularyStack::VocabularyStack( int maxDepth )
: mStack( NULL )
, mMaxDepth( maxDepth )
, mTop( 0 )
, mSerial( 0 )
{
    mpEngine = Engine::GetInstance();
}

VocabularyStack::~VocabularyStack()
{
    delete mStack;
}

void VocabularyStack::Initialize( void )
{
    delete mStack;
    mTop = 0;
    mStack = new Vocabulary* [ mMaxDepth ];
}

void VocabularyStack::DupTop( void )
{
    if ( mTop < (mMaxDepth - 1) )
    {
        mTop++;
        mStack[ mTop ] = mStack[ mTop - 1 ];
    }
    else
    {
        // TBD: report overflow
    }
}

bool VocabularyStack::DropTop( void )
{
    if ( mTop )
    {
        mTop--;
    }
    else
    {
        return false;
    }
    return true;
}

void VocabularyStack::Clear( void )
{
    mTop = 0;
    mStack[0] = mpEngine->GetOuterInterpreter()->GetForthVocabulary();
//    mStack[1] = mpEngine->GetPrecedenceVocabulary();
}

void VocabularyStack::SetTop( Vocabulary* pVocab )
{
    mStack[mTop] = pVocab;
}

Vocabulary* VocabularyStack::GetTop( void )
{
    return mStack[mTop];
}

Vocabulary* VocabularyStack::GetElement( int depth )
{
    return (depth > mTop) ? NULL : mStack[mTop - depth];
}

// return pointer to symbol entry, NULL if not found
// ppFoundVocab will be set to the vocabulary the symbol was actually found in
// set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
forthop* VocabularyStack::FindSymbol( const char *pSymName, Vocabulary** ppFoundVocab )
{
    forthop* pEntry = NULL;
    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pSymName, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    if ( (pEntry == NULL) && mpEngine->GetOuterInterpreter()->CheckFeature( kFFIgnoreCase ) )
    {
        // if symbol wasn't found, convert it to lower case and try again
        char buffer[128];
        strncpy( buffer, pSymName, sizeof(buffer) );
        for ( int i = 0; i < sizeof(buffer); i++ )
        {
            char ch = buffer[i];
            if ( ch == '\0' )
            {
                break;
            }
            buffer[i] = tolower( ch );
        }
        // try to find the lower cased version
        mSerial++;
        for ( int i = mTop; i >= 0; i-- )
        {
            pEntry = mStack[i]->FindSymbol( buffer, mSerial );
            if ( pEntry )
            {
                if ( ppFoundVocab != NULL )
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    return pEntry;
}

// return pointer to symbol entry, NULL if not found, given its value
forthop * VocabularyStack::FindSymbolByValue( int32_t val, Vocabulary** ppFoundVocab )
{
    forthop *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbolByValue( val, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    return pEntry;
}

// return pointer to symbol entry, NULL if not found
// pSymName is required to be a longword aligned address, and to be padded with 0's
// to the next longword boundary
forthop * VocabularyStack::FindSymbol( ParseInfo *pInfo, Vocabulary** ppFoundVocab )
{
    forthop *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pInfo, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }

    if ( (pEntry == NULL) && mpEngine->GetOuterInterpreter()->CheckFeature( kFFIgnoreCase ) )
    {
        // if symbol wasn't found, convert it to lower case and try again
        char buffer[128];
        strncpy( buffer, pInfo->GetToken(), sizeof(buffer) );
        for ( int i = 0; i < sizeof(buffer); i++ )
        {
            char ch = buffer[i];
            if ( ch == '\0' )
            {
                break;
            }
            buffer[i] = tolower( ch );
        }
        // try to find the lower cased version
        mSerial++;
        for ( int i = mTop; i >= 0; i-- )
        {
            pEntry = mStack[i]->FindSymbol( buffer, mSerial );
            if ( pEntry )
            {
                if ( ppFoundVocab != NULL )
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    return pEntry;
}

