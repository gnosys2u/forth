//////////////////////////////////////////////////////////////////////
//
// DLLVocabulary.cpp: vocabulary for dynamic loaded libraries
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "DLLVocabulary.h"
#include "ForthEngine.h"
#include "OuterInterpreter.h"

//////////////////////////////////////////////////////////////////////
////
///     DLLVocabulary
//
//

DLLVocabulary::DLLVocabulary(const char      *pName,
    const char      *pDLLName,
    int             valueLongs,
    int             storageBytes,
    void*           pForgetLimit,
    forthop         op)
    : ForthVocabulary(pName, valueLongs, storageBytes, pForgetLimit, op)
    , mDLLFlags(0)
{
    int len = strlen(pDLLName) + 1;
    mpDLLName = new char[len];
    strcpy(mpDLLName, pDLLName);
    LoadDLL();
    mType = VocabularyType::kDLL;
}

DLLVocabulary::~DLLVocabulary()
{
    UnloadDLL();
    delete [] mpDLLName;
}

void* DLLVocabulary::LoadDLL( void )
{

    ForthEngine* pEngine = ForthEngine::GetInstance();
    ForthShell* pShell = pEngine->GetShell();
    char* pDLLPath = nullptr;
    const char *pDLLSrc = mpDLLName;
    int len = strlen(mpDLLName) + 1;
    if (!pEngine->GetCoreState()->pFileFuncs->fileExists(mpDLLName))
    {
        const char* pDLLDir = pShell->GetDLLDir();
        pDLLPath = new char[strlen(pDLLDir) + len];
        strcpy(pDLLPath, pDLLDir);
        strcat(pDLLPath, mpDLLName);
        pDLLSrc = pDLLPath;
    }
#if defined(WINDOWS_BUILD)
    mhDLL = LoadLibrary(pDLLSrc);
    if (mhDLL == 0)
    {
        pEngine->SetError(ForthError::kFileOpen, "failed to open DLL ");
        pEngine->AddErrorText(pDLLSrc);
    }
    delete[] pDLLPath;
    return mhDLL;
#elif defined(LINUX) || defined(MACOSX)
    mLibHandle = dlopen(pDLLSrc, RTLD_LAZY);
    if (mLibHandle == nullptr)
    {
        pEngine->SetError(ForthError::kFileOpen, "failed to open DLL ");
        pEngine->AddErrorText(pDLLSrc);
    }
    delete[] pDLLPath;
    return mLibHandle;
#endif
}

void DLLVocabulary::UnloadDLL( void )
{
#if defined(WINDOWS_BUILD)
    if ( mhDLL != 0 )
    {
    	FreeLibrary( mhDLL );
    	mhDLL = 0;
    }
#elif defined(LINUX) || defined(MACOSX)
    if ( mLibHandle != NULL )
    {
        dlclose( mLibHandle  );
        mLibHandle = NULL;
    }
#endif
}

forthop * DLLVocabulary::AddEntry( const char *pFuncName, const char* pEntryName, int32_t numArgs )
{
    forthop *pEntry = NULL;
#if defined(WINDOWS_BUILD)
    void* pFunc = GetProcAddress( mhDLL, pFuncName );
#elif defined(LINUX) || defined(MACOSX)
    void* pFunc = dlsym( mLibHandle, pFuncName );
#endif
    if (pFunc )
    {
        forthop dllOp = mpEngine->GetOuterInterpreter()->AddOp(pFunc);
        dllOp = COMPILED_OP(kOpDLLEntryPoint, dllOp) | ((numArgs << 19) | mDLLFlags);
        pEntry = AddSymbol(pEntryName, dllOp);
    }
    else
    {
        mpEngine->SetError( ForthError::kUnknownSymbol, " unknown entry point" );
    }
	// void and 64-bit flags only apply to one vocabulary entry
	mDLLFlags &= ~(DLL_ENTRY_FLAG_RETURN_VOID | DLL_ENTRY_FLAG_RETURN_64BIT);

    return pEntry;
}

const char*
DLLVocabulary::GetDescription( void )
{
    return "dllOp";
}

void
DLLVocabulary::SetFlag( uint32_t flag )
{
	mDLLFlags |= flag;
}

