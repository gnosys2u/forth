//////////////////////////////////////////////////////////////////////
//
// UsingVocabulary.cpp: vocabulary for the 'using:' feature
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "UsingVocabulary.h"
#include "Engine.h"
#include "ParseInfo.h"
#include "OuterInterpreter.h"

/*
  UsingVocabulary is used to provide an easy way to generate offsets
  to structure members in assembly code.
*/

#define LABEL_ENDCHAR '#'

//////////////////////////////////////////////////////////////////////
////
///     UsingVocabulary
//
//

UsingVocabulary::UsingVocabulary() 
: Vocabulary( "_using", 0, 16 )
{
    mType = VocabularyType::kUsing;
    mValueLongs = 0;
}

UsingVocabulary::~UsingVocabulary()
{
}

const char* UsingVocabulary::GetDescription( void )
{
    return "using";
}

void UsingVocabulary::Push(Vocabulary* pVocab)
{
    if (mStack.empty())
    {
        // add ourselves to top of vocab stack
        OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
        VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
        pVocabStack->DupTop();
        pVocabStack->SetTop(this);
    }

    mStack.push_back(pVocab);
}

void UsingVocabulary::Empty()
{
    Vocabulary::Empty();

    // remove ourself from top of vocab stack
    if (!mStack.empty())
    {
        mStack.clear();
        OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
        VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
        if (pVocabStack->GetTop() == this)
        {
            pVocabStack->DropTop();
        }
    }
}

// continue searching a vocabulary 
forthop* UsingVocabulary::FindNextSymbol(ParseInfo* pInfo, forthop* pStartEntry, ucell serial)
{
    forthop* result = nullptr;
    char* pToken = pInfo->GetToken();
    int originalTokenLength = pInfo->GetTokenLength();
    if (pToken[originalTokenLength - 1] == LABEL_ENDCHAR)
    {
        // if symbol ends in ')', it might be a structure member name,
        //  so strip off the ')' and try to find it in the stack of using vocabs
        pToken[originalTokenLength - 1] = '\0';
        pInfo->UpdateLength(originalTokenLength - 1);

        for (Vocabulary* vocab : mStack)
        {
            forthop* pEntry = vocab->FindSymbol(pInfo);
            uint32_t typeCode = VOCABENTRY_TO_TYPECODE(pEntry);
            if (CODE_TO_BASE_TYPE(typeCode) <= BaseType::kStruct)
            {
                result = pEntry;
                break;
            }
        }

        pToken[originalTokenLength - 1] = LABEL_ENDCHAR;      // restore the token to original state
        pInfo->UpdateLength(originalTokenLength);
    }
    
    return result;
}

// compile/interpret entry returned by FindSymbol
OpResult UsingVocabulary::ProcessEntry(forthop* pEntry)
{
    OpResult exitStatus = OpResult::kError;

    if (pEntry)
    {
        int64_t offset = (int64_t)*pEntry;
        mpEngine->GetOuterInterpreter()->ProcessConstant(offset, false, true);
        exitStatus = OpResult::kOk;
    }
    return exitStatus;
}

// do-nothing ops
void UsingVocabulary::ForgetCleanup( void *pForgetLimit, forthop op ) {}
void UsingVocabulary::DoOp( CoreState *pCore ) {}
forthop* UsingVocabulary::AddSymbol(const char* pSymName, forthop symValue) { return nullptr; }
void UsingVocabulary::CopyEntry(forthop* pEntry ) {}
void UsingVocabulary::DeleteEntry(forthop* pEntry ) {}
bool UsingVocabulary::ForgetSymbol(const char* pSymName) { return true; }
void UsingVocabulary::ForgetOp( forthop op ) {}
forthop* UsingVocabulary::FindSymbolByValue(forthop val, ucell serial) { return nullptr; }
forthop* UsingVocabulary::FindNextSymbolByValue(forthop val, forthop* pStartEntry, ucell serial) { return nullptr; }
void UsingVocabulary::PrintEntry(forthop*   pEntry ) {}
void UsingVocabulary::SmudgeNewestSymbol( void ) {}
void UsingVocabulary::UnSmudgeNewestSymbol( void ) {}
int UsingVocabulary::GetEntryName(const forthop* pEntry, char* pDstBuff, int buffSize) { return 0; }

