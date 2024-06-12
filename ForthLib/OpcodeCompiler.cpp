//////////////////////////////////////////////////////////////////////
//
// OpcodeCompiler.cpp: implementation of the OpcodeCompiler class.
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

#if defined(LINUX) || defined(MACOSX)
#include <ctype.h>
#endif
#include "Engine.h"
#include "OpcodeCompiler.h"

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
#define ENABLED_COMBO_OPS  (kCEOpBranch | kCEVaropVar | kCERefOp | kCENumOp)

//////////////////////////////////////////////////////////////////////
////
///
//                     OpcodeCompiler
// 

OpcodeCompiler::OpcodeCompiler(MemorySection*	pDictionarySection)
: mpDictionarySection( pDictionarySection )
, mCompileComboOpFlags(ENABLED_COMBO_OPS)
{
	for ( uint32_t i = 0; i < MAX_PEEPHOLE_PTRS; ++i )
	{
		mPeephole[i] = NULL;
	}
	Reset();
}

OpcodeCompiler::~OpcodeCompiler()
{
} 

void OpcodeCompiler::Reset()
{
	mPeepholeIndex = 0;
	mPeepholeValidCount = 0;
	mpLastIntoOpcode = NULL;
}

#define FITS_IN_BITS( VAL, NBITS )  ((VAL) < (1 << NBITS))
// VAL must already have high 8 bits zeroed
#define FITS_IN_SIGNED_BITS( VAL, NBITS )  (((VAL) < (1 << (NBITS - 1))) || ((VAL) > (((1 << 24) - (1 << NBITS)) + 1)))

void OpcodeCompiler::CompileOpcode( forthOpType opType, forthop opVal )
{
    forthop* pOpcode = mpDictionarySection->pCurrent;
    forthop* pPreviousOpcode = GetPreviousOpcodeAddress();
    int previousOpcodeSize = pPreviousOpcode == nullptr ? 0 : pOpcode - pPreviousOpcode;
	forthop op = COMPILED_OP( opType, opVal );
    forthop savedOp = op;
    forthOpType previousOpType = kOpUserDef;
    forthop previousOpVal = 0;
    bool bClearPeephole = false;

    forthop newOpVal = opVal & OPCODE_VALUE_MASK;
    switch( opType )
	{
    case kOpNative:
    case kOpCCode:
    {
            if (GetPreviousOpcode(previousOpType, previousOpVal))
            {
                if (opType == NATIVE_OPTYPE && (previousOpType == kOpMemberRef || previousOpType == kOpLocalRef))
                {
                    if ((mCompileComboOpFlags & kCERefOp) != 0
                        && FITS_IN_BITS(previousOpVal, 12) && FITS_IN_BITS(newOpVal, 12))
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
                }
                else if ((mCompileComboOpFlags & kCENumOp) != 0
                        && op != gCompiledOps[OP_INTO])     // don't combine 'NNN ->', it breaks var initialization
                {
                    if (opType == kOpNative)
                    {
                        // if previous opcode was a literal value opcode, stuff the kOpNativeXXX opcode in its place with the new op,
                        if (previousOpType == kOpConstant)
                        {
                            UncompileLastOpcode();
                            op = *--pOpcode & 0xFFFFFF;
                            if ((op & 0x800000) != 0)
                            {
                                op |= 0xFF000000;
                            }
                            *pOpcode++ = COMPILED_OP(kOpNativeS32, opVal);
                            bClearPeephole = true;
                            SPEW_COMPILATION("Replaced int32 compact literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);

                        }
                        else if (previousOpType == NATIVE_OPTYPE)
                        {
                            // TODO! these assume that opVal(gCompiledOps[OP_XXX_VAL]) == OP_XXX_VAL
                            if (previousOpVal == OP_INT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpNativeS32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced int32 literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_UINT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpNativeU32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced uint32 literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_FLOAT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpNativeF32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced float literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_DOUBLE_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 3;
                                *pOpcode = COMPILED_OP(kOpNativeF64, opVal);
                                pOpcode += 2;
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced double literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-2], pOpcode - 2);
                            }
                            else if (previousOpVal == OP_LONG_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 3;
                                *pOpcode = COMPILED_OP(kOpNativeS64, opVal);
                                pOpcode += 2;
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced int64 literal with native combo 0x%08x @ 0x%08x\n", pOpcode[-2], pOpcode - 2);
                            }
                        }
                    }
                    else if (opType == kOpCCode)
                    {
                        // uncompile the literal value opcode, stuff the kOpCCodeXXX opcode in its place with the new op,
                        if (previousOpType == kOpConstant)
                        {
                            UncompileLastOpcode();
                            op = *--pOpcode & 0xFFFFFF;
                            if ((op & 0x800000) != 0)
                            {
                                op |= 0xFF000000;
                            }
                            *pOpcode++ = COMPILED_OP(kOpCCodeS32, opVal);
                            bClearPeephole = true;
                            SPEW_COMPILATION("Replaced int32 compact literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);

                        }
                        else if (previousOpType == NATIVE_OPTYPE)
                        {
                            if (previousOpVal == OP_INT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpCCodeS32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced int32 literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_UINT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpCCodeU32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced uint32 literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_FLOAT_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 2;
                                *pOpcode++ = COMPILED_OP(kOpCCodeF32, opVal);
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced float literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-1], pOpcode - 1);
                            }
                            else if (previousOpVal == OP_DOUBLE_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 3;
                                *pOpcode = COMPILED_OP(kOpCCodeF64, opVal);
                                pOpcode += 2;
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced double literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-2], pOpcode - 2);
                            }
                            else if (previousOpVal == OP_LONG_VAL)
                            {
                                UncompileLastOpcode();
                                pOpcode -= 3;
                                *pOpcode = COMPILED_OP(kOpCCodeS64, opVal);
                                pOpcode += 2;
                                op = *pOpcode;
                                bClearPeephole = true;
                                SPEW_COMPILATION("Replaced int64 literal with ccode combo 0x%08x @ 0x%08x\n", pOpcode[-2], pOpcode - 2);
                            }
                        }
                    }
                    else if (opType == kOpUserDef)
                    {
                        // TODO - is this even a good idea?  we don't have control of
                        //  what user defs are going to do, if a user is compiling some
                        //  new type of control structure, this could fail horribly
                    }
                }
            }

            if ((savedOp == gCompiledOps[OP_INTO]) || (savedOp == gCompiledOps[OP_INTO_PLUS]))
            {
                // we need this to support initialization of vars (ugh)
                mpLastIntoOpcode = pOpcode;
            }
        }
		break;

	case kOpBranchZ:
		{
			if (((mCompileComboOpFlags & kCEOpBranch) != 0)
                && GetPreviousOpcode( previousOpType, previousOpVal )
                && previousOpcodeSize == 1      // only combine with simple opcodes with no immediate data
                && (previousOpType == NATIVE_OPTYPE)
				&& FITS_IN_BITS(previousOpVal, 12) && FITS_IN_SIGNED_BITS(newOpVal, 12) )
			{
				// OP ZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
				UncompileLastOpcode();
                pOpcode--;
				op = COMPILED_OP(kOpOZBCombo, previousOpVal | (newOpVal << 12));
                SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x   opZBranchCombo\n", op, pOpcode);
                bClearPeephole = true;
            }
		}
		break;

    case kOpBranchNZ:
    {
        if (((mCompileComboOpFlags & kCEOpBranch) != 0)
            && GetPreviousOpcode(previousOpType, previousOpVal)
            && previousOpcodeSize == 1      // only combine with simple opcodes with no immediate data
            && (previousOpType == NATIVE_OPTYPE)
            && FITS_IN_BITS(previousOpVal, 12) && FITS_IN_SIGNED_BITS(newOpVal, 12))
        {
            // OP NZBRANCH combo - bits 0:11 are opcode, bits 12:23 are signed integer branch offset in longs
            UncompileLastOpcode();
            pOpcode--;
            op = COMPILED_OP(kOpONZBCombo, previousOpVal | (newOpVal << 12));
            SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x   opNZBranchCombo\n", op, pOpcode);
            bClearPeephole = true;
        }
    }
    break;

	default:
        if (((mCompileComboOpFlags & kCEVaropVar) != 0)
            && GetPreviousOpcode(previousOpType, previousOpVal)
            && (previousOpType == NATIVE_OPTYPE)
            && ((opType >= kOpLocalByte) && (opType <= kOpMemberObject))
            && FITS_IN_BITS(newOpVal, 20))
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
                        forthop varOpBits = (previousOp - (gCompiledOps[OP_FETCH] - 1)) << 20;
                        op = COMPILED_OP(opType, varOpBits | opVal);
                    }
                    SPEW_COMPILATION("Compiling 0x%08x @ 0x%08x   varop\n", op, pOpcode);
                    bClearPeephole = true;
                }
            }
        }
        break;
	}

    if (bClearPeephole)
    {
        // the just compiled op has made the peephole invalid
        ClearPeephole();
        *pOpcode++ = op;
    }
    else
    {
        mPeepholeIndex = (mPeepholeIndex + 1) & PEEPHOLE_PTR_MASK;
        mPeephole[mPeepholeIndex] = pOpcode;
        *pOpcode++ = op;
        mPeepholeValidCount++;
    }
    mpDictionarySection->pCurrent = pOpcode;
}

void OpcodeCompiler::PatchOpcode(forthOpType opType, forthop opVal, forthop* pOpcode)
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
    else if (opType == kOpBranch)
    {
        *pOpcode = COMPILED_OP(opType, opVal);
    }
    else
    {
        // TODO: report error
    }
}

void OpcodeCompiler::UncompileLastOpcode()
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

uint32_t OpcodeCompiler::PeepholeValidCount()
{
	return (mPeepholeValidCount > MAX_PEEPHOLE_PTRS) ? MAX_PEEPHOLE_PTRS : mPeepholeValidCount;
}

void OpcodeCompiler::ClearPeephole()
{
	Reset();
}

forthop* OpcodeCompiler::GetLastCompiledOpcodePtr( void )
{
	return (mPeepholeValidCount > 0) ? mPeephole[mPeepholeIndex] : NULL;
}

forthop* OpcodeCompiler::GetLastCompiledIntoPtr( void )
{
	return mpLastIntoOpcode;
}

bool OpcodeCompiler::GetPreviousOpcode( forthOpType& opType, forthop& opVal, uint32_t index )
{
    // index of 0 means most recent opcode
	if ( mPeepholeValidCount > index )
	{
		forthop op = *(mPeephole[(mPeepholeIndex - index) & PEEPHOLE_PTR_MASK]);

		opType = FORTH_OP_TYPE( op );
		opVal = FORTH_OP_VALUE( op );
		return true;
	}
	return false;
}

forthop* OpcodeCompiler::GetPreviousOpcodeAddress(uint32_t index)
{
    // index of 0 means most recent opcode
    if (mPeepholeValidCount > index)
    {
        return mPeephole[(mPeepholeIndex - index) & PEEPHOLE_PTR_MASK];
    }

    return nullptr;
}

