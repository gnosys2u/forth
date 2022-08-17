//////////////////////////////////////////////////////////////////////
//
// ForthOpcodeCompiler.cpp: implementation of the ForthOpcodeCompiler class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#endif
#include "ForthEngine.h"
#include "ForthOpcodeCompiler.h"

// compile enable flags for peephole optimizer features
enum
{
    kCENumVarop       = 1,
    kCENumOp          = 2,
    kCEOpBranch       = 4,          // enables both OpZBranch and OpNZBranch
    kCERefOp          = 8,          // enables localRefOp and memberRefOp combos
    kCEVaropVar       = 16          // enables all varop local/member/field var combos
};
//#define ENABLED_COMBO_OPS  (kCENumVarop | kCENumOp | kCEOpBranch | kCERefOp | kCEVaropVar)
#define ENABLED_COMBO_OPS  (kCEOpBranch | kCEVaropVar | kCERefOp)

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthOpcodeCompiler
// 

ForthOpcodeCompiler::ForthOpcodeCompiler(ForthMemorySection*	pDictionarySection)
: mpDictionarySection( pDictionarySection )
, mCompileComboOpFlags(ENABLED_COMBO_OPS)
{
	for ( uint32_t i = 0; i < MAX_PEEPHOLE_PTRS; ++i )
	{
		mPeephole[i] = NULL;
	}
	Reset();
}

ForthOpcodeCompiler::~ForthOpcodeCompiler()
{
} 

void ForthOpcodeCompiler::Reset()
{
	mPeepholeIndex = 0;
	mPeepholeValidCount = 0;
	mpLastIntoOpcode = NULL;
}

#ifdef ASM_INNER_INTERPRETER
#define NATIVE_OPTYPE kOpNative
#else
#define NATIVE_OPTYPE kOpCCode
#endif

#define FITS_IN_BITS( VAL, NBITS )  ((VAL) < (1 << NBITS))
// VAL must already have high 8 bits zeroed
#define FITS_IN_SIGNED_BITS( VAL, NBITS )  (((VAL) < (1 << (NBITS - 1))) || ((VAL) > (((1 << 24) - (1 << NBITS)) + 1)))

void ForthOpcodeCompiler::CompileOpcode( forthOpType opType, forthop opVal )
{
    forthop* pOpcode = mpDictionarySection->pCurrent;
	forthop op = COMPILED_OP( opType, opVal );
    forthOpType previousOpType = kOpUserDef;
    forthop previousOpVal = 0;
	
    forthop newOpVal = opVal & OPCODE_VALUE_MASK;
    switch( opType )
	{
	case NATIVE_OPTYPE:
		{
            forthop lastOp = op;

			if (((mCompileComboOpFlags & kCERefOp) != 0)
                && GetPreviousOpcode(previousOpType, previousOpVal)
                && ((previousOpType == kOpMemberRef) || (previousOpType == kOpLocalRef))
                && FITS_IN_BITS(previousOpVal, 12) && FITS_IN_BITS(newOpVal, 12) )
			{
                // LOCALREF OP combo - bits 0:11 are frame offset, bits 12:23 are opcode
                // MEMBERREF OP combo - bits 0:11 are member offset, bits 12:23 are opcode
                UncompileLastOpcode();
                pOpcode--;
                if (previousOpType == kOpMemberRef)
                {
                    op = COMPILED_OP(kOpMemberRefOpCombo, previousOpVal | (newOpVal << 12));
                }
                else
                {
                    op = COMPILED_OP(kOpLocalRefOpCombo, previousOpVal | (newOpVal << 12));
                }
                //printf("compiling 0x%x @ %p  previousOpType %0x\n", op, pOpcode, previousOpType);
            }

            if ((lastOp == gCompiledOps[OP_INTO]) || (lastOp == gCompiledOps[OP_INTO_PLUS]))
            {
                // we need this to support initialization of local string vars (ugh)
                mpLastIntoOpcode = pOpcode;
            }
        }
		break;

	case kOpBranchZ:
		{
			if (((mCompileComboOpFlags & kCEOpBranch) != 0)
                && GetPreviousOpcode( previousOpType, previousOpVal )
                && (previousOpType == NATIVE_OPTYPE)
				&& FITS_IN_BITS(previousOpVal, 12) && FITS_IN_SIGNED_BITS(newOpVal, 12) )
			{
				// OP ZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
				UncompileLastOpcode();
                pOpcode--;
				op = COMPILED_OP(kOpOZBCombo, previousOpVal | (newOpVal << 12));
			}
		}
		break;

    case kOpBranchNZ:
    {
        if (((mCompileComboOpFlags & kCEOpBranch) != 0)
            && GetPreviousOpcode(previousOpType, previousOpVal)
            && (previousOpType == NATIVE_OPTYPE)
            && FITS_IN_BITS(previousOpVal, 12) && FITS_IN_SIGNED_BITS(newOpVal, 12))
        {
            // OP NZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
            UncompileLastOpcode();
            pOpcode--;
            op = COMPILED_OP(kOpONZBCombo, previousOpVal | (newOpVal << 12));
        }
    }
    break;

	default:
        if (((mCompileComboOpFlags & kCEVaropVar) != 0)
            && GetPreviousOpcode(previousOpType, previousOpVal)
            && (previousOpType == NATIVE_OPTYPE)
            && ((opType >= kOpLocalByte) && (opType <= kOpMemberObject))
            && FITS_IN_BITS(newOpVal, 21))
        {
            forthop previousOp = COMPILED_OP(previousOpType, previousOpVal);
            if ((previousOp >= gCompiledOps[OP_FETCH]) && (previousOp <= gCompiledOps[OP_OCLEAR]))
            {
                bool isLocal = (opType >= kOpLocalByte) && (opType <= kOpLocalObject);
                bool isField = (opType >= kOpFieldByte) && (opType <= kOpFieldObject);
                bool isMember = (opType >= kOpMemberByte) && (opType <= kOpMemberObject);
                if (isLocal || isField || isMember)
                {
                    UncompileLastOpcode();
                    pOpcode--;
                    if ((previousOp == gCompiledOps[OP_REF]) && (isLocal || isMember))
                    {
                        op = COMPILED_OP(isLocal ? kOpLocalRef : kOpMemberRef, opVal);
                    }
                    else
                    {
                        forthop varOpBits = (previousOp - (gCompiledOps[OP_FETCH] - 1)) << 21;
                        op = COMPILED_OP(opType, varOpBits | opVal);
                    }
                    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x\n", op, pOpcode);
                }
            }
        }
        break;
	}

	mPeepholeIndex = (mPeepholeIndex + 1) & PEEPHOLE_PTR_MASK;
	mPeephole[mPeepholeIndex] = pOpcode;
	*pOpcode++ = op;
	mpDictionarySection->pCurrent = pOpcode;
	mPeepholeValidCount++;
}

void ForthOpcodeCompiler::PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode)
{
    if ((opType == kOpBranchZ) || (opType == kOpBranchNZ))
    {
        forthop oldOpcode = *pOpcode;
        forthOpType oldOpType = FORTH_OP_TYPE(oldOpcode);
        forthop oldOpVal = FORTH_OP_VALUE(oldOpcode);
        switch (oldOpType)
        {
        case kOpBranchZ:
        case kOpBranchNZ:
            *pOpcode = COMPILED_OP(opType, opVal);
            break;

        case kOpOZBCombo:
        case kOpONZBCombo:
            // preserve opcode part of old value
            oldOpVal &= 0xFFF;
            if (opType == kOpBranchZ)
            {
                *pOpcode = COMPILED_OP(kOpOZBCombo, oldOpVal | (opVal << 12));
            }
            else
            {
                *pOpcode = COMPILED_OP(kOpONZBCombo, oldOpVal | (opVal << 12));
            }
            break;

        default:
            // TODO - report error
            break;
        }
    }
    else
    {
        // TODO: report error
    }
}

void ForthOpcodeCompiler::UncompileLastOpcode()
{
	if (mPeepholeValidCount > 0)
	{
		if ( mPeephole[mPeepholeIndex] <= mpLastIntoOpcode )
		{
			mpLastIntoOpcode = NULL;

		}
        SPEW_COMPILATION("Uncompiling: move DP back from 0x%08x to 0x%08x\n", mpDictionarySection->pCurrent, mPeephole[mPeepholeIndex]);
        mpDictionarySection->pCurrent = mPeephole[mPeepholeIndex];
		mPeepholeIndex = (mPeepholeIndex - 1) & PEEPHOLE_PTR_MASK;
		mPeepholeValidCount--;
	}
}

uint32_t ForthOpcodeCompiler::PeepholeValidCount()
{
	return (mPeepholeValidCount > MAX_PEEPHOLE_PTRS) ? MAX_PEEPHOLE_PTRS : mPeepholeValidCount;
}

void ForthOpcodeCompiler::ClearPeephole()
{
	Reset();
}

forthop* ForthOpcodeCompiler::GetLastCompiledOpcodePtr( void )
{
	return (mPeepholeValidCount > 0) ? mPeephole[mPeepholeIndex] : NULL;
}

forthop* ForthOpcodeCompiler::GetLastCompiledIntoPtr( void )
{
	return mpLastIntoOpcode;
}

bool ForthOpcodeCompiler::GetPreviousOpcode( forthOpType& opType, forthop& opVal, int index )
{
    // index of 0 means most recently opcode
	if ( mPeepholeValidCount > index )
	{
		forthop op = *(mPeephole[(mPeepholeIndex - index) & PEEPHOLE_PTR_MASK]);

		opType = FORTH_OP_TYPE( op );
		opVal = FORTH_OP_VALUE( op );
		return true;
	}
	return false;
}


