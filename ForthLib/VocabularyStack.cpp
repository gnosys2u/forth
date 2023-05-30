//////////////////////////////////////////////////////////////////////
//
// VocabularyStack.cpp: stack of vocabularies
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "VocabularyStack.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "ParseInfo.h"

//////////////////////////////////////////////////////////////////////
////
///     VocabularyStack
//
//

VocabularyStack::VocabularyStack( int maxDepth )
: mStack( NULL )
, mMaxDepth( maxDepth )
, mDepth( 0 )
, mSerial( 0 )
{
    mpEngine = Engine::GetInstance();
}

VocabularyStack::~VocabularyStack()
{
    delete mStack;
}

void VocabularyStack::Initialize(Vocabulary* pRootVocab)
{
    mpRootVocab = pRootVocab;
    delete mStack;
    mDepth = 0;
    mStack = new Vocabulary* [ mMaxDepth ];
}

void VocabularyStack::DupTop( void )
{
    if (mDepth < (mMaxDepth - 1))
    {
        if (mDepth == 0)
        {
            mStack[mDepth] = mpRootVocab;
        }
        else
        {
            mStack[mDepth] = mStack[mDepth - 1];
        }
        mDepth++;
    }
    else
    {
        // TBD: report overflow
    }
}

bool VocabularyStack::DropTop( void )
{
    if ( mDepth )
    {
        mDepth--;
    }
    else
    {
        return false;
    }
    return true;
}

void VocabularyStack::Clear(Vocabulary* pOnlyVocab)
{
    mDepth = 0;
    if (pOnlyVocab)
    {
        Push(pOnlyVocab);
    }
}

void VocabularyStack::SetTop( Vocabulary* pVocab )
{
    if (mDepth)
    {
        mStack[mDepth - 1] = pVocab;
    }
    else
    {
        mStack[mDepth] = pVocab;
        mDepth++;
    }
}

void VocabularyStack::Push(Vocabulary* pVocab)
{
    mStack[mDepth] = pVocab;
    mDepth++;
}

Vocabulary* VocabularyStack::GetTop( void )
{
    return mDepth ? mStack[mDepth - 1] : mpRootVocab;
}

Vocabulary* VocabularyStack::GetElement( ucell depth )
{
    Vocabulary* pVocab = nullptr;

    if (mDepth && depth < mDepth)
    {
        pVocab = mStack[mDepth - (depth + 1)];
    }
    return pVocab;
}

// return pointer to symbol entry, NULL if not found
// ppFoundVocab will be set to the vocabulary the symbol was actually found in
// set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
forthop* VocabularyStack::FindSymbol( const char *pSymName, Vocabulary** ppFoundVocab )
{
    mSerial++;
    forthop* pEntry = FindSymbolInner(pSymName, ppFoundVocab);

    if ( (pEntry == nullptr) && mpEngine->GetOuterInterpreter()->CheckFeature( kFFIgnoreCase ) )
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
        pEntry = FindSymbolInner(buffer, ppFoundVocab);

        if (pEntry == nullptr)
        {
            // last chance - try upper case 
            for (int i = 0; i < sizeof(buffer); i++)
            {
                char ch = buffer[i];
                if (ch == '\0')
                {
                    break;
                }
                buffer[i] = toupper(ch);
            }
            mSerial++;
            pEntry = FindSymbolInner(buffer, ppFoundVocab);
        }
    }

    return pEntry;
}

forthop* VocabularyStack::FindSymbolInner(const char* pSymName, Vocabulary** ppFoundVocab)
{
    forthop* pEntry = nullptr;

    if (mDepth)
    {
        for (int i = mDepth - 1; i >= 0; i--)
        {
            pEntry = mStack[i]->FindSymbol(pSymName, mSerial);
            if (pEntry)
            {
                if (ppFoundVocab != nullptr)
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    if (pEntry == nullptr)
    {
        // check root vocab
        pEntry = mpRootVocab->FindSymbol(pSymName, mSerial);
        if (pEntry)
        {
            if (ppFoundVocab != nullptr)
            {
                *ppFoundVocab = mpRootVocab;
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
    if (mDepth)
    {
        for (int i = mDepth - 1; i >= 0; i--)
        {
            pEntry = mStack[i]->FindSymbolByValue(val, mSerial);
            if (pEntry)
            {
                if (ppFoundVocab != nullptr)
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    if (pEntry == nullptr)
    {
        // check root vocab
        pEntry = mpRootVocab->FindSymbolByValue(val, mSerial);
        if (pEntry)
        {
            if (ppFoundVocab != nullptr)
            {
                *ppFoundVocab = mpRootVocab;
            }
        }
    }

    return pEntry;
}

// return pointer to symbol entry, NULL if not found
// pSymName is required to be a longword aligned address, and to be padded with 0's
// to the next longword boundary
forthop * VocabularyStack::FindSymbol( ParseInfo *pInfo, Vocabulary** ppFoundVocab )
{
    mSerial++;
    forthop* pEntry = FindSymbolInner(pInfo, ppFoundVocab);

    if ((pEntry == nullptr) && mpEngine->GetOuterInterpreter()->CheckFeature(kFFIgnoreCase))
    {
        // if symbol wasn't found, change to lower case and try again
        char buffer[128];
        strncpy(buffer, pInfo->GetToken(), sizeof(buffer));
        for (int i = 0; i < sizeof(buffer); i++)
        {
            char ch = buffer[i];
            if (ch == '\0')
            {
                break;
            }
            buffer[i] = tolower(ch);
        }

        // try to find the lower case version
        mSerial++;
        pEntry = FindSymbolInner(buffer, ppFoundVocab);

        if (pEntry == nullptr)
        {
            // last chance - try upper case 
            for (int i = 0; i < sizeof(buffer); i++)
            {
                char ch = buffer[i];
                if (ch == '\0')
                {
                    break;
                }
                buffer[i] = toupper(ch);
            }
            mSerial++;
            pEntry = FindSymbolInner(buffer, ppFoundVocab);
        }

    }

    return pEntry;
}

forthop* VocabularyStack::FindSymbolInner(ParseInfo* pInfo, Vocabulary** ppFoundVocab)
{
    forthop* pEntry = nullptr;

    if (mDepth)
    {
        for (int i = mDepth - 1; i >= 0; i--)
        {
            pEntry = mStack[i]->FindSymbol(pInfo, mSerial);
            if (pEntry)
            {
                if (ppFoundVocab != nullptr)
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    if (pEntry == nullptr)
    {
        // check root vocab
        pEntry = mpRootVocab->FindSymbol(pInfo, mSerial);
        if (pEntry)
        {
            if (ppFoundVocab != nullptr)
            {
                *ppFoundVocab = mpRootVocab;
            }
        }
    }

    return pEntry;
}

