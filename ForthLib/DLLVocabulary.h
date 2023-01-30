#pragma once
//////////////////////////////////////////////////////////////////////
//
// DLLVocabulary.h: vocabulary for dynamic loaded libraries
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthVocabulary.h"

class DLLVocabulary : public Vocabulary
{
public:
    DLLVocabulary( const char *pVocabName,
                        const char *pDLLName,
                        int valueLongs = DEFAULT_VALUE_FIELD_LONGS,
                        int storageBytes = DEFAULT_VOCAB_STORAGE,
                        void* pForgetLimit = NULL,
                        forthop op = 0 );
    virtual ~DLLVocabulary();

    // return a string telling the type of library
    virtual const char* GetDescription( void );

    void *              LoadDLL( void );
    void                UnloadDLL( void );
	forthop*            AddEntry(const char* pFuncName, const char* pEntryName, int32_t numArgs);
	void				SetFlag( uint32_t flag );
protected:
    char *              mpDLLName;
	uint32_t		mDLLFlags;
#if defined(WINDOWS_BUILD)
    HINSTANCE           mhDLL;
#endif
#if defined(LINUX) || defined(MACOSX)
    void*				mLibHandle;
#endif
};

