//////////////////////////////////////////////////////////////////////
//
// LocalVocabulary.cpp: vocabulary for local variables
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "LocalVocabulary.h"
#include "ForthEngine.h"

//////////////////////////////////////////////////////////////////////
////
///     LocalVocabulary
//
//

LocalVocabulary::LocalVocabulary( const char    *pName,
                                            int           valueLongs,
                                            int           storageBytes )
: Vocabulary( pName, valueLongs, storageBytes )
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

