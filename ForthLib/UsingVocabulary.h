#pragma once
//////////////////////////////////////////////////////////////////////
//
// UsingVocabulary.h: vocabulary for 'using:' feature
//
//////////////////////////////////////////////////////////////////////

#include "Vocabulary.h"

#define MAX_LOCAL_DEPTH 16
#define LOCAL_STACK_STRIDE 3

class UsingVocabulary : public Vocabulary
{
public:
    UsingVocabulary();
    virtual ~UsingVocabulary();

    // return a string telling the type of library
    const char* GetDescription( void ) override;

	void                Push(Vocabulary* pVocab);
    void                Empty( void ) override;

	// continue searching a vocabulary 
    forthop*            FindNextSymbol( ParseInfo *pInfo, forthop* pStartEntry, ucell serial=0 ) override;
    // compile/interpret entry returned by FindSymbol
    OpResult            ProcessEntry(forthop* pEntry ) override;

    // do-nothing ops
    void        ForgetCleanup( void *pForgetLimit, forthop op ) override;
    void        DoOp( CoreState *pCore ) override;
    forthop*    AddSymbol( const char *pSymName, forthop symValue) override;
    void        CopyEntry(forthop* pEntry ) override;
    void        DeleteEntry(forthop* pEntry ) override;
    bool        ForgetSymbol( const char   *pSymName ) override;
    void        ForgetOp( forthop op ) override;
    forthop*    FindSymbolByValue(forthop val, ucell serial=0 ) override;
    forthop*    FindNextSymbolByValue(forthop val, forthop* pStartEntry, ucell serial=0 ) override;
    void        PrintEntry(forthop*   pEntry ) override;
    void        SmudgeNewestSymbol( void ) override;
    void        UnSmudgeNewestSymbol( void ) override;
    int         GetEntryName( const forthop* pEntry, char *pDstBuff, int buffSize ) override;

protected:
    std::vector<Vocabulary*> mStack;
};

