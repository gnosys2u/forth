#pragma once
//////////////////////////////////////////////////////////////////////
//
// OpcodeCompiler.h: interface for the OpcodeCompiler class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////


class OpcodeCompiler
{
public:
                    OpcodeCompiler( MemorySection*	mpDictionarySection );
			        ~OpcodeCompiler();
	void			Reset();
	void			CompileOpcode( forthOpType opType, forthop opVal );
    void			PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode);
    void			UncompileLastOpcode();
	uint32_t	PeepholeValidCount();
	void			ClearPeephole();
    forthop*        GetLastCompiledOpcodePtr();
    forthop*        GetLastCompiledIntoPtr();
	bool			GetPreviousOpcode( forthOpType& opType, forthop& opVal, uint32_t index = 0 );
	forthop*		GetPreviousOpcodeAddress(uint32_t index = 0);

// MAX_PEEPHOLE_PTRS must be power of 2
#define MAX_PEEPHOLE_PTRS	8
#define PEEPHOLE_PTR_MASK   (MAX_PEEPHOLE_PTRS - 1)
private:
	MemorySection*	mpDictionarySection;
    forthop*        mPeephole[MAX_PEEPHOLE_PTRS];
	uint32_t	mPeepholeIndex;
	uint32_t	mPeepholeValidCount;
    forthop*        mpLastIntoOpcode;
    int32_t            mCompileComboOpFlags;
};

