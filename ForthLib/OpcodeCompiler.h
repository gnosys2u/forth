#pragma once
//////////////////////////////////////////////////////////////////////
//
// OpcodeCompiler.h: interface for the OpcodeCompiler class.
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

