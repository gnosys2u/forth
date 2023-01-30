#pragma once
//////////////////////////////////////////////////////////////////////
//
// LocalVocabulary.h: vocabulary for local variables
//
//////////////////////////////////////////////////////////////////////

#include "Vocabulary.h"

#define MAX_LOCAL_DEPTH 16
#define LOCAL_STACK_STRIDE 3

class LocalVocabulary : public Vocabulary
{
public:
    LocalVocabulary( const char *pName=NULL,
                               int valueLongs=NUM_LOCALS_VOCAB_VALUE_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
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

