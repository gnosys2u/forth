//////////////////////////////////////////////////////////////////////
//
// DLLVocabulary.cpp: vocabulary for dynamic loaded libraries
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "DLLVocabulary.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#if defined(LINUX) || defined(MACOSX)
#include <dlfcn.h>
#endif

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
    : Vocabulary(pName, valueLongs, storageBytes, pForgetLimit, op)
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

    Engine* pEngine = Engine::GetInstance();
    Shell* pShell = pEngine->GetShell();
    char* pDLLPath = nullptr;
    const char *pDLLSrc = mpDLLName;
    int len = strlen(mpDLLName) + 1;
    std::string containingDir;
    std::string dllPath;

    if (pShell->FindFileInPaths(mpDLLName, pShell->GetDllPaths(), containingDir))
    {
        dllPath = containingDir;
        dllPath.append(mpDLLName);
    }

#if defined(WINDOWS_BUILD)
    mhDLL = LoadLibrary(dllPath.c_str());
    if (mhDLL == 0)
    {
        pEngine->SetError(ForthError::openFile, "failed to open DLL ");
        pEngine->AddErrorText(pDLLSrc);
    }
    delete[] pDLLPath;
    return mhDLL;
#elif defined(LINUX) || defined(MACOSX)
    mLibHandle = dlopen(pDLLSrc, RTLD_LAZY);
    if (mLibHandle == nullptr)
    {
        pEngine->SetError(ForthError::openFile, "failed to open DLL ");
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
        mpEngine->SetError( ForthError::undefinedWord, " unknown entry point" );
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

