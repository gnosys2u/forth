#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthOpcodeCompiler.h: interface for the ForthOpcodeCompiler class.
//
//////////////////////////////////////////////////////////////////////


class ForthOpcodeCompiler
{
public:
                    ForthOpcodeCompiler( ForthMemorySection*	mpDictionarySection );
			        ~ForthOpcodeCompiler();
	void			Reset();
	void			CompileOpcode( forthOpType opType, forthop opVal );
    void			PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode);
    void			UncompileLastOpcode();
	unsigned int	PeepholeValidCount();
	void			ClearPeephole();
    forthop*        GetLastCompiledOpcodePtr();
    forthop*        GetLastCompiledIntoPtr();
	bool			GetPreviousOpcode( forthOpType& opType, forthop& opVal, int index = 0 );
// MAX_PEEPHOLE_PTRS must be power of 2
#define MAX_PEEPHOLE_PTRS	8
#define PEEPHOLE_PTR_MASK   (MAX_PEEPHOLE_PTRS - 1)
private:
	ForthMemorySection*	mpDictionarySection;
    forthop*        mPeephole[MAX_PEEPHOLE_PTRS];
	unsigned int	mPeepholeIndex;
	unsigned int	mPeepholeValidCount;
    forthop*        mpLastIntoOpcode;
    long            mCompileComboOpFlags;
};

