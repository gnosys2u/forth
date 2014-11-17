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
	void			CompileOpcode( forthOpType opType, long opVal );
	void			UncompileLastOpcode();
	unsigned int	PeepholeValidCount();
	void			ClearPeephole();
    long*           GetLastCompiledOpcodePtr();
    long*           GetLastCompiledIntoPtr();
	bool			GetPreviousOpcode( forthOpType& opType, long& opVal );
// MAX_PEEPHOLE_PTRS must be power of 2
#define MAX_PEEPHOLE_PTRS	8
private:
	ForthMemorySection*	mpDictionarySection;
	long*			mPeephole[MAX_PEEPHOLE_PTRS];
	unsigned int	mPeepholeIndex;
	unsigned int	mPeepholeValidCount;
	long*			mpLastIntoOpcode;
};
