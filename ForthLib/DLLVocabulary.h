#pragma once
//////////////////////////////////////////////////////////////////////
//
// DLLVocabulary.h: vocabulary for dynamic loaded libraries
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

#include "Forth.h"
#include "Vocabulary.h"

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

