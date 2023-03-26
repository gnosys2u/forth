//////////////////////////////////////////////////////////////////////
//
// ForthOps.cpp: forth builtin operator definitions
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <math.h>

#ifdef ARM9
#include <nds.h>
#endif

#include <ctype.h>
#include <time.h>
#if defined(WINDOWS_BUILD)
#include <sys/timeb.h>
#endif

#if defined(MACOSX)
#include <unistd.h>
#endif

#include "Forth.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "Thread.h"
#include "Vocabulary.h"
#include "Shell.h"
#include "InputStream.h"
#include "InputStack.h"
#include "BlockInputStream.h"
#include "BufferInputStream.h"
#include "ConsoleInputStream.h"
#include "FileInputStream.h"
#include "StructVocabulary.h"
#include "Server.h"
#include "Client.h"
#include "ForthMessages.h"
#include "ForthPortability.h"
#include "Object.h"
#include "BlockFileManager.h"
#include "ShowContext.h"
#include "ObjectReader.h"
#include "BuiltinClasses.h"
#include "TypesManager.h"
#include "LocalVocabulary.h"
#include "ClassVocabulary.h"
#include "NativeType.h"
#include "DLLVocabulary.h"

#if defined(LINUX) || defined(MACOSX)
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

extern int kbhit(void);
extern int getch(void);
#endif

forthop gCompiledOps[NUM_COMPILED_OPS];

extern "C"
{

extern float sinf(float a);
extern float cosf(float a);
extern float tanf(float a);
extern float asinf(float a);
extern float acosf(float a);
extern float atanf(float a);
extern float ceilf(float a);
extern float floorf(float a);
extern float sqrtf(float a);
extern float expf(float a);
extern float logf(float a);
extern float log10f(float a);
extern float atan2f(float a, float b);
extern float ldexpf(float a, int b);
extern float frexpf(float a, int* b);
extern float modff(float a, float* b);
extern float fmodf(float a, float b);

// compiled token is 32-bits,
// top 8 bits are opcode type (opType)
// bottom 24 bits are immediate operand

extern void intVarAction(CoreState* pCore, int* pVar);
extern void longVarAction(CoreState* pCore, int* pVar);
extern GFORTHOP( byteVarActionBop );
extern GFORTHOP( ubyteVarActionBop );
extern GFORTHOP( shortVarActionBop );
extern GFORTHOP( ushortVarActionBop );
extern GFORTHOP( intVarActionBop );
#ifdef FORTH64
extern GFORTHOP(uintVarActionBop);
#endif
extern GFORTHOP( longVarActionBop );
extern GFORTHOP( floatVarActionBop );
extern GFORTHOP( doubleVarActionBop );
extern GFORTHOP( stringVarActionBop );
extern GFORTHOP( opVarActionBop );
extern GFORTHOP( objectVarActionBop );

#if defined(WINDOWS_BUILD) && !defined(MINGW)
// this madness is here to fix an unexplained link error - single precision float math functions are undefined
//  even though they are declared extern in InnerInterp.asm
float maximumBogosity()
{
	float aa;
	int bb;
	return asinf(sinf(0.5f)) + acosf(cosf(0.5f)) + atanf(tanf(0.5f))
		+ atan2f(0.5f, 0.7f) + powf(2.0f, 4.0f) + ldexpf(2.0f, 5) + frexpf(2.0f, &bb)
		+ ceilf(modff(0.7f, &aa)) + floorf(fmodf(1.0f, 2.0f)) + log10f(sqrtf(expf(logf(2.0f))));
}
#endif

//############################################################################
//
//   normal forth ops
//
//############################################################################

// bye is a user command which causes the entire forth program to exit
FORTHOP( byeOp )
{
    SET_STATE( OpResult::kExitShell );
}

// badOp is used to detect executing uninitialized op variables
FORTHOP( badOpOp )
{
    SET_ERROR( ForthError::kBadOpcode );
}

// unimplementedMethodOp is used to detect executing unimplemented methods
FORTHOP( unimplementedMethodOp )
{
    SET_ERROR( ForthError::kUnimplementedMethod );
    METHOD_RETURN;
}

// unimplementedMethodOp is used to detect executing unimplemented methods
FORTHOP( illegalMethodOp )
{
    SET_ERROR( ForthError::kUnimplementedMethod );
}

FORTHOP( argvOp )
{
    NEEDS( 1 );
    cell argNum = SPOP;
    SPUSH( (cell) (GET_ENGINE->GetShell()->GetArg( argNum )) );
}

FORTHOP( argcOp )
{
    Shell* pShell = GET_ENGINE->GetShell();
    SPUSH( pShell->GetCommandLineArgs().size() );
}

//##############################
//
// math ops
//

FORTHOP( ldivideOp )
{
#if defined(FORTH64)
    cell b = SPOP;
    cell a = SPOP;
    cell quotient = a / b;
    SPUSH(quotient);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 quotient;
    LPOP(b);
    LPOP(a);
    quotient.s64 = a.s64 / b.s64;
    LPUSH(quotient);
#endif
}

FORTHOP( lmodOp )
{
#if defined(FORTH64)
    cell b = SPOP;
    cell a = SPOP;
    cell remainder = a % b;
    SPUSH(remainder);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 remainder;
    LPOP( b );
    LPOP( a );
    remainder.s64 = a.s64 % b.s64;
    LPUSH( remainder );
#endif
}

FORTHOP( ldivmodOp )
{
#if defined(FORTH64)
    cell b = SPOP;
    cell a = SPOP;
    cell remainder = a % b;
    SPUSH(remainder);
    cell quotient = a / b;
    SPUSH(quotient);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 quotient;
    stackInt64 remainder;
    LPOP(b);
    LPOP(a);
    remainder.s64 = a.s64 % b.s64;
    LPUSH(remainder);
    quotient.s64 = a.s64 / b.s64;
    LPUSH(quotient);
#endif
}

#ifdef RASPI
FORTHOP( timesDivOp )
{
	int denom = SPOP;
	int a = SPOP;
	int b = SPOP;
	int64_t product = (int64_t) a * (int64_t) b;
	int result = product / denom;
	SPUSH(result);
}

FORTHOP( timesDivModOp )
{
	int denom = SPOP;
	int a = SPOP;
	int b = SPOP;
	int64_t product = (int64_t) a * (int64_t) b;
	int quotient = product / denom;
	int remainder = product % denom;
	SPUSH(remainder);
	SPUSH(quotient);
}

FORTHOP( umDivModOp )
{
    stackInt64 a;
    uint32_t denom = SPOP;
    //LPOP(a);
    a.s32[1] = SPOP;
    a.s32[0] = SPOP;
	uint32_t quotient = a.u64 / denom;
	uint32_t remainder = a.u64 % denom;
	SPUSH(remainder);
	SPUSH(quotient);
}

FORTHOP( smRemOp )
{
    stackInt64 a;
    int denom = SPOP;
    //LPOP(a);
    a.s32[1] = SPOP;
    a.s32[0] = SPOP;
	int quotient = a.s64 / denom;
	int remainder = a.s64 % denom;
	SPUSH(remainder);
	SPUSH(quotient);
}
#endif

#if !defined(FORTH64)
//##############################
//
// 64-bit integer comparison ops
//

#ifndef ASM_INNER_INTERPRETER
FORTHOP(lEqualsBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    SPUSH((a.s64 == b.s64) ? -1L : 0);
}

FORTHOP(lNotEqualsBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    SPUSH((a.s64 != b.s64) ? -1L : 0);
}

FORTHOP(lGreaterThanBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 > b.s64 ) ? -1L : 0 );
}

FORTHOP(lGreaterEqualsBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 >= b.s64 ) ? -1L : 0 );
}

FORTHOP(lLessThanBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 < b.s64 ) ? -1L : 0 );
}

FORTHOP(lLessEqualsBop)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP( b );
    LPOP( a );
    SPUSH( ( a.s64 <= b.s64 ) ? -1L : 0 );
}

FORTHOP(lEquals0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 == 0L ) ? -1L : 0 );
}

FORTHOP(lNotEquals0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 != 0L ) ? -1L : 0 );
}

FORTHOP(lGreaterThan0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 > 0L ) ? -1L : 0 );
}

FORTHOP(lGreaterEquals0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 >= 0L ) ? -1L : 0 );
}

FORTHOP(lLessThan0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 < 0L ) ? -1L : 0 );
}

FORTHOP(lLessEquals0Bop)
{
    NEEDS(2);
    stackInt64 a;
    LPOP( a );
    SPUSH( ( a.s64 <= 0L ) ? -1L : 0 );
}

#endif


FORTHOP(lWithinOp)
{
    NEEDS(3);
    stackInt64 hiLimit;
    stackInt64 loLimit;
    stackInt64 val;
    LPOP(hiLimit);
    LPOP(loLimit);
    LPOP(val);
    SPUSH(((loLimit.s64 <= val.s64) && (val.s64 < hiLimit.s64)) ? -1L : 0);
}

FORTHOP(lMinOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    LPUSH(((a.s64 < b.s64) ? a : b));
}

FORTHOP(lMaxOp)
{
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    LPUSH(((a.s64 > b.s64) ? a : b));
}
#endif


FORTHOP( i2lOp )
{
#if defined(FORTH64)
    int a = SPOP;
    SPUSH(a);
#else
    NEEDS(1);
    int a = SPOP;
    stackInt64 b;
    b.s64 = (int64_t) a;
    LPUSH( b );
#endif
}

FORTHOP( l2fOp )
{
#if defined(FORTH64)
    cell a = SPOP;
    float b = (float)a;
    FPUSH(b);
#else
    NEEDS(1);
    stackInt64 a;
    LPOP( a );
    float b = (float) a.s64;
    FPUSH( b );
#endif
}

FORTHOP( l2dOp )
{
#if defined(FORTH64)
    cell a = SPOP;
    double b = (double)a;
    DPUSH(b);
#else
    NEEDS(1);
    stackInt64 a;
    LPOP( a );
    double b = (double) a.s64;
    DPUSH( b );
#endif
}

FORTHOP( f2lOp )
{
#if defined(FORTH64)
    float a = FPOP;
    cell b = a;
    SPUSH(b);
#else
    NEEDS(1);
    float a = FPOP;
    stackInt64 b;
    b.s64 = (int64_t) a;
    LPUSH( b );
#endif
}

FORTHOP( d2lOp )
{
#if defined(FORTH64)
    double a = DPOP;
    cell b = a;
    SPUSH(b);
#else
    NEEDS(2);
    double a = DPOP;
    stackInt64 b;
    b.s64 = (int64_t) a;
    LPUSH( b );
#endif
}

//##############################
//
// control ops
//
// TODO: replace branch, tbranch, fbranch with immediate ops

// has precedence!
FORTHOP(doOp)
{
    NEEDS(2);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_DO ] );
    // save address for loop/+loop
    pShellStack->PushAddress( GET_DP );
    pShellStack->PushTag( kShellTagDo );
    // this will be fixed by loop/+loop
    pOuter->CompileDummyOpcode();
    pOuter->CompileInt( 0 );
    pOuter->StartLoopContinuations();
    pOuter->ClearPeephole();
}

// has precedence!
FORTHOP(checkDoOp)
{
    NEEDS(2);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthShellStack *pShellStack = pEngine->GetShell()->GetShellStack();
    pShellStack->Push( gCompiledOps[ OP_DO_CHECKDO ] );
    // save address for loop/+loop
    pShellStack->PushAddress(GET_DP);
    pShellStack->PushTag( kShellTagDo );
    // this will be fixed by loop/+loop
    pOuter->CompileDummyOpcode();
    pOuter->CompileInt( 0 );
    pOuter->StartLoopContinuations();
    pOuter->ClearPeephole();
}

// has precedence!
FORTHOP(loopOp)
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "loop", pShellStack->PopTag(), kShellTagDo ) )
    {
        return;
    }
    forthop* pDoOp = pShellStack->PopAddress();
	forthop doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pOuter->CompileBuiltinOpcode( OP_DO_LOOP );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
    pOuter->EndLoopContinuations(kShellTagDo);
    pOuter->ClearPeephole();
}

// has precedence!
FORTHOP(loopNOp)
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "+loop", pShellStack->PopTag(), kShellTagDo ) )
    {
        return;
    }
    forthop*pDoOp = pShellStack->PopAddress();
	forthop doOpcode = pShellStack->Pop();
    *pDoOp++ = doOpcode;
    // compile the "_loop" opcode
    pOuter->CompileBuiltinOpcode( OP_DO_LOOPN );
    // fill in the branch to after loop opcode
    *pDoOp = COMPILED_OP( kOpBranch, (GET_DP - pDoOp) - 1 );
    pOuter->EndLoopContinuations(kShellTagDo);
    pOuter->ClearPeephole();
}

// if - has precedence
FORTHOP( ifOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // this will be fixed by else/endif
    pOuter->CompileOpcode(kOpBranchZ, 0);
    forthop* branchAddr = (GET_DP - 1);
    // save address for else/endif
    pShellStack->PushAddress(branchAddr);
    // flag that this is the "if" branch
    pShellStack->PushTag(kShellTagIf);
    pOuter->ClearPeephole();
}

// ]if - has precedence
FORTHOP(elifOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // this will be fixed by else/endif
    pOuter->CompileOpcode(kOpBranchZ, 0);
    forthop* branchAddr = (GET_DP - 1);
    // save address for else/endif
    pShellStack->PushAddress(branchAddr);
    // flag that this is the "if" branch
    pShellStack->PushTag(kShellTagElif);
    pOuter->ClearPeephole();
}

// orif - has precedence
FORTHOP(orifOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // this will be fixed by else/endif
    pOuter->CompileOpcode(kOpBranchZ, 0);
    forthop* branchAddr = (GET_DP - 1);
    // save address for else/endif
    pShellStack->PushAddress(branchAddr);
    // flag that this is an "orif" clause
    pShellStack->PushTag(kShellTagOrIf);
    pOuter->ClearPeephole();
}

// andif - has precedence
FORTHOP(andifOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
	ForthShellStack *pShellStack = pShell->GetShellStack();
    // this will be fixed by else/endif
    pOuter->CompileOpcode(kOpBranchZ, 0);
    forthop* branchAddr = (GET_DP - 1);
    // save address for else/endif
    pShellStack->PushAddress(branchAddr);
    // flag that this is an "andif" clause
    pShellStack->PushTag(kShellTagAndIf);
    pOuter->ClearPeephole();
}



// else - has precedence
FORTHOP(elseOp)
{
    NEEDS(2);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    eShellTag branchTag = pShellStack->PeekTag();
    if (!pShell->CheckSyntaxError("else", branchTag, (kShellTagIf | kShellTagElif | kShellTagWhile | kShellTagOrIf | kShellTagAndIf)))
    {
        return;
    }
    forthop* falseIP = GET_DP + 1;
    forthop* trueIP = pShellStack->PeekAddress(1) + 1;
    bool notDone = true;
    bool followedByOr = false;
    while (notDone)
    {
        branchTag = pShellStack->PopTag();
        forthop* pBranch = pShellStack->PopAddress();
        if (followedByOr)
        {
            pOuter->PatchOpcode(kOpBranchNZ, (trueIP - pBranch) - 1, pBranch);
        }
        else
        {
            pOuter->PatchOpcode(kOpBranchZ, (falseIP - pBranch) - 1, pBranch);
        }
        switch (branchTag)
        {
        case kShellTagIf:
        case kShellTagElif:
        case kShellTagWhile:
            notDone = false;
            break;
        case kShellTagOrIf:
            followedByOr = true;
            falseIP = pShellStack->PeekAddress(1) + 1;
            break;
        case kShellTagAndIf:
            followedByOr = false;
            trueIP = pShellStack->PeekAddress(1) + 1;
            break;
        default:
            pShell->CheckSyntaxError("else", branchTag, kShellTagIf);
            return;
        }
    }
    cell elseCount = 1; // assume there is only one else branch
    if (branchTag == kShellTagElif)
    {
        // this isn't first else
        // shell stack at this point: elseTag numberOfElseBranches <N elseBranchAddresses>
        branchTag = pShellStack->PopTag();
        if (!pShell->CheckSyntaxError("else", branchTag, (kShellTagElse)))
        {
            return;
        }
        elseCount = pShellStack->Pop() + 1;
    }
    // save address for endif
    pShellStack->PushAddress(GET_DP);
    pShellStack->Push(elseCount);
    // flag that this is the "else" branch
    pShellStack->PushTag(kShellTagElse);
    // this will be fixed by endif
    pOuter->CompileDummyOpcode();
}


// endif - has precedence
FORTHOP( endifOp )
{
    NEEDS(2);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    eShellTag branchTag = pShellStack->PeekTag();
    if (!pShell->CheckSyntaxError("endif", branchTag, (kShellTagIf | kShellTagElif | kShellTagElse | kShellTagWhile | kShellTagOrIf | kShellTagAndIf)))
	{
		return;
	}
    bool processElseBranches = false;
	if (branchTag == kShellTagElse)
	{
        processElseBranches = true;
	}
    else
    {
        // there was no "else", so process if/andif/orif
        forthop* falseIP = GET_DP;
        forthop* trueIP = ((forthop *)pShellStack->Peek(1)) + 1;
        bool notDone = true;
        bool followedByOr = false;
        while (notDone)
        {
            branchTag = pShellStack->PopTag();
            forthop *pBranch = pShellStack->PopAddress();
            if (followedByOr)
            {
                pOuter->PatchOpcode(kOpBranchNZ, (trueIP - pBranch) - 1, pBranch);
            }
            else
            {
                pOuter->PatchOpcode(kOpBranchZ, (falseIP - pBranch) - 1, pBranch);
            }
            switch (branchTag)
            {
            case kShellTagIf:
            case kShellTagWhile:
                notDone = false;
                break;
            case kShellTagElif:
                notDone = false;
                processElseBranches = true;
                break;
            case kShellTagOrIf:
                followedByOr = true;
                break;
            case kShellTagAndIf:
                followedByOr = false;
                break;
            default:
                //huh ? can this happen ?
                pShell->CheckSyntaxError("else", branchTag, kShellTagIf);
                return;
            }
        }
    }
    if (processElseBranches)
    {
        // "else" has already handled if/andif/orif, just do branch at end of true body around false body
        branchTag = pShellStack->PopTag();
        cell numElseBranches = pShellStack->Pop();
        for (cell i = 0; i < numElseBranches; ++i)
        {
            forthop *pBranch = pShellStack->PopAddress();
            *pBranch = COMPILED_OP(kOpBranch, (GET_DP - pBranch) - 1);
        }
    }
    pOuter->ClearPeephole();
}



// begin - has precedence
FORTHOP( beginOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for repeat/until/again
    pShellStack->PushAddress(GET_DP);
    pShellStack->PushTag( kShellTagBegin );
    pOuter->StartLoopContinuations();
    pOuter->ClearPeephole();
}


// until - has precedence
FORTHOP( untilOp )
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( pShell->CheckSyntaxError( "until", pShellStack->PopTag(), kShellTagBegin ) )
    {
        forthop *pBeginOp = (forthop *) pShellStack->Pop();
        pOuter->CompileOpcode(kOpBranchZ, 0);
        pOuter->PatchOpcode(kOpBranchZ, (pBeginOp - GET_DP), GET_DP - 1);
    }
    pOuter->EndLoopContinuations(kShellTagBegin);
    pOuter->ClearPeephole();
}


// while - has precedence
FORTHOP( whileOp )
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if (!pShell->CheckSyntaxError("while", pShellStack->PopTag(), kShellTagBegin))
    {
        return;
    }
    // stick while tag/address under the begin tag/address
    forthop* oldAddress = pShellStack->PopAddress();
    // this will be fixed by else/endif
    pOuter->CompileOpcode(kOpBranchZ, 0);
    forthop* branchAddr = GET_DP - 1;
    pShellStack->PushAddress(branchAddr);
    pShellStack->PushTag(kShellTagWhile);
    pShellStack->PushAddress(oldAddress);
    pShellStack->PushTag(kShellTagBegin);
    pOuter->ClearPeephole();
}


// repeat - has precedence
FORTHOP( repeatOp )
{
    NEEDS(2);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if (!pShell->CheckSyntaxError("repeat", pShellStack->PopTag(), kShellTagBegin))
    {
        return;
    }
    forthop *pBeginAddress = pShellStack->PopAddress();
    eShellTag branchTag = pShellStack->PopTag();
    if (!pShell->CheckSyntaxError("repeat", branchTag, kShellTagWhile))
    {
        return;
    }
    // fill in the branch taken when "while" fails
    forthop* pBranch =  (forthop*) pShellStack->Pop();
    //*pBranch = COMPILED_OP((branchTag == kShellTagBranchZ) ? kOpBranchZ : kOpBranch, (GET_DP - pBranch));
    pOuter->PatchOpcode(kOpBranchZ, (GET_DP - pBranch), pBranch);
    forthop* branchAddress = GET_DP;
    pOuter->CompileOpcode(kOpBranch, (pBeginAddress - GET_DP) - 1);
    //printf("repeat compiled branch 0x%x @ %p\n", *branchAddress, branchAddress);
    pOuter->EndLoopContinuations(kShellTagBegin);
    pOuter->ClearPeephole();
}

// again - has precedence
FORTHOP( againOp )
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "again", pShellStack->PopTag(), kShellTagBegin ) )
    {
        return;
    }
    forthop*pLoopTop =  (forthop*) pShellStack->Pop();
    pOuter->CompileOpcode( kOpBranch, (pLoopTop - GET_DP) - 1 );
    pOuter->EndLoopContinuations(kShellTagBegin);
    pOuter->ClearPeephole();
}

// case - has precedence
FORTHOP( caseOp )
{
    // leave marker for end of list of case-exit branches for endcase
   Engine *pEngine = GET_ENGINE;
   OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
   Shell *pShell = pEngine->GetShell();
   ForthShellStack *pShellStack = pShell->GetShellStack();
   pShellStack->Push( 0 );
   pShellStack->PushTag( kShellTagCase );
   pOuter->StartLoopContinuations();
   pOuter->ClearPeephole();
}


// of - has precedence
FORTHOP( ofOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    // save address for endof
    pShellStack->PushAddress(GET_DP);
    pShellStack->PushTag( kShellTagOf );
    // this will be set to a caseBranch by endof
    pOuter->CompileDummyOpcode();
}


// ofif - has precedence
FORTHOP( ofifOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();

    // save address for endof
    pShellStack->PushAddress(GET_DP);
    pShellStack->PushTag( kShellTagOfIf );
    // this will be set to a zBranch by endof
    pOuter->CompileDummyOpcode();
	// if the ofif test succeeded, we need to dispose of the switch input value
    pOuter->CompileBuiltinOpcode( OP_DROP );
    pOuter->ClearPeephole();
}


// endof - has precedence
FORTHOP( endofOp )
{
    NEEDS(1);
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    forthop* pDP = GET_DP;

    // this will be fixed by endcase
    pOuter->CompileDummyOpcode();
    bool isLastCase = true;
    forthop* pCaseBody = nullptr;

    while (true)
    {
        eShellTag tag = pShellStack->PopTag();
        if (!pShell->CheckSyntaxError("endof", tag, kShellTagOf | kShellTagOfIf | kShellTagCase))
        {
            return;
        }
        if (tag == kShellTagCase)
        {
            break;
        }
        forthop* pOp = (forthop* )pShellStack->Pop();
        // fill in the branch taken when case doesn't match
        forthop branchOp;
        cell branchOffset;
        forthop* pHere = GET_DP;
        if (isLastCase)
        {
            // last case test compiles a branch around case body on mismatch
            branchOp = (tag == kShellTagOfIf) ? kOpBranchZ : kOpCaseBranchF;
            pCaseBody = pOp + 1;
            branchOffset = (pHere - pOp) - 1;
        }
        else
        {
            // all case tests except last compiles a branch to case body on match
            branchOp = (tag == kShellTagOfIf) ? kOpBranchNZ : kOpCaseBranchT;
            branchOffset = (pCaseBody - pOp) - 1;
        }
        *pOp = COMPILED_OP(branchOp, branchOffset);
        isLastCase = false;
    }
    // save address for endcase
    pShellStack->PushAddress(pDP);
    pShellStack->PushTag( kShellTagCase );
    pOuter->ClearPeephole();
}


// endcase - has precedence
FORTHOP( endcaseOp )
{
    cell* pSP = GET_SP;
    forthop* pEndofOp;

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();
    if ( !pShell->CheckSyntaxError( "endcase", pShellStack->PopTag(), kShellTagCase ) )
    {
        return;
    }

    // compile a "drop" to dispose of the case selector on TOS
    pOuter->CompileBuiltinOpcode(OP_DROP);

    // patch branches from end-of-case to common exit point
    while (true)
    {
        pEndofOp = (forthop* )(pShellStack->Pop());
        if (pEndofOp == nullptr)
        {
            break;
        }
        *pEndofOp = COMPILED_OP(kOpBranch, (GET_DP - pEndofOp) - 1);
    }
    SET_SP( pSP );
    pOuter->EndLoopContinuations(kShellTagCase);
    pOuter->ClearPeephole();
}

FORTHOP(labelOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pHere = GET_DP;
	char* labelName = pOuter->GetNextSimpleToken();
    pOuter->DefineLabel(labelName, pHere);
}

FORTHOP(gotoOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pHere = GET_DP;
	char* labelName = pOuter->GetNextSimpleToken();
	// this will be fixed when label is defined
	pOuter->CompileDummyOpcode();
    pOuter->AddGoto(labelName, kOpBranch, pHere);
}

FORTHOP(gotoIfOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pHere = GET_DP;
	char* labelName = pOuter->GetNextSimpleToken();
	// this will be fixed when label is defined
	pOuter->CompileDummyOpcode();
    pOuter->AddGoto(labelName, kOpBranchNZ, pHere);
}

FORTHOP(gotoIfNotOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pHere = GET_DP;
	char* labelName = pOuter->GetNextSimpleToken();
	// this will be fixed when label is defined
	pOuter->CompileDummyOpcode();
    pOuter->AddGoto(labelName, kOpBranchZ, pHere);
}

FORTHOP(continueDefineOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->SetContinuationDestination(GET_DP);
}

FORTHOP(continueOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddContinuationBranch(GET_DP, kOpBranch);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

FORTHOP(continueIfOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddContinuationBranch(GET_DP, kOpBranchNZ);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

FORTHOP(continueIfNotOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddContinuationBranch(GET_DP, kOpBranchZ);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

FORTHOP(breakOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddBreakBranch(GET_DP, kOpBranch);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

FORTHOP(breakIfOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddBreakBranch(GET_DP, kOpBranchNZ);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

FORTHOP(breakIfNotOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->AddBreakBranch(GET_DP, kOpBranchZ);
    // this will be fixed when surrounding loop is finished
    pOuter->CompileDummyOpcode();
}

// ialign (upwards) DP to int boundary
FORTHOP( ialignOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pEngine->AlignDP();
    pOuter->ClearPeephole();
}

FORTHOP(lalignOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    cell DP = ((cell)pEngine->GetDP() + 7) & ~7;
    pEngine->SetDP((forthop *)DP);
    pOuter->ClearPeephole();
}

FORTHOP( allotOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    cell nBytes = SPOP;
    pEngine->AllotBytes(nBytes);
    pOuter->ClearPeephole();
}

FORTHOP(iCommaOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->CompileInt(SPOP);
    pOuter->ClearPeephole();
}

FORTHOP(lCommaOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
#if defined(FORTH64)
    pOuter->CompileCell(SPOP);
#else
    double d = DPOP;
    pOuter->CompileDouble(d);
#endif
    pOuter->ClearPeephole();
}

FORTHOP(bCommaOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pChar = (char*)GET_DP;
    *pChar++ = (char)SPOP;
    pEngine->SetDP((forthop*)pChar);
    pOuter->ClearPeephole();
}

FORTHOP(sCommaOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    short* pShort = (short*)GET_DP;
    *pShort++ = (short)SPOP;
    pEngine->SetDP((forthop*)pShort);
    pOuter->ClearPeephole();
}

FORTHOP( unusedOp )
{
    MemorySection* pDictionarySection = pCore->pDictionary;
    SPUSH( (4 * (pDictionarySection->len - (pDictionarySection->pCurrent - pDictionarySection->pBase))) );
}

FORTHOP( here0Op )
{
    SPUSH( (cell)(pCore->pDictionary->pBase) );
}

FORTHOP( mallocOp )
{
    NEEDS(1);
    int32_t numBytes = SPOP;
	void* pMemory = __MALLOC(numBytes);
    SPUSH( (cell) pMemory  );
}

FORTHOP( reallocOp )
{
    NEEDS(2);
    size_t newSize = (size_t)(SPOP);
    void* allocPtr = (void *)(SPOP);
    SPUSH(  (cell) __REALLOC( allocPtr, newSize )  );
}

FORTHOP( freeOp )
{
    NEEDS(1);
    __FREE( (void *) SPOP );
}

FORTHOP(initStringArrayOp)
{
	// TOS: maximum length, number of elements, ptr to first char of first element
	int32_t len, nLongs;
	int32_t* pStr;
	int i, numElements;

	len = SPOP;
	numElements = SPOP;
    // TODO!
	pStr = ((int32_t *)(SPOP)) - 2;
	nLongs = (len >> 2) + 3;

	for (i = 0; i < numElements; i++)
	{
		*pStr = len;
		pStr[1] = 0;
		*((char *)(pStr + 2)) = 0;
		pStr += nLongs;
	}
}

FORTHOP(initStructArrayOp)
{
	// TOS: struct index, number of elements, ptr to first struct
	Engine *pEngine = GET_ENGINE;
	TypesManager* pManager = TypesManager::GetInstance();

	cell typeIndex = SPOP;
    cell numElements = SPOP;
	char* pStruct = (char *)(SPOP);
	ForthTypeInfo* typeInfo = pManager->GetTypeInfo(typeIndex);
	if (typeInfo != nullptr)
	{
		StructVocabulary* pVocab = typeInfo->pVocab;
		if (pVocab != nullptr)
		{
			if (pVocab->IsStruct())
			{
				int32_t initStructOp = pVocab->GetInitOpcode();
				if (initStructOp != 0)
				{
					int32_t len = pVocab->GetSize();

					for (int i = 0; i < numElements; i++)
					{
						SPUSH((cell)pStruct);
						pEngine->FullyExecuteOp(pCore, initStructOp);
						pStruct += len;
					}
				}
			}
			else
			{
				// TODO report that vocab isn't a struct
			}
		}
		else
		{
			// TODO report struct has no vocabulary
		}
	}
	else
	{
		// TODO report unknown type for typeIndex
	}
}

//##############################
//
//  exception handling ops
//
FORTHOP(doTryOp)
{
    cell *pNewRP = (cell *)((cell)(GET_RP) - sizeof(ForthExceptionFrame));
    ForthExceptionFrame *pFrame = (ForthExceptionFrame *)pNewRP;
    SET_RP(pNewRP);
    forthop* IP = GET_IP;
    pFrame->pNextFrame = pCore->pExceptionFrame;
    pFrame->pSavedSP = GET_SP;
    pFrame->pSavedFP = GET_FP;
    pFrame->pHandlerOffsets = IP;
    pFrame->exceptionNumber = 0;
    pFrame->exceptionState = ExceptionState::kTry;

    SET_IP(IP + 2);     // skip immediately following handlerIPs
    pCore->pExceptionFrame = pFrame;
}

FORTHOP(tryOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();

    pOuter->CompileBuiltinOpcode(OP_DO_TRY);
    pShellStack->PushAddress(GET_DP);
    pShellStack->PushTag(kShellTagTry);
    pOuter->CompileInt(0);
    pOuter->CompileInt(0);
    pOuter->ClearPeephole();
}

FORTHOP(exceptOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();

    eShellTag tryTag = pShellStack->PopTag();
    if (!pShell->CheckSyntaxError("except", tryTag, kShellTagTry))
    {
        return;
    }
    forthop* pHandlerOffsets = pShellStack->PopAddress();
    pOuter->CompileInt(0);        // this will be a branch to the finally section
    *pHandlerOffsets = GET_DP - pHandlerOffsets;
    pShellStack->PushAddress(pHandlerOffsets);
    pShellStack->PushTag(kShellTagExcept);
    pOuter->ClearPeephole();
}

FORTHOP(doFinallyOp)
{
    if (pCore->pExceptionFrame != nullptr)
    {
        pCore->pExceptionFrame->exceptionState = ExceptionState::kFinally;
    }
    else
    {
        GET_ENGINE->SetError(ForthError::kIllegalOperation, "_doFinally executed when there is no exception frame");
    }
}

FORTHOP(finallyOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();

    eShellTag exceptTag = pShellStack->PopTag();
    if (!pShell->CheckSyntaxError("finally", exceptTag, kShellTagExcept))
    {
        return;
    }
    // compile raise(0) to clear the exception
    pOuter->CompileInt(COMPILED_OP(kOpConstant, 0));
    pOuter->CompileBuiltinOpcode(OP_RAISE);
    forthop* pHandlerOffsets = pShellStack->PopAddress();
    forthop* dp = GET_DP;
    pHandlerOffsets[1] = dp - pHandlerOffsets;
    pOuter->CompileBuiltinOpcode(OP_DO_FINALLY);
    // create branch from end of try section to finally section
    forthop* pBranch = pHandlerOffsets +(pHandlerOffsets[0] - 1);
    *pBranch = COMPILED_OP(kOpBranch, (dp - pBranch) - 1);
    pShellStack->PushTag(kShellTagFinally);
    pOuter->ClearPeephole();
}

FORTHOP(endtryOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    ForthShellStack *pShellStack = pShell->GetShellStack();

    eShellTag tag = pShellStack->PopTag();
    if (!pShell->CheckSyntaxError("endtry", tag, (kShellTagExcept | kShellTagFinally)))
    {
        return;
    }

    if (tag == kShellTagExcept)
    {
        // compile raise(0) to clear the exception
        pOuter->CompileInt(COMPILED_OP(kOpConstant, 0));
        pOuter->CompileBuiltinOpcode(OP_RAISE);
        // set finallyIP to here
        forthop* pHandlerOffsets = pShellStack->PopAddress();
        forthop* dp = GET_DP;
        pHandlerOffsets[1] = dp - pHandlerOffsets;
        // create branch from end of try section to empty finally section
        forthop* pBranch = pHandlerOffsets + (pHandlerOffsets[0] - 1);
        *pBranch = COMPILED_OP(kOpBranch, (dp - pBranch) - 1);
    }
    pOuter->CompileBuiltinOpcode(OP_DO_ENDTRY);
    pOuter->ClearPeephole();
}

FORTHOP(doEndtryOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    if (pCore->pExceptionFrame != nullptr)
    {
        // unwind current exception frame
        SET_FP(pCore->pExceptionFrame->pSavedFP);
        cell oldExceptionNum = pCore->pExceptionFrame->exceptionNumber;
        cell *pNewRP = (cell *)((cell)(GET_RP) + sizeof(ForthExceptionFrame));
        SET_RP(pNewRP);
        pCore->pExceptionFrame = pCore->pExceptionFrame->pNextFrame;
        if (oldExceptionNum != 0)
        {
            pEngine->RaiseException(pCore, oldExceptionNum);
        }
    }
    else
    {
        pEngine->SetError(ForthError::kIllegalOperation, "endtry executed when there is no exception frame");
    }
}

FORTHOP(raiseOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    cell exceptionNum = SPOP;
    pEngine->RaiseException(pCore, exceptionNum);
}

//##############################
//
//  forth defining ops
//

static forthop    *gpSavedDP;

// builds
// - does not have precedence
// - is executed while executing the defining word
// - begins definition of new symbol
//
FORTHOP(buildsOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pEntry;

    // get next symbol, add it to vocabulary with type "builds/does"
    pEngine->AlignDP();
    pOuter->AddUserOp( pOuter->GetNextSimpleToken(), &pEntry, true );
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    // remember current DP (for does)
    gpSavedDP = GET_DP;
    // compile dummy word at DP, will be filled in by does
    pOuter->CompileInt( 0 );
    pOuter->ClearPeephole();
}

// does
// - has precedence
// - is executed while compiling the defining word
// - it completes the definition of the builds part of the word, and starts
//   the definition of the does part of the word
//
FORTHOP( doesOp )
{
    cell newUserOp;
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    if ( pEngine->IsCompiling() )
    {
        // compile dodoes opcode & dummy word
        pOuter->CompileBuiltinOpcode( OP_END_BUILDS );
        pOuter->CompileInt( 0 );
        // create a nameless vocabulary entry for does-body opcode
        newUserOp = pOuter->AddOp( GET_DP );
        newUserOp = COMPILED_OP( kOpUserDef, newUserOp );
        pOuter->CompileBuiltinOpcode( OP_DO_DOES );
        // stuff does-body opcode in dummy word
        GET_DP[-2] = newUserOp;
        // compile local vars allocation op (if needed)
        pOuter->EndOpDefinition();
    }
    else
    {
        // this is a one-off does body which will only be used by
        //  the most recently created op
        newUserOp = pOuter->AddOp( GET_DP );
        *gpSavedDP = COMPILED_OP( kOpUserDef, newUserOp );
        pOuter->CompileBuiltinOpcode( OP_DO_DOES );
        // switch to compile mode
        pEngine->SetCompileState( 1 );
    }
}

// endBuilds
// - does not have precedence
// - is executed while executing the defining word
// - it completes the definition of the new symbol created by builds by compiling
//   the opcode (defined by does) into the newly defined word
//
FORTHOP( endBuildsOp )
{
    // finish current symbol definition (of op defined by builds)
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
    
    // fetch opcode at pIP, compile it into dummy word remembered by builds
    *gpSavedDP = *GET_IP;
    // we are done defining, bail out
    SET_IP( (forthop* ) (RPOP) );
    pOuter->ClearPeephole();
}

// exit has precedence
FORTHOP( exitOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // compile exitOp
    int32_t flags = pOuter->GetFlags();

	bool isMethodDef = ((flags & kEngineFlagIsMethod) != 0);
    pOuter->ClearPeephole();
    if ( pOuter->HasLocalVariables() )
	{
		pOuter->CompileBuiltinOpcode( isMethodDef ? OP_DO_EXIT_ML : OP_DO_EXIT_L );
	}
	else
	{
		pOuter->CompileBuiltinOpcode( isMethodDef ? OP_DO_EXIT_M : OP_DO_EXIT );
	}
    pOuter->ClearPeephole();
}

// semi has precedence
FORTHOP( semiOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
	pOuter->EndOpDefinition( !pOuter->CheckFlag( kEngineFlagNoNameDefinition ) );
    pOuter->ClearFlag( kEngineFlagNoNameDefinition );

	pEngine->GetShell()->CheckDefinitionEnd(":", "coln");
    if (pOuter->HasPendingContinuations())
    {
        pEngine->SetError(ForthError::kBadSyntax, "There are unresolved continue/break branches at end of definition");
    }
}

static void startColonDefinition(CoreState* pCore, const char* pName)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pEntry = pOuter->StartOpDefinition(pName, true);
	pEntry[1] = (forthop)BASE_TYPE_TO_CODE(BaseType::kUserDefinition);
	// switch to compile mode
	pEngine->SetCompileState(1);
	pOuter->ClearFlag(kEngineFlagNoNameDefinition);

	pEngine->GetShell()->StartDefinition(pName, "coln");
}

FORTHOP( colonOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
	const char* pName = pOuter->GetNextSimpleToken();
	startColonDefinition(pCore, pName);
}


FORTHOP( colonNoNameOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
    forthop newDef = pOuter->AddOp( GET_DP );
	newDef = COMPILED_OP(kOpUserDef, newDef);
	SPUSH((cell) newDef);
    // switch to compile mode
    pOuter->SetCompileState( 1 );
    pOuter->SetFlag( kEngineFlagNoNameDefinition );

	pEngine->GetShell()->StartDefinition("", "coln");
    pOuter->ClearPeephole();
}


FORTHOP(pushTokenOp)
{
	char* pName = (char*)(SPOP);
	GET_ENGINE->GetOuterInterpreter()->GetTokenStack()->Push(pName);
}


FORTHOP(popTokenOp)
{
	SPUSH((cell)(GET_ENGINE->GetOuterInterpreter()->GetTokenStack()->Pop()));
}


// func: has precedence
FORTHOP( funcOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
	ForthShellStack *pShellStack = pShell->GetShellStack();

	// if compiling, push DP and compile kOpPushBranch
	// if interpreting, push DP and enter compile mode

	//pShellStack->Push(pOuter->GetFlags() & kEngineFlagNoNameDefinition);
	// save address for ;func
	pShellStack->PushAddress(GET_DP);
	if (pEngine->IsCompiling())
	{
		pOuter->GetLocalVocabulary()->Push();
		// this op will be replaced by ;func
		pOuter->CompileDummyOpcode();
		// flag that previous state was compiling
		pShellStack->Push(1);
	}
	else
	{
		// switch to compile mode
		pEngine->SetCompileState(1);
		// flag that previous state was interpreting
		pShellStack->Push(0);
	}
	// TODO: push hasLocalVars flag?
    //pOuter->ClearFlag( kEngineFlagNoNameDefinition);

	pEngine->GetShell()->StartDefinition("", "func");
    pOuter->ClearPeephole();
}

// ;func has precedence
FORTHOP( endfuncOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
	ForthShellStack *pShellStack = pShell->GetShellStack();

	// compile appropriate __exit opcode
    exitOp( pCore );
    // compile local vars allocation op (if needed)
	pOuter->EndOpDefinition( false );

	pOuter->GetLocalVocabulary()->Pop();
	if (pShell->CheckDefinitionEnd("func", "func"))
	{
		bool wasCompiling = (pShellStack->Pop() != 0);
        forthop* pOldDP = pShellStack->PopAddress();
		if (wasCompiling)
		{
            forthop branchOffset = GET_DP - (pOldDP + 1);
			*pOldDP = branchOffset | (kOpRelativeDefBranch << 24);
		}
		else
		{
			forthop opcode = (pOldDP - pCore->pDictionary->pBase) | (kOpRelativeDef << 24);
			SPUSH((cell)opcode);
			// switch back from compile mode to execute mode
			pEngine->SetCompileState(0);
		}
	}
    pOuter->ClearPeephole();
}

FORTHOP( codeOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
    forthop* pEntry = pOuter->StartOpDefinition( NULL, false );
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    forthop newestOp = *pEntry;
    *pEntry = COMPILED_OP( kOpNative, FORTH_OP_VALUE( newestOp ) );
    pOuter->ClearPeephole();
}

FORTHOP( createOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
    forthop* pEntry = pOuter->StartOpDefinition( NULL, false );
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    // remember current DP (for does)
    gpSavedDP = GET_DP;
    pOuter->CompileBuiltinOpcode( OP_DO_VAR );
    pOuter->ClearPeephole();
}

FORTHOP( forthVocabOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->GetForthVocabulary()->DoOp( pCore );
}

FORTHOP(literalsVocabOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->GetLiteralsVocabulary()->DoOp(pCore);
}

FORTHOP( definitionsOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
	if ( GET_VAR_OPERATION != VarOperation::kVarDefaultOp )
	{
		pOuter->GetDefinitionVocabulary()->DoOp( pCore );
	}
	else
	{
		pOuter->SetDefinitionVocabulary( pVocabStack->GetTop() );
	}
	CLEAR_VAR_OPERATION;
}

FORTHOP( alsoOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
    pVocabStack->DupTop();
}

FORTHOP( previousOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
    if ( !pVocabStack->DropTop() )
    {
        CONSOLE_STRING_OUT( "Attempt to drop last item on vocabulary stack ignored.\n" );
    }
}

FORTHOP( onlyOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
    pVocabStack->Clear();
}

FORTHOP( vocabularyOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary *pDefinitionsVocab = pOuter->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pVocabName = pOuter->GetNextSimpleToken();
    forthop* pEntry = pOuter->StartOpDefinition( pVocabName );
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    pOuter->CompileBuiltinOpcode( OP_DO_VOCAB );
    Vocabulary* pVocab = new Vocabulary( pVocabName,
                                                   NUM_FORTH_VOCAB_VALUE_LONGS,
                                                   512,
                                                   GET_DP,
                                                   Vocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pOuter->CompileCell( (cell) pVocab );
}

FORTHOP( strForgetOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    const char* pSym = (const char *)(SPOP);
	bool verbose = (GET_VAR_OPERATION != VarOperation::kVarDefaultOp);
    bool forgotIt = pOuter->ForgetSymbol( pSym, !verbose );
    // reset search & definitions vocabs in case we deleted a vocab we were using
    pOuter->SetDefinitionVocabulary( pOuter->GetForthVocabulary() );
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
    pVocabStack->Clear();
    SPUSH( forgotIt ? -1 : 0 );
}

#define SCREEN_COLUMNS 120

// return 'q' IFF user quit out
static char
ShowVocab( CoreState   *pCore,
           Vocabulary  *pVocab,
           bool             quick )
{
    int i;
    char retVal = 0;
    Shell *pShell = GET_ENGINE->GetShell();
    int nEntries = pVocab->GetNumEntries();
    forthop* pEntry = pVocab->GetFirstEntry();
#define ENTRY_COLUMNS 16
    char spaces[ENTRY_COLUMNS + 1];
    int column = 0;
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];

    memset( spaces, ' ', ENTRY_COLUMNS );
    spaces[ENTRY_COLUMNS] = '\0';

    bool bDoPauses = true;
    for ( i = 0; i < nEntries; i++ )
    {
        bool lastEntry = (i == (nEntries-1));
        if ( quick )
        {
            int nameLength = pVocab->GetEntryNameLength( pEntry );
            int numSpaces = ENTRY_COLUMNS - (nameLength % ENTRY_COLUMNS);
            if ( (column + nameLength) > SCREEN_COLUMNS )
            {
                CONSOLE_CHAR_OUT( '\n' );
                column = 0;
            }
            pVocab->GetEntryName( pEntry, buff, sizeof(buff) );
            CONSOLE_STRING_OUT( buff );
            column += (nameLength + numSpaces);
            if ( column >= SCREEN_COLUMNS )
            {
                CONSOLE_CHAR_OUT( '\n' );
                column = 0;
            }
            else
            {
                CONSOLE_STRING_OUT( spaces + (ENTRY_COLUMNS - numSpaces) );
            }
            if ( lastEntry )
            {
                CONSOLE_CHAR_OUT( '\n' );
            }
            else
            {
                pEntry = pVocab->NextEntry( pEntry );
            }
        }
        else
        {
            pVocab->PrintEntry( pEntry );
            CONSOLE_CHAR_OUT( '\n' );
            pEntry = pVocab->NextEntry( pEntry );
            if ( (bDoPauses && ((i % 22) == 21)) || lastEntry )
            {
                if ( (pShell != NULL) && pShell->GetInput()->Top()->IsInteractive() )
                {
                    CONSOLE_STRING_OUT( "\nHit ENTER to continue, 'q' & ENTER to quit, 'n' & ENTER to do next vocabulary, 'z' to stop pausing\n" );
                    retVal = tolower( pShell->GetChar() );
                    if ( retVal == 'q' )
                    {
                        pShell->GetChar();
                        break;
                    }
                    else if ( retVal == 'n' )
                    {
                        pShell->GetChar();
                        break;
                    }
                    else if (retVal == 'z')
                    {
                        bDoPauses = false;
                    }
                }
            }
        }
    }

    return retVal;
}


FORTHOP( vlistOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    bool quit = false;
    VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();
    Vocabulary* pVocab;
	bool verbose = (GET_VAR_OPERATION != VarOperation::kVarDefaultOp);
    pOuter->ShowSearchInfo();
	int depth = 0;
	while (!quit)
    {
        pVocab = pVocabStack->GetElement( depth );
        if ( pVocab == NULL )
        {
            return;
        }
        CONSOLE_STRING_OUT( pVocab->GetName() );
        CONSOLE_STRING_OUT( " vocabulary:\n" );
        quit = ( ShowVocab( pCore, pVocab, !verbose ) == 'q' );
        depth++;
    }
	CLEAR_VAR_OPERATION;
}

FORTHOP( verboseBop )
{
    SET_VAR_OPERATION( VarOperation::kNumVarops );
}

FORTHOP( strFindOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary* pFoundVocab;

    char *pSymbol = (char *) (SPOP);
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol( pSymbol, &pFoundVocab );
    SPUSH( ((cell) pEntry) );
}


FORTHOP( variableOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
    forthop* pEntry = pOuter->StartOpDefinition();
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    pOuter->CompileBuiltinOpcode( OP_DO_VAR );
    pOuter->CompileCell( 0 );
}

FORTHOP( iConstantOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pEntry = pOuter->StartOpDefinition();
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    pOuter->CompileBuiltinOpcode( OP_DO_ICONSTANT );
    pOuter->CompileCell( SPOP );
}

FORTHOP( lConstantOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pEntry = pOuter->StartOpDefinition();
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    // TODO: this is wrong for 32-bit!
    pOuter->CompileBuiltinOpcode( OP_DO_LCONSTANT );
    double d = DPOP;
    pOuter->CompileDouble( d );
}

FORTHOP( byteOp )
{
    char val = 0;
	gNativeTypeByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ubyteOp )
{
    char val = 0;
	gNativeTypeUByte.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( shortOp )
{
    short val = 0;
	gNativeTypeShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ushortOp )
{
    short val = 0;
	gNativeTypeUShort.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( intOp )
{
    int val = 0;
	gNativeTypeInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( uintOp )
{
    int val = 0;
	gNativeTypeUInt.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( longOp )
{
    int64_t val = 0;
	gNativeTypeLong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( ulongOp )
{
    int64_t val = 0;
	gNativeTypeULong.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( floatOp )
{
    float val = 0.0;
	gNativeTypeFloat.DefineInstance( GET_ENGINE, &val );
}


FORTHOP( doubleOp )
{
    double val = 0.0;
	gNativeTypeDouble.DefineInstance( GET_ENGINE, &val );
}

FORTHOP( stringOp )
{
    gNativeTypeString.DefineInstance( GET_ENGINE, NULL );
}

FORTHOP( opOp )
{
    int val = gCompiledOps[ OP_BAD_OP] ;
	gNativeTypeOp.DefineInstance( GET_ENGINE, &val );
}

//FORTHOP( objectOp )
//{
//    int32_t val[2] = { 0, 0  };
//	gNativeTypeObject.DefineInstance( GET_ENGINE, val );
//}

FORTHOP( voidOp )
{
}

FORTHOP( arrayOfOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    int32_t numElements;

    if ( pEngine->IsCompiling() )
    {
        // the symbol just before "arrayOf" should have been an integer constant
        if (pOuter->GetLastConstant( numElements ) )
        {
            // uncompile the integer contant opcode
            pOuter->UncompileLastOpcode();
            // save #elements for var declaration ops
            pOuter->SetArraySize( numElements );
        }
        else
        {
            SET_ERROR( ForthError::kMissingSize );
        }
    }
    else
    {
        // save #elements for var declaration ops
        numElements = SPOP;
        pOuter->SetArraySize( numElements );
    }
}

FORTHOP( ptrToOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->SetFlag( kEngineFlagIsPointer );
}

FORTHOP( structOp )
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->SetFlag( kEngineFlagInStructDefinition );
    TypesManager* pManager = TypesManager::GetInstance();
	const char* pName = pOuter->GetNextSimpleToken();
    StructVocabulary* pVocab = pManager->StartStructDefinition(pName);
    pOuter->CompileBuiltinOpcode( OP_DO_STRUCT_TYPE );
    pOuter->CompileCell((cell)pVocab);

	pEngine->GetShell()->StartDefinition(pName, "stru");
}

FORTHOP( endStructOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->ClearFlag( kEngineFlagInStructDefinition );
    TypesManager* pManager = TypesManager::GetInstance();
    pManager->EndStructDefinition();
	pEngine->GetShell()->CheckDefinitionEnd("struct", "stru");
}

FORTHOP( classOp )
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    const char* pName = pOuter->GetNextSimpleToken();
    pOuter->StartClassDefinition(pName);

	pEngine->GetShell()->StartDefinition(pName, "clas");
}

FORTHOP( endClassOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->EndClassDefinition();
	pEngine->GetShell()->CheckDefinitionEnd("class", "clas");
}

FORTHOP(interfaceOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    const char* pName = pOuter->GetNextSimpleToken();
    pOuter->StartInterfaceDefinition(pName);

    pEngine->GetShell()->StartDefinition(pName, "intf");
}

FORTHOP(endInterfaceOp)
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->EndInterfaceDefinition();
    pEngine->GetShell()->CheckDefinitionEnd("interface", "intf");
}

FORTHOP(defineNewOp)
{
	startColonDefinition(pCore, "__newOp");
	Engine *pEngine = GET_ENGINE;
	ClassVocabulary* pVocab = TypesManager::GetInstance()->GetNewestClass();
	if (pVocab)
	{
        forthop* pEntry = pVocab->GetNewestEntry();
		if (pEntry)
		{
			pVocab->GetClassObject()->newOp = pEntry[0];
		}
		else
		{
			pEngine->SetError(ForthError::kBadSyntax, "__newOp being defined not found in 'new:'");
		}
	}
	else
	{
		pEngine->SetError(ForthError::kBadSyntax, "Defining class not found in 'new:'");
	}
}

FORTHOP( methodOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    // get next symbol, add it to vocabulary with type "user op"
    Vocabulary* pDefinitionVocab = nullptr;
    const char* pMethodName = pOuter->GetNextSimpleToken();
    int len = strlen(pMethodName);
    char* pColon = (char *) strchr(pMethodName, ':');
    if ((pColon != nullptr) && (len > 2) && (*pMethodName != ':') && (pMethodName[len - 1] != ':'))
    {
        // symbol may be of form CLASS:METHOD
        // this allows you to add methods to a class outside the class: ... ;class 
        *pColon = '\0';
        Vocabulary* pVocab = Vocabulary::FindVocabulary(pMethodName);
        if ((pVocab != NULL) && pVocab->IsClass())
        {
            pMethodName = pColon + 1;
            pDefinitionVocab = pVocab;
        }
    }

    TypesManager* pManager = TypesManager::GetInstance();
    ClassVocabulary* pVocab = pManager->GetNewestClass();
    if (pVocab == nullptr)
    {
        pEngine->SetError(ForthError::kBadSyntax, "Defining class not found in 'method:'");
        return;
    }

    bool isInterface = (pOuter->GetFlags() & kEngineFlagInInterfaceImplementation) != 0;

    int methodIndex = pVocab->FindMethod( pMethodName );
    if (isInterface)
    {
        if (methodIndex < 0)
        {
            pEngine->AddErrorText(pMethodName);
            pEngine->SetError(ForthError::kBadSyntax, "unknown method name in interface implementation");
            return;
        }

        ForthInterface* pInterface = pVocab->GetCurrentInterface();
        if (pInterface)
        {
            // this case is a class defining a method for a secondary interface
            //  in other words, within a "implements:"...";implements" block
            // an opcode needs to be allocated and set into the interface methods,
            //  but no symbol should be added to any vocabulary
            forthop newestOp = pOuter->AddOp(pCore->pDictionary->pCurrent);
            newestOp = COMPILED_OP(kOpUserDef, newestOp);
            pEngine->SetCompileState(1);
            pOuter->SetFlag(kEngineFlagIsMethod);
            pInterface->SetMethod(methodIndex, newestOp);
            pOuter->CompileBuiltinOpcode(OP_DEVOLVE);
        }
        else
        {
            pEngine->SetError(ForthError::kBadParameter, "current interface not found while implementing interface");
        }
    }
    else
    {
        pOuter->StartOpDefinition(pMethodName, true, kOpUserDef, pDefinitionVocab);
        pEngine->SetCompileState( 1 );          // switch to compile mode
        pOuter->SetFlag(kEngineFlagIsMethod);
        forthop* pEntry = pVocab->GetNewestEntry();
        if ( pEntry )
        {
            methodIndex = pVocab->AddMethod( pMethodName, methodIndex, pEntry[0] );
            pEntry[0] = methodIndex;
            pEntry[1] |= (kDTIsMethod | (uint32_t)BaseType::kVoid);
        }
		else
		{
			pEngine->SetError(ForthError::kBadSyntax, "method op being defined not found in 'method:'");
		}
    }

	pEngine->GetShell()->StartDefinition(pMethodName, "meth");
}

FORTHOP( endmethodOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    bool isInterface = (pOuter->GetFlags() & kEngineFlagInInterfaceImplementation) != 0;

    exitOp( pCore );
    // switch back from compile mode to execute mode
    pEngine->SetCompileState( 0 );
    // finish current symbol definition
    // compile local vars allocation op (if needed)
    pOuter->EndOpDefinition(!isInterface);
    pOuter->ClearFlag( kEngineFlagIsMethod );

	pEngine->GetShell()->CheckDefinitionEnd("method", "meth");
}

FORTHOP( returnsOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    if ( pOuter->CheckFlag( kEngineFlagIsMethod ) )
    {
        char *pToken = pOuter->GetNextSimpleToken();
        TypesManager* pManager = TypesManager::GetInstance();
        ClassVocabulary* pVocab = pManager->GetNewestClass();
        forthop* pEntry = pVocab->GetNewestEntry();
        if ( strcmp( pToken, "ptrTo" ) == 0 )
        {
            pToken = pOuter->GetNextSimpleToken();
            pEntry[1] |= kDTIsPtr;
        }

        // wipe the bottom 6 bits of type - this is probaby set to 15 (void)
        pEntry[1] &= ~STORAGE_DESCRIPTOR_TYPE_MASK;

        BaseType baseType = pManager->GetBaseTypeFromName( pToken );
        if ( baseType != BaseType::kUnknown )
        {
            pEntry[1] |= NATIVE_TYPE_TO_CODE(0, baseType);
        }
        else
        {
            // see if it is a struct type
            StructVocabulary* pTypeVocab = pManager->GetStructVocabulary( pToken );
            if ( pTypeVocab )
            {
                if ( pTypeVocab->IsClass() )
                {
                    pEntry[1] |= OBJECT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
                else
                {
                    pEntry[1] |= STRUCT_TYPE_TO_CODE( 0, pTypeVocab->GetTypeIndex() );
                }
            }
            else
            {
                pEngine->AddErrorText( pToken );
                pEngine->SetError( ForthError::kUnknownSymbol, " is not a known return type" );
            }
        }
    }
    else
    {
        // report error - "returns" outside of method
        pEngine->SetError( ForthError::kBadSyntax, "returns can only be used inside a method" );
    }
}

FORTHOP( doMethodOp )
{
    Engine *pEngine = GET_ENGINE;
    cell methodIndex = SPOP;
    RPUSH( ((cell) GET_TP) );
    ForthObject obj;
    POP_OBJECT(obj);
    SET_TP( obj );
    pEngine->ExecuteOp(pCore,  obj->pMethods[ methodIndex ] );
}

FORTHOP( implementsOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    TypesManager* pManager = TypesManager::GetInstance();
    ClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab && pOuter->CheckFlag( kEngineFlagInClassDefinition ) )
    {
        if (pOuter->CheckFlag(kEngineFlagInInterfaceImplementation))
        {
            pEngine->SetError(ForthError::kBadSyntax, "implements: already implementing an interface");
        }
        else
        {
            pOuter->SetFlag(kEngineFlagInInterfaceImplementation);
            pVocab->Implements(pOuter->GetNextSimpleToken());
        }
    }
    else
    {
        pEngine->SetError( ForthError::kBadSyntax, "implements: outside of class definition" );
    }
}

FORTHOP( endImplementsOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    TypesManager* pManager = TypesManager::GetInstance();
    ClassVocabulary* pVocab = pManager->GetNewestClass();
    if ( pVocab && pOuter->CheckFlag( kEngineFlagInInterfaceImplementation ) )
    {
        pVocab->EndImplements();
        pOuter->ClearFlag(kEngineFlagInInterfaceImplementation);
    }
    else
    {
        pEngine->SetError( ForthError::kBadSyntax, ";implements outside of interface definition" );
    }
}

FORTHOP(classIdOfOp)      // has precedence
{
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pClassName = pOuter->GetNextSimpleToken();
    TypesManager* pManager = TypesManager::GetInstance();

    // see if it is a struct type
    StructVocabulary* pTypeVocab = pManager->GetStructVocabulary(pClassName);
    if (pTypeVocab)
    {
        if (pTypeVocab->IsClass())
        {
            pOuter->ProcessConstant(pTypeVocab->GetTypeIndex());
        }
        else
        {
            pEngine->AddErrorText(pClassName);
            pEngine->SetError(ForthError::kBadSyntax, " is a struct, not a class");
        }
    }
    else
    {
        pEngine->AddErrorText(pClassName);
        pEngine->SetError(ForthError::kUnknownSymbol, " is not a known class");
    }
}

FORTHOP(getInterfaceOp)
{
    // TOS: classId pObject
    // query pObject for its interface of classID
    // if pObject doesn't have that interface, return null
    // else
    //      create an InterfaceObject object and set its wrappedObject field to pObject
    //      increment pObject's refcount
    int32_t classId = (int32_t)(SPOP);
    ForthObject pObject;
    POP_OBJECT(pObject);
    InterfaceObject interfaceObject = nullptr;

    if (pObject != nullptr)
    {
        ForthClassObject* pClassObj = GET_CLASS_OBJECT(pObject);
        ClassVocabulary* pVocab = pClassObj->pVocab;
        int32_t interfaceIndex = pVocab->FindInterfaceIndex(classId);
        if (interfaceIndex == 0)
        {
            interfaceObject = (InterfaceObject)pObject;
        }
        else if (interfaceIndex > 0)
        {
            ForthInterface* pInterface = pVocab->GetInterface(interfaceIndex);
            if (pInterface)
            {
                interfaceObject = (InterfaceObject)ALLOCATE_BYTES(sizeof(oInterfaceObjectStruct));
                TRACK_NEW;
                interfaceObject->pMethods = pInterface->GetMethods();
                interfaceObject->refCount = 0;
                interfaceObject->pWrappedObject = pObject;
                pObject->refCount += 1;
            }
        }
    }

    PUSH_OBJECT(interfaceObject);
}

FORTHOP( unionOp )
{
    TypesManager* pManager = TypesManager::GetInstance();
    pManager->GetNewestStruct()->StartUnion();
}

FORTHOP( extendsOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pSym = pOuter->GetNextSimpleToken();
    Vocabulary* pFoundVocab;
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );
    if ( pEntry )
    {
        TypesManager* pManager = TypesManager::GetInstance();
        StructVocabulary* pParentVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pParentVocab )
        {
			if ( pOuter->CheckFlag( kEngineFlagInClassDefinition ) )
			{
				if ( pParentVocab->IsClass() )
				{
					ClassVocabulary* pParentClassVocab = reinterpret_cast<ClassVocabulary *>(pParentVocab);
					pManager->GetNewestClass()->Extends( pParentClassVocab );
				}
				else
				{
					pEngine->SetError( ForthError::kBadSyntax, "extend class from struct is illegal" );
				}
			}
			else
			{
				if ( pOuter->CheckFlag( kEngineFlagInStructDefinition ) )
				{
					if ( pParentVocab->IsClass() )
					{
						pEngine->SetError( ForthError::kBadSyntax, "extend struct from class is illegal" );
					}
					else
					{
						pManager->GetNewestStruct()->Extends( pParentVocab );
					}
				}
				else
				{
					pEngine->SetError( ForthError::kBadSyntax, "extends must be used in a class or struct definition" );
				}
			}
        }
        else
        {
            pEngine->SetError( ForthError::kUnknownSymbol, pSym );
            pEngine->AddErrorText( " is not a structure" );
        }
    }
    else
    {
        pEngine->SetError( ForthError::kUnknownSymbol, pSym );
    }
}

FORTHOP( strSizeOfOp )
{
    // TODO: allow sizeOf to be applied to variables
    // TODO: allow sizeOf to apply to native types, including strings
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pSym = (char *)(SPOP);
    Vocabulary* pFoundVocab;
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol( pSym, &pFoundVocab );
	int32_t size = 0;

    if ( pEntry )
    {
        TypesManager* pManager = TypesManager::GetInstance();
        StructVocabulary* pStructVocab = pManager->GetStructVocabulary( pEntry[0] );
        if ( pStructVocab != nullptr)
        {
			size = pStructVocab->GetSize();
        }
        else
        {
            size = pManager->GetBaseTypeSizeFromName( pSym );
            if ( size <= 0 )
            {
                pEngine->AddErrorText( pSym );
                pEngine->SetError( ForthError::kUnknownSymbol, " is not a structure or base type" );
            }
        }
    }
    else
    {
        pEngine->SetError( ForthError::kUnknownSymbol, pSym );
    }
	SPUSH(size);
}

FORTHOP( strOffsetOfOp )
{
    // TODO: allow offsetOf to be variable.field instead of just type.field
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    int32_t size = 0;

	char *pType = (char *)(SPOP);
    char *pField = strchr( pType, '.' );
    if ( pField == NULL )
    {
        pEngine->SetError( ForthError::kBadSyntax, "argument must contain a period" );
    }
	else
	{
		char oldChar = *pField;
		*pField++ = '\0';

		Vocabulary* pFoundVocab;
		forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol(pType, &pFoundVocab);
		if (pEntry)
		{
			TypesManager* pManager = TypesManager::GetInstance();
			StructVocabulary* pStructVocab = pManager->GetStructVocabulary(pEntry[0]);
			if (pStructVocab)
			{
				pEntry = pStructVocab->FindSymbol(pField);
				if (pEntry)
				{
					size = pEntry[0];
				}
				else
				{
					pEngine->AddErrorText(pField);
					pEngine->AddErrorText(" is not a field in ");
					pEngine->SetError(ForthError::kUnknownSymbol, pType);
				}
			}
			else
			{
				pEngine->AddErrorText(pType);
				pEngine->SetError(ForthError::kUnknownSymbol, " is not a structure");
			}
		}
		else
		{
			pEngine->SetError(ForthError::kUnknownSymbol, pType);
		}
		pField[-1] = oldChar;
	}

	SPUSH(size);
}

FORTHOP( processConstantOp )
{
	// if interpreting, do nothing
	// if compiling, get value from TOS and compile what is needed to push that value at runtime
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    if (pEngine->IsCompiling())
	{
		cell constantValue = (SPOP);
        pOuter->ProcessConstant(constantValue, false, true);
	}
}

FORTHOP( processLongConstantOp )
{
	// if interpreting, do nothing
	// if compiling, get value from TOS and compile what is needed to push that value at runtime
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    if (pEngine->IsCompiling())
	{
#if defined(FORTH64)
        cell val = SPOP;
        pOuter->ProcessConstant(val, false, false);
#else
        stackInt64 val;
        LPOP(val);
        pOuter->ProcessConstant(val.s64, false, false);
#endif
	}
}

FORTHOP(showStructOp)
{
	// TOS: structVocabPtr ptrToStructData
	StructVocabulary *pStructVocab = (StructVocabulary *)(SPOP);
	void* pStruct = (void *)(SPOP);
	pStructVocab->ShowData(pStruct, pCore, true);
}

void __newOp(CoreState* pCore, const char* pClassName)
{
    // TODO: allow sizeOf to be applied to variables
    // TODO: allow sizeOf to apply to native types, including strings
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary* pFoundVocab;
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol(pClassName, &pFoundVocab);

    if ( pEntry )
    {
        TypesManager* pManager = TypesManager::GetInstance();
        ClassVocabulary* pClassVocab = (ClassVocabulary *) (pManager->GetStructVocabulary( pEntry[0] ));

        if ( pClassVocab && pClassVocab->IsClass() )
        {
			int32_t initOpcode = pClassVocab->GetInitOpcode();
			if (pEngine->IsCompiling())
            {
                pOuter->ProcessConstant(pClassVocab->GetTypeIndex(), false);
                pOuter->CompileBuiltinOpcode( OP_DO_NEW );
				if (initOpcode != 0)
				{
					pOuter->CompileBuiltinOpcode(OP_OVER);
					pOuter->CompileOpcode(initOpcode);
				}
			}
            else
            {
                SPUSH( (cell) pClassVocab );
				pEngine->FullyExecuteOp(pCore, pClassVocab->GetClassObject()->newOp);
				if (initOpcode != 0)
				{
					// copy object data pointer to TOS to be used by init 
					int32_t a = (GET_SP)[1];
					SPUSH(a);
					pEngine->FullyExecuteOp(pCore, initOpcode);
				}
			}
        }
        else
        {
            pEngine->AddErrorText(pClassName);
            pEngine->SetError( ForthError::kUnknownSymbol, " is not a class" );
        }
    }
    else
    {
        pEngine->SetError(ForthError::kUnknownSymbol, pClassName);
    }
}

FORTHOP(newOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

	char *pSym = pOuter->GetNextSimpleToken();
	__newOp(pCore, pSym);
}

FORTHOP(strNewOp)
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary* pFoundVocab;

    char *pClassName = (char*)(SPOP);
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol(pClassName, &pFoundVocab);

	if (pEntry)
	{
		TypesManager* pManager = TypesManager::GetInstance();
		ClassVocabulary* pClassVocab = (ClassVocabulary *)(pManager->GetStructVocabulary(pEntry[0]));

		if (pClassVocab && pClassVocab->IsClass())
		{
			int32_t initOpcode = pClassVocab->GetInitOpcode();
			SPUSH((cell)pClassVocab);
			pEngine->FullyExecuteOp(pCore, pClassVocab->GetClassObject()->newOp);
			if (initOpcode != 0)
			{
				// copy object data pointer to TOS to be used by init 
				int32_t a = (GET_SP)[1];
				SPUSH(a);
				pEngine->FullyExecuteOp(pCore, initOpcode);
			}
		}
		else
		{
            pEngine->AddErrorText(pClassName);
			pEngine->SetError(ForthError::kUnknownSymbol, " is not a class");
		}
	}
	else
	{
        pEngine->SetError(ForthError::kUnknownSymbol, pClassName);
	}
}

FORTHOP(makeObjectOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    const char *pClassName = pOuter->AddTempString(pOuter->GetNextSimpleToken());
    char* pInstanceName = pOuter->GetNextSimpleToken();
    char* pContainedClassName = nullptr;
    if (::strcmp(pInstanceName, "of") == 0)
    {
        pContainedClassName = pOuter->AddTempString(pOuter->GetNextSimpleToken());
        pInstanceName = pOuter->GetNextSimpleToken();
    }

    __newOp(pCore, pClassName);
    if (GET_STATE != OpResult::kOk)
    {
        return;
    }

    Vocabulary* pFoundVocab;
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol(pClassName, &pFoundVocab);

    if (pEntry)
    {
        TypesManager* pManager = TypesManager::GetInstance();
        ClassVocabulary* pClassVocab = (ClassVocabulary *)(pManager->GetStructVocabulary(pEntry[0]));

        if (pClassVocab && pClassVocab->IsClass())
        {
            if (pEngine->IsCompiling())
            {
                pOuter->CompileBuiltinOpcode(OP_INTO);
            }
            else
            {
                SET_VAR_OPERATION(VarOperation::kVarSet);
            }
            pClassVocab->DefineInstance(pInstanceName, pContainedClassName);
        }
        else
        {
            pEngine->AddErrorText(pClassName);
            pEngine->SetError(ForthError::kUnknownSymbol, " is not a class");
        }
    }
    else
    {
        pEngine->SetError(ForthError::kUnknownSymbol, pClassName);
    }

}

FORTHOP(doNewOp)
{
	// this op is compiled for 'new foo', the class typeIndex is on TOS
	TypesManager* pManager = TypesManager::GetInstance();
	Engine *pEngine = GET_ENGINE;

	int typeIndex = SPOP;
	ForthTypeInfo* pTypeInfo = pManager->GetTypeInfo(typeIndex);
	if (pTypeInfo != nullptr)
	{
		if (pTypeInfo->pVocab->IsClass())
		{
			ClassVocabulary *pClassVocab = static_cast<ClassVocabulary *>(pTypeInfo->pVocab);
			SPUSH((cell)pClassVocab);
			pEngine->FullyExecuteOp(pCore, pClassVocab->GetClassObject()->newOp);
		}
		else
		{
			// TODO: report that vocab is not a class vocab
		}
	}
	else
	{
		// TODO report unknown class with index N
	}
}

FORTHOP( allocObjectOp )
{
	// allocObject is the default new op, it is used only for classes which don't define their
	//   own 'new' op, or extend a class that does
    ClassVocabulary *pClassVocab = (ClassVocabulary *) (SPOP);
    forthop* pMethods = pClassVocab->GetMethods();
    if (pMethods)
    {
        int32_t nBytes = pClassVocab->GetSize();
        ForthObject newObject = (ForthObject) ALLOCATE_BYTES( nBytes );
		memset(newObject, 0, nBytes );
        newObject->pMethods = pMethods;
        SPUSH( (cell)newObject);
    }
    else
    {
        Engine *pEngine = GET_ENGINE;
        pEngine->AddErrorText( pClassVocab->GetName() );
        pEngine->SetError( ForthError::kBadParameter, " failure in new - has no methods" );
    }
}

FORTHOP( initMemberStringOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pString = pOuter->GetNextSimpleToken();
    TypesManager* pManager = TypesManager::GetInstance();
    ClassVocabulary* pVocab = pManager->GetNewestClass();
    forthop* pEntry;

    if ( !pOuter->CheckFlag( kEngineFlagIsMethod ) || (pVocab == NULL) )
    {
        pEngine->SetError( ForthError::kBadSyntax, "initMemberString can only be used inside a method" );
        return;
    }
    pEntry = pVocab->FindSymbol( pString );
    if ( !pEntry )
    {
        pEngine->AddErrorText( pString );
        pEngine->SetError( ForthError::kUnknownSymbol, " is not a field in this class" );
        return;
    }

    int32_t typeCode = pEntry[1];
    if ( !CODE_IS_SIMPLE(typeCode) || !CODE_IS_NATIVE(typeCode) || (CODE_TO_BASE_TYPE(typeCode) != BaseType::kString) )
    {
        pEngine->SetError( ForthError::kBadSyntax, "initMemberString can only be used on a simple string" );
        return;
    }
	// pEntry[2] is the total string struct length in bytes, subtract 9 for maxLen, curLen and terminating null
    int32_t len = pEntry[2] - 9;
	if ( len > 4095 )
    {
        pEngine->SetError( ForthError::kBadParameter, "initMemberString can not be used on string with size > 4095" );
        return;
    }
	if ( pEntry[0] > 4095 )
    {
        pEngine->SetError( ForthError::kBadParameter, "initMemberString can not be used on string with member offset > 4095" );
        return;
    }

	// only shift up by 10 bits since pEntry[0] is member byte offset, opcode holds member offset in longs
    int32_t varOffset = (pEntry[0] << 10) | len;

    pOuter->CompileOpcode( kOpMemberStringInit, varOffset );
}

FORTHOP(readObjectsOp)
{
    ForthObject outObjects;
    ForthObject inStream;
    POP_OBJECT(outObjects);
    POP_OBJECT(inStream);
    ObjectReader reader;

    bool itWorked = reader.ReadObjects(inStream, outObjects, pCore);
    if (!itWorked)
    {
        Engine *pEngine = GET_ENGINE;
        pEngine->SetError(ForthError::kBadSyntax, reader.GetError().c_str());
    }
}

FORTHOP(enumOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    //if ( pOuter->CheckFlag( kEngineFlagInStructDefinition ) )
    //{
    //    pEngine->SetError( ForthError::kBadSyntax, "enum definition not allowed inside struct definition" );
    //    return;
    //}
    pOuter->StartEnumDefinition();
	const char* pName = pOuter->GetNextSimpleToken();
    forthop* pEntry = pOuter->StartOpDefinition(pName, false, kOpUserDefImmediate);
    pEntry[1] = (forthop)BASE_TYPE_TO_CODE( BaseType::kUserDefinition );
    pOuter->CompileBuiltinOpcode( OP_DO_ENUM );
    // save ptr to enum info block for ;enum to fill in number of symbols
    forthop* pHere = pEngine->GetDP();
    pOuter->SetNewestEnumInfo((ForthEnumInfo*)pHere);
    // enum info block holds:
    // 0  ptr to vocabulary enum is defined in
    // 1  enum size in bytes (default to 4)
    // 2  number of enums defined
    // 3  offset in longs from top of vocabulary to last enum symbol defined
    // 4+ enum name string
    Vocabulary* pDefinitionVocabulary = pOuter->GetDefinitionVocabulary();
    pOuter->CompileCell((cell)pDefinitionVocabulary);
    pOuter->CompileInt(4);	// default enum size is 4 bytes
    // the actual number of enums and vocabulary offset of last enum are filled in in endenumOp
    pOuter->CompileInt(pDefinitionVocabulary->GetNumEntries());
    pOuter->CompileInt(0);
    // stick enum name string on the end
    int nameLen = strlen(pName) + 1;
    memcpy(pEngine->GetDP(), pName, nameLen);
    pEngine->AllotBytes(nameLen);
    pEngine->AlignDP();
    ForthEnumInfo* pEnum = (ForthEnumInfo*)pHere;

	pEngine->GetShell()->StartDefinition(pName, "enum");
}

FORTHOP( endenumOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthEnumInfo* pNewestEnum = pOuter->GetNewestEnumInfo();
    pOuter->EndEnumDefinition();
    if (pNewestEnum != nullptr)
    {
        if (pEngine->GetShell()->CheckDefinitionEnd("enum", "enum"))
        {
            Vocabulary* pVocab = pNewestEnum->pVocab;
            pNewestEnum->numEnums = pVocab->GetNumEntries() - pNewestEnum->numEnums;
            pNewestEnum->vocabOffset = pVocab->GetEntriesEnd() - pVocab->GetNewestEntry();
        }
        pOuter->SetNewestEnumInfo(nullptr);
    }
    else
    {
        pEngine->SetError(ForthError::kBadSyntax, "Missing enum info pointer at end of enum definition");
    }
}

FORTHOP(findEnumSymbolOp)
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthEnumInfo* pEnumInfo = (ForthEnumInfo*)(SPOP);
    int32_t enumValue = SPOP;
    Vocabulary* pVocab = pEnumInfo->pVocab;
    // NOTE: this will only find enums which are defined as constants with 24 bits or less
    int32_t numEnums = pEnumInfo->numEnums;
    int32_t lastEnumOffset = pEnumInfo->vocabOffset;
    int32_t numSymbols = pVocab->GetNumEntries();
    forthop* pFoundEntry = nullptr;
    int32_t currentVocabOffset = pVocab->GetEntriesEnd() - pVocab->GetNewestEntry();
    if (currentVocabOffset >= lastEnumOffset)
    {
        forthop* pEntry = pVocab->GetEntriesEnd() - lastEnumOffset;
        for (int i = 0; i < numEnums; i++)
        {
            if (pEntry < pVocab->GetNewestEntry())
            {
                break;
            }
            if ((*pEntry & 0xFFFFFF) == enumValue)
            {
                char* pEnumName = pOuter->AddTempString(pVocab->GetEntryName(pEntry), pVocab->GetEntryNameLength(pEntry));
                SPUSH((cell)pEnumName);
                SPUSH(~0);
                return;
            }
            pEntry = pVocab->NextEntry(pEntry);
            --numSymbols;
        }
    }
    SPUSH(0);
}

FORTHOP(doVocabOp)
{
    // IP points to data field
    Vocabulary* pVocab = (Vocabulary *) *((ucell *)GET_IP);
    pVocab->DoOp( pCore );
    SET_IP( (forthop* ) (RPOP) );
}

FORTHOP(getClassByIndexOp)
{
	// IP points to data field
	int typeIndex = SPOP;
	ForthObject classObject = nullptr;
	ClassVocabulary* pClassVocab = GET_CLASS_VOCABULARY(typeIndex);
	if (pClassVocab != nullptr)
	{
		ForthClassObject* pClassObject = pClassVocab->GetClassObject();
		if (pClassObject != nullptr)
		{
			classObject = (ForthObject) pClassObject;
		}
	}
	PUSH_OBJECT(classObject);
}


// doStructTypeOp is compiled at the start of each user-defined structure defining word 
FORTHOP(doStructTypeOp)
{
	// IP points to data field
	StructVocabulary *pVocab = *(StructVocabulary **)(GET_IP);

	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    bool doDefineInstance = true;
	if (pEngine->IsCompiling())
	{
		// handle the case 'ref STRUCT_TYPE'
		forthop* pLastOp = pOuter->GetLastCompiledOpcodePtr();
		if ((pLastOp != NULL) && (*pLastOp == gCompiledOps[OP_REF]))
		{
			// compile this opcode so at runtime (ref STRUCT_OP) will push struct vocab address
			TypesManager* pManager = TypesManager::GetInstance();
			pOuter->CompileOpcode(pManager->GetTypeInfo(pVocab->GetTypeIndex())->op);
			doDefineInstance = false;
		}
	}
	else
	{
		if (GET_VAR_OPERATION == VarOperation::kVarRef)
		{
			SPUSH((cell)pVocab);
			doDefineInstance = false;
			CLEAR_VAR_OPERATION;
		}
	}
	if (doDefineInstance)
	{
		pVocab->DefineInstance();
	}
	SET_IP((forthop* )(RPOP));
}

// doClassTypeOp is compiled at the start of each user-defined class defining word 
FORTHOP( doClassTypeOp )
{
    // IP points to data field
	// TODO: change this to take class typeIndex on TOS like doNewOp
    ClassVocabulary *pVocab = *((ClassVocabulary **)GET_IP);
	pVocab->DefineInstance();
    SET_IP( (forthop* ) (RPOP) );
}

// has precedence!
// this is compiled as the only op for each enum set defining word
// an enum set defining word acts just like the "int" op - it defines
// a variable or field that is 4 bytes in size
FORTHOP( doEnumOp )
{
    bool doDefineInstance = true;
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* oldIP = GET_IP;
    // we need to pop the IP, since there is no instruction after this one
    SET_IP((forthop* )(RPOP));

    if (pEngine->IsCompiling())
    {
        // handle the case 'ref ENUM_TYPE'
        forthop* pLastOp = pOuter->GetLastCompiledOpcodePtr();
        if ((pLastOp != NULL) && (*pLastOp == gCompiledOps[OP_REF]))
        {
            // compile the opcode that called us so at runtime (ref ENUM_OP) will push enum info ptr
            pOuter->CompileOpcode(GET_IP[-1]);
            doDefineInstance = false;
        }
    }
    else
    {
        if (GET_VAR_OPERATION == VarOperation::kVarRef)
        {
            // ref ENUM_DEFINING_OP ... returns ptr to enum info block
            SPUSH((cell)oldIP);
            doDefineInstance = false;
        }
    }

    if (doDefineInstance)
    {
        ForthEnumInfo* pInfo = (ForthEnumInfo *)oldIP;
        switch (pInfo->size)
        {
        case 1:
            byteOp(pCore);
            break;

        case 2:
            shortOp(pCore);
            break;

        case 4:
            intOp(pCore);
            break;

            //case 8:
            //	longOp(pCore);
            //	break;

        default:
            pEngine->SetError(ForthError::kBadParameter, "enum size must be 1, 2 or 4");
        }
    }
    CLEAR_VAR_OPERATION;
}

// in traditional FORTH, the vocabulary entry for the operation currently
// being defined is "smudged" so that if a symbol appears in its own definition
// it will normally only match any previously existing symbol of the same name.
// This makes it easy to redefine existing symbols in terms of their old
// definitions.
// The "recursive" op just unsmudges the vocabulary entry currently being
// defined so that a definition can refer to itself.

// has precedence!
FORTHOP( recursiveOp )
{
    GET_ENGINE->GetOuterInterpreter()->GetDefinitionVocabulary()->UnSmudgeNewestSymbol();
}

FORTHOP( precedenceOp )
{
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pSym = pOuter->GetNextSimpleToken();
	forthop* pEntry = pOuter->GetDefinitionVocabulary()->FindSymbol(pSym);
    
    if ( pEntry )
    {
        forthop op = *pEntry;
        uint32_t opVal = FORTH_OP_VALUE( op );
        switch ( FORTH_OP_TYPE( op ) )
        {
            case kOpNative:
                *pEntry = COMPILED_OP( kOpNativeImmediate, opVal );
                break;

            case kOpUserDef:
                *pEntry = COMPILED_OP( kOpUserDefImmediate, opVal );
                break;

            case kOpCCode:
                *pEntry = COMPILED_OP( kOpCCodeImmediate, opVal );
                break;

            default:
                CONSOLE_STRING_OUT( "!!!! Can\'t set precedence for " );
                CONSOLE_STRING_OUT( pSym );
                CONSOLE_STRING_OUT( "s - wrong type !!!!\n" );
                SPEW_ENGINE( "!!!! Can\'t set precedence for %s - wrong type !!!!\n", pSym );
                break;
        }
    }
    else
    {
        CONSOLE_STRING_OUT( "!!!! Failure finding symbol " );
        CONSOLE_STRING_OUT( pSym );
        SPEW_ENGINE( "!!!! Failure finding symbol %s !!!!\n", pSym );
    }
}

FORTHOP(strLoadOp)
{
    char* pFileName = ((char*)(SPOP));

    if (pFileName != NULL)
    {
        Engine* pEngine = GET_ENGINE;
        if (pEngine->PushInputFile(pFileName) == false)
        {
            CONSOLE_STRING_OUT("!!!! Failure opening source file ");
            CONSOLE_STRING_OUT(pFileName);
            CONSOLE_STRING_OUT(" !!!!\n");
            SPEW_ENGINE("!!!! Failure opening source file %s !!!!\n", pFileName);
        }
    }
}

FORTHOP(strLoadQOp)
{
    char* pFileName = ((char*)(SPOP));

    if (pFileName != NULL)
    {
        Engine* pEngine = GET_ENGINE;
        if (pEngine->PushInputFile(pFileName) == false)
        {
            SPUSH(0);
        }
        else
        {
            SPUSH(-1);
        }
    }
}

FORTHOP(strRunFileOp)
{
	char *pFileName = ((char *)(SPOP));

	if (pFileName != NULL)
	{
		Engine *pEngine = GET_ENGINE;
		Shell* pShell = pEngine->GetShell();
        std::string containingDir;
		FILE *pInFile = pShell->OpenForthFile(pFileName, containingDir);
		if (pInFile != NULL)
		{
			FileInputStream* pInputStream = new FileInputStream(pInFile, pFileName);
			pShell->RunOneStream(pInputStream);
		}
		else
		{
			CONSOLE_STRING_OUT("!!!! Failure opening source file ");
			CONSOLE_STRING_OUT(pFileName);
			CONSOLE_STRING_OUT(" !!!!\n");
			SPEW_ENGINE("!!!! Failure opening source file %s !!!!\n", pFileName);
		}
	}
}

FORTHOP(loadDoneOp)
{
    GET_ENGINE->PopInputStream();
}

FORTHOP( requiresOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pSymbolName = pOuter->GetNextSimpleToken();
	VocabularyStack* pVocabStack = pOuter->GetVocabularyStack();

	if ( pVocabStack->FindSymbol( pSymbolName ) == NULL )
    {
        // symbol not found - load symbol.fs
		int buffLen = strlen( pSymbolName ) + 8;
        char *pFileName = (char *) __MALLOC( buffLen );
		SNPRINTF( pFileName, buffLen, "%s.fs", pSymbolName );
        if ( pEngine->PushInputFile( pFileName ) == false )
        {
            CONSOLE_STRING_OUT( "!!!! Failure opening source file " );
            CONSOLE_STRING_OUT( pFileName );
            CONSOLE_STRING_OUT( " !!!!\n" );
            SPEW_ENGINE( "!!!! Failure opening source file %s !!!!\n", pFileName );
        }
        __FREE( pFileName );
    }
}

FORTHOP( evaluateOp )
{
    int len = SPOP;
    const char* pStr = (const char *) SPOP;
    if ( (pStr != NULL) && (len > 0) )
    {
        Engine *pEngine = GET_ENGINE;
        pEngine->PushInputBuffer( pStr, len );
		Shell* pShell = pEngine->GetShell();
		pShell->ProcessLine( pStr );
		pEngine->PopInputStream();
    }
}

FORTHOP( strEvaluateOp )
{
    const char* pStr = (const char *) SPOP;
    if ( pStr != NULL )
    {
        int len = strlen( pStr );
        Engine *pEngine = GET_ENGINE;
		Shell* pShell = pEngine->GetShell();
		BufferInputStream* pInputStream = new BufferInputStream(pStr, len, false, len + 4);
		pShell->RunOneStream(pInputStream);
    }
}

FORTHOP( stateInterpretOp )
{
    GET_ENGINE->SetCompileState( 0 );
}

FORTHOP( stateCompileOp )
{
    GET_ENGINE->SetCompileState( 1 );
}

FORTHOP( stateOp )
{
    SPUSH( (cell) GET_ENGINE->GetCompileStatePtr() );
}

FORTHOP( strTickOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pToken = (char *) SPOP;
    Vocabulary* pFoundVocab;
    forthop* pSymbol = pOuter->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pSymbol != NULL )
    {
        SPUSH( *pSymbol );
    }
    else
    {
        SET_ERROR( ForthError::kUnknownSymbol );
        pEngine->AddErrorText( pToken );
    }
}

FORTHOP( compileOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop* pIP = GET_IP;
    forthop op = *pIP++;
    SET_IP( pIP );
    pOuter->CompileOpcode( op );
}

// has precedence!
FORTHOP( postponeOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary* pFoundVocab;

    char *pToken = pOuter->GetNextSimpleToken();
    forthop* pEntry = pOuter->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pEntry != NULL )
    {
        forthop op = *pEntry;
        int opType = FORTH_OP_TYPE( op );
        switch ( opType )
        {
        case kOpNativeImmediate:
        case kOpUserDefImmediate:
        case kOpCCodeImmediate:
            pOuter->CompileOpcode( op );
            break;

        default:
            // op without precedence
            pOuter->CompileBuiltinOpcode( OP_COMPILE );
            pOuter->CompileOpcode( op );
            break;
        }
    }
    else
    {
        SET_ERROR( ForthError::kUnknownSymbol );
    }
}

// has precedence!
FORTHOP( bracketTickOp )
{
    // TODO: what should this do if state is interpret? an error? or act the same as tick?
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pToken = pOuter->GetNextSimpleToken();
    Vocabulary* pFoundVocab;
    forthop* pSymbol = pOuter->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    if ( pSymbol != NULL )
    {
        pOuter->CompileBuiltinOpcode( OP_INT_VAL );
        pOuter->CompileInt( *pSymbol );
    }
    else
    {
        SET_ERROR( ForthError::kUnknownSymbol );
    }
}

FORTHOP( bodyOp )
{
    Engine *pEngine = GET_ENGINE;

    forthop op = SPOP;
    uint32_t opIndex = FORTH_OP_VALUE( op );
    int32_t opType = FORTH_OP_TYPE( op );
    switch ( opType )
    {
    case kOpNativeImmediate:
    case kOpUserDefImmediate:
    case kOpNative:
    case kOpUserDef:
        if ( opIndex < GET_NUM_OPS )
        {
            SPUSH( ((cell)(OP_TABLE[ opIndex ])) + 4 );
        }
        else
        {
            SET_ERROR( ForthError::kBadOpcode );
        }
        break;

    default:
        SET_ERROR( ForthError::kBadOpcodeType );
        break;
    }
}

//##############################
//
//  output ops
//
void
consoleOutToFile( CoreState   *pCore,
                  const char       *pMessage )
{    
    FILE *pOutFile = stdout;//static_cast<FILE*>(GET_CON_OUT_DATA);
    if ( pOutFile != NULL )
    {
        fprintf(pOutFile, "%s", pMessage );
    }
    else
    {
        // TODO: report error
    }
}

/*
void
consoleOutToString( CoreState   *pCore,
                    const char       *pMessage )
{    
    char *pOutStr = static_cast<char*>(GET_CON_OUT_DATA);
    if ( pOutStr != NULL )
    {
        strcat( pOutStr, pMessage );
    }
    else
    {
        // TODO: report error
    }
}

void
consoleOutToOp( CoreState   *pCore,
                const char       *pMessage )
{
    SPUSH( (cell) pMessage );
    forthop op = GET_CON_OUT_OP;
    Engine* pEngine = GET_ENGINE;
    pEngine->ExecuteOneOp(pCore,  op );
}
*/
static void
printNumInCurrentBase( CoreState   *pCore,
                       long             val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    ldiv_t v;
    long base;
    bool bIsNegative, bPrintUnsigned;
    uint32_t urem;
    ePrintSignedMode signMode;

    if ( val == 0 )
    {
        strcpy( buff, "0 " );
        pNext = buff;
    }
    else
    {
        base = *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( (base == 10) && (signMode == kPrintSignedDecimal) )
        {

            // most common case - print signed decimal
            SNPRINTF( buff, sizeof(buff), "%d ", val );
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            *--pNext = ' ';
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
                urem = ((uint32_t) val) % ((uint32_t) base);
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                val = ((uint32_t) val) / ((uint32_t) base);
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    val = (-val);
                }
            }
            while ( val != 0 )
            {
#if defined(WINDOWS_BUILD)
#if defined(MSDEV)
                v = div( val, base );
#else
                v = ldiv( val, base );
#endif
#elif defined(LINUX) || defined(MACOSX)
                v = ldiv( val, base );
#endif
                *--pNext = (char) ( (v.rem < 10) ? (v.rem + '0') : ((v.rem - 10) + 'a') );
                val = v.quot;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


static void
printLongNumInCurrentBase( CoreState   *pCore,
						   int64_t        val )
{
    NEEDS(1);
#define PRINT_NUM_BUFF_CHARS 68
    char buff[ PRINT_NUM_BUFF_CHARS ];
    char *pNext = &buff[ PRINT_NUM_BUFF_CHARS ];
    uint32_t base;
    bool bIsNegative, bPrintUnsigned;
    uint32_t urem;
	uint64_t uval;
    ePrintSignedMode signMode;

    if ( val == 0L )
    {
        strcpy( buff, "0 " );
        pNext = buff;
    }
    else
    {
        base = (uint32_t) *(GET_BASE_REF);

        signMode = (ePrintSignedMode) GET_PRINT_SIGNED_NUM_MODE;
        if ( base == 10 ) 
        {

            // most common case - print signed decimal
			switch ( signMode )
			{
			case kPrintAllUnsigned:
	            SNPRINTF( buff, sizeof(buff), "%ulld ", val );
				break;
			default:
	            SNPRINTF( buff, sizeof(buff), "%lld ", val );
				break;
			}
            pNext = buff;

        }
        else
        {

            // unsigned or any base other than 10

            *--pNext = 0;
            *--pNext = ' ';
            bPrintUnsigned = !(signMode == kPrintAllSigned);
            if ( bPrintUnsigned )
            {
                // since div is defined as signed divide/mod, make sure
                //   that the number is not negative by generating the bottom digit
                bIsNegative = false;
				uval = (uint64_t) val;
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            else
            {
                bIsNegative = ( val < 0 );
                if ( bIsNegative )
                {
                    uval = (-val);
                }
				else
				{
					uval = val;
				}
            }
            while ( uval != 0 )
            {
                urem = uval % base;
                *--pNext = (char) ( (urem < 10) ? (urem + '0') : ((urem - 10) + 'a') );
                uval = uval / base;
            }
            if ( bIsNegative )
            {
                *--pNext = '-';
            }
        }
    }
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", pNext );
#endif

    CONSOLE_STRING_OUT( pNext );
}


FORTHOP( printNumOp )       // . 
{
#if defined(FORTH64)
    cell val = SPOP;
    printLongNumInCurrentBase(pCore, val);
#else
    printNumInCurrentBase(pCore, SPOP);
#endif
}

FORTHOP( printLongNumOp )       // l.
{
#if defined(FORTH64)
    cell val = SPOP;
    printLongNumInCurrentBase(pCore, val);
#else
    stackInt64 val;
    LPOP(val);
    printLongNumInCurrentBase(pCore, val.s64);
#endif
}

FORTHOP( printNumDecimalOp )    // %d
{
    NEEDS(1);
    char buff[40];

    cell val = SPOP;
#if defined(FORTH64)
    SNPRINTF(buff, sizeof(buff), "%lld", val);
#else
    SNPRINTF(buff, sizeof(buff), "%d", val);
#endif

#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printNumHexOp )    // %x
{
    NEEDS(1);
    char buff[20];

    cell val = SPOP;
#if defined(FORTH64)
    SNPRINTF(buff, sizeof(buff), "%llx", val);
#else
    SNPRINTF(buff, sizeof(buff), "%x", val);
#endif

#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongDecimalOp )   // %2d
{
    NEEDS(2);
    char buff[40];

    int64_t val;
#if defined(FORTH64)
    val = SPOP;
#else
    stackInt64 sval;
    LPOP(sval);
    val = sval.s64;
#endif
#if defined(WINDOWS_BUILD)
    SNPRINTF( buff, sizeof(buff), "%I64d", val );
#else
    SNPRINTF( buff, sizeof(buff), "%lld", val);
#endif
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printLongHexOp )   // %2x
{
    NEEDS(1);
    char buff[20];

    int64_t val;
#if defined(FORTH64)
    val = SPOP;
#else
    stackInt64 sval;
    LPOP(sval);
    val = sval.s64;
#endif
#if defined(WINDOWS_BUILD)
    SNPRINTF( buff, sizeof(buff), "%I64x", val );
#else
    SNPRINTF( buff, sizeof(buff), "%llx", val );
#endif
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printFloatOp )
{
    NEEDS(1);
    char buff[36];

    float fval = FPOP;
    SNPRINTF( buff, sizeof(buff), "%f", fval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printDoubleOp )
{
    NEEDS(2);
    char buff[36];
    double dval = DPOP;

    SNPRINTF( buff, sizeof(buff), "%f", dval );
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP(printFloatGOp)
{
	NEEDS(1);
	char buff[36];

	float fval = FPOP;
	SNPRINTF(buff, sizeof(buff), "%g", fval);
#ifdef TRACE_PRINTS
	SPEW_PRINTS("printed %s\n", buff);
#endif

	CONSOLE_STRING_OUT(buff);
}

FORTHOP(printDoubleGOp)
{
	NEEDS(2);
	char buff[36];
	double dval = DPOP;

	SNPRINTF(buff, sizeof(buff), "%g", dval);
#ifdef TRACE_PRINTS
	SPEW_PRINTS("printed %s\n", buff);
#endif

	CONSOLE_STRING_OUT(buff);
}

FORTHOP(format32Op)
{
    NEEDS(2);

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pFmt = (char *) (SPOP);
    int buffLen = 32;
    char *pDst = pOuter->AddTempString(nullptr, buffLen);
	int len = strlen( pFmt );

	if ( len < 2 ) 
	{
        pEngine->SetError( ForthError::kBadParameter, " failure in format - format string too short" );
	}
	else
	{
		char lastChar = pFmt[ len - 1 ];
		switch ( tolower( lastChar ) )
		{
			case 'a': case 'e': case 'f': case 'g':
			{
				float fval = FPOP;
				double dval = (double) fval;
				SNPRINTF( pDst, buffLen, pFmt, dval );
				break;
			}
			default:
			{
				int32_t val = SPOP;
				SNPRINTF( pDst, buffLen, pFmt, val );
			}
		}

#ifdef TRACE_PRINTS
		SPEW_PRINTS( "printed %s\n", pDst );
#endif
		SPUSH( (cell) pDst );
	}
}

FORTHOP( format64Op )
{
    NEEDS(2);

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pFmt = (char *) (SPOP);
    int buffLen = 64;
    char *pDst = pOuter->AddTempString(nullptr, buffLen);
	int len = strlen( pFmt );

	if ( len < 2 ) 
	{
        pEngine->SetError( ForthError::kBadParameter, " failure in 2format - format string too short" );
	}
	else
	{
		char lastChar = pFmt[ len - 1 ];
		switch ( tolower( lastChar ) )
		{
			case 'a': case 'e': case 'f': case 'g':
			{
				double dval = DPOP;
				SNPRINTF( pDst, buffLen, pFmt, dval );
				break;
			}
			default:
			{
                int64_t val;
#if defined(FORTH64)
                val = SPOP;
#else
                stackInt64 sval;
                LPOP(sval);
                val = sval.s64;
#endif
				SNPRINTF( pDst, buffLen, pFmt, val );
			}
		}

#ifdef TRACE_PRINTS
		SPEW_PRINTS( "printed %s\n", pDst );
#endif
		SPUSH( (cell) pDst );
	}
}

FORTHOP(representOp)
{
    char buffer[48];
    char fmtString[16];
    //cell precision = SPOP;
    cell bufferLen = SPOP;
    char* pDstBuffer = (char*)(SPOP);
    double val = DPOP;

    cell exponent = 0;
    cell isValid = -1;
    cell isNegative = 0;

    if (val < 0.0)
    {
        val = -val;
        isNegative = -1;
    }
    //SNPRINTF(fmtString, sizeof(fmtString) - 1, "%%.%de", precision);
    SNPRINTF(fmtString, sizeof(fmtString) - 1, "%%.%de", bufferLen - 1);

    buffer[0] = '\0';
    SNPRINTF(buffer, sizeof(buffer) - 1, fmtString, val);
    if (!isdigit(buffer[0]))
    {
        isValid = 0;
        // this is something like '+infinity' or 'nan'
        memcpy(pDstBuffer, &(buffer[0]), min(bufferLen, strlen(buffer)));
    }
    else
    {
        int ix = 0;
        int outIx = 0;
        while (buffer[ix] != '\0')
        {
            char c = buffer[ix];
            if (isdigit(c))
            {
                if (outIx < bufferLen)
                {
                    pDstBuffer[outIx++] = c;
                }
            }
            else if (tolower(c) == 'e')
            {
                // get the exponent
                int64_t exp;
                sscanf(&(buffer[ix + 1]), "%lld", &exp);
                exponent = (cell)exp;
                break;
            }
            else if (c != '.')
            {
                // unexpected character found
                break;
            }
            ix++;
        }
    }

    exponent++;
    SPUSH(exponent);
    SPUSH(isNegative);
    SPUSH(isValid);
}

FORTHOP(scanIntOp)
{
    int base = SPOP;
    int numChars = SPOP;
    const char* pToken = (const char *)(SPOP);

    if (numChars <= 0)
    {
        numChars = strlen(pToken);
    }
    int value = 0;
    int digitsFound = 0;
    int digit;
    int isValid = -1;
    bool isNegative = false;

    if (numChars > 1)
    {
        if (*pToken == '-')
        {
            isNegative = true;
            ++pToken;
            --numChars;
        }
    }

    for (int i = 0; i < numChars; ++i)
    {
        char c = *pToken++;
        if ((c >= '0') && (c <= '9'))
        {
            digit = c - '0';
            digitsFound++;
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            digit = 10 + (c - 'A');
            digitsFound++;
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            digit = 10 + (c - 'a');
            digitsFound++;
        }
        else
        {
            isValid = 0;
            break;
        }

        if (digit >= base)
        {
            // invalid digit for current base
            isValid = 0;
            break;
        }
        value = (value * base) + digit;
    }
    
    if (digitsFound == 0)
    {
        isValid = false;
    }

    if (isNegative)
    {
        value *= -1;
    }

    if (isValid)
    {
        SPUSH(value);
    }
    SPUSH(isValid);
}

FORTHOP(scanLongOp)
{
    int base = SPOP;
    int numChars = SPOP;
    const char* pToken = (const char *)(SPOP);

    if (numChars <= 0)
    {
        numChars = strlen(pToken);
    }
    uint64_t uvalue = 0L;

    int digitsFound = 0;
    int digit;
    int isValid = -1;
    bool isNegative = false;

    if (numChars > 1)
    {
        if (*pToken == '-')
        {
            isNegative = true;
            ++pToken;
            --numChars;
        }
    }

    for (int i = 0; i < numChars; ++i)
    {
        char c = *pToken++;
        if ((c >= '0') && (c <= '9'))
        {
            digit = c - '0';
            digitsFound++;
        }
        else if ((c >= 'A') && (c <= 'Z'))
        {
            digit = 10 + (c - 'A');
            digitsFound++;
        }
        else if ((c >= 'a') && (c <= 'z'))
        {
            digit = 10 + (c - 'a');
            digitsFound++;
        }
        else
        {
            isValid = 0;
            break;
        }

        if (digit >= base)
        {
            // invalid digit for current base
            isValid = 0;
            break;
        }
        uvalue = (uvalue * base) + digit;
    }

    if (digitsFound == 0)
    {
        isValid = false;
    }

    int64_t value = isNegative ? (uvalue * -1) : uvalue;
    if (isValid)
    {
#if defined(FORTH64)
        SPUSH(value);
#else
        stackInt64 sval;
        sval.s64 = value;
        LPUSH(sval);
#endif
    }
    SPUSH(isValid);
}

FORTHOP(addTempStringOp)
{
    NEEDS(2);

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    int32_t stringSize = SPOP;
    char* pSrc = (char *)(SPOP);
    char *pDst = pOuter->AddTempString(pSrc, stringSize);
    SPUSH((cell)pDst);
}

#ifdef PRINTF_SUBS_IN_ASM
extern void fprintfSub( CoreState* pCore );
extern void snprintfSub(CoreState* pCore);
extern void fscanfSub(CoreState* pCore);
extern void sscanfSub( CoreState* pCore );
#else

void fprintfSub( CoreState* pCore )
{
    cell a[8];
    // TODO: assert if numArgs > 8
    int32_t numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* outfile = (FILE *) SPOP;
    cell result = fprintf( outfile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
    SPUSH(result);
}

void snprintfSub( CoreState* pCore )
{
    cell a[8];
    // TODO: assert if numArgs > 8
    int32_t numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = SPOP;
    }
    const char* fmt = (const char *) SPOP;
    size_t maxLen = (size_t) SPOP;
    char* outbuff = (char *) SPOP;

#if defined(WINDOWS_BUILD)
    HRESULT hres = StringCchPrintf(outbuff, maxLen, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
    cell result = (hres == S_OK) ? strlen(outbuff) : -1;
#else
    cell result = snprintf(outbuff, maxLen, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
#endif
    SPUSH(result);
}

void fscanfSub( CoreState* pCore )
{
    void* a[8];
    // TODO: assert if numArgs > 8
    int32_t numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    FILE* infile = (FILE *) SPOP;
    cell result = fscanf( infile, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
    SPUSH(result);
}

void sscanfSub( CoreState* pCore )
{
    void* a[8];
    // TODO: assert if numArgs > 8
    cell numArgs = SPOP;
    for ( int i = numArgs - 1; i >= 0; --i )
    {
        a[i] = (void *) SPOP;
    }
    const char* fmt = (const char *) SPOP;
    char* inbuff = (char *) SPOP;
    cell result = sscanf( inbuff, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7] );
    SPUSH(result);
}

cell oStringFormatSub(CoreState* pCore, char* pBuffer, int bufferSize)
{
    cell a[8];
	// TODO: assert if numArgs > 8
	cell numArgs = SPOP;
    for (cell i = numArgs - 1; i >= 0; --i)
	{
		a[i] = SPOP;
	}
	const char* fmt = (const char *)SPOP;
    cell result = 0;
    switch (numArgs)
    {
    case 0:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt));
        break;
    case 1:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0]));
        break;
    case 2:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1]));
        break;
    case 3:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2]));
        break;
    case 4:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2], a[3]));
        break;
    case 5:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2], a[3], a[4]));
        break;
    case 6:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2], a[3], a[4], a[5]));
        break;
    case 7:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6]));
        break;
    default:
        result = (int)(SNPRINTF(pBuffer, bufferSize, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]));
        break;
    }
#ifdef WINDOWS_BUILD
    if (result == S_OK)
    {
        result = ::strnlen(pBuffer, bufferSize);
    }
    else
    {
        result = -1;
    }
#endif

    return result;
}

#endif

FORTHOP( fprintfOp )
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fprintfSub( pCore );
}

FORTHOP(snprintfOp)
{
    // TOS: N argN ... arg1 formatStr bufferLen bufferPtr       (arg1 to argN are optional)
    snprintfSub(pCore);
}

FORTHOP(fscanfOp)
{
    // TOS: N argN ... arg1 formatStr filePtr       (arg1 to argN are optional)
    fscanfSub( pCore );
}

FORTHOP( sscanfOp )
{
    // TOS: N argN ... arg1 formatStr bufferPtr       (arg1 to argN are optional)
    sscanfSub( pCore );
}

FORTHOP(atoiOp)
{
	char *buff = (char *)SPOP;
	int result = atoi(buff);
	SPUSH(result);
}

FORTHOP(atofOp)
{
	char *buff = (char *)SPOP;
	double result = atof(buff);
	DPUSH(result);
}

FORTHOP(printStrOp)
{
    NEEDS(1);
    const char *buff = (const char *) SPOP;
	if ( buff == NULL )
	{
		buff = "<<<NULL>>>";
	}
#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %s\n", buff );
#endif

    CONSOLE_STRING_OUT( buff );
}

FORTHOP( printBytesOp )
{
    NEEDS(2);
    int32_t count = SPOP;
    const char* pChars = (const char*)(SPOP);
	if ( pChars == NULL )
	{
		pChars = "<<<NULL>>>";
		count = strlen( pChars );
	}
    CONSOLE_BYTES_OUT( pChars, count );
}

FORTHOP( printCharOp )
{
    NEEDS(1);
    char ch = static_cast<char>(SPOP);

#ifdef TRACE_PRINTS
    SPEW_PRINTS( "printed %d\n", ch );
#endif

    CONSOLE_CHAR_OUT( ch );
}

FORTHOP(print4Op)
{
	NEEDS(1);
	int chars = SPOP;
	const char* pChars = (const char*)&chars;
	CONSOLE_BYTES_OUT(pChars, 4);
}

FORTHOP(print8Op)
{
	NEEDS(2);
	stackInt64 chars;
	LPOP(chars);
	const char* pChars = (const char*)&chars;
	CONSOLE_BYTES_OUT(pChars, 8);
}

FORTHOP(printSpaceOp)
{
    CONSOLE_CHAR_OUT( ' ' );
}

FORTHOP( printNewlineOp )
{
    CONSOLE_CHAR_OUT( '\n' );
}

FORTHOP( baseOp )
{
    SPUSH( (cell) GET_BASE_REF );
}

FORTHOP( octalOp )
{
    *GET_BASE_REF = 8;
}

FORTHOP( decimalOp )
{
    *GET_BASE_REF = 10;
}

FORTHOP( hexOp )
{
    *GET_BASE_REF = 16;
}

FORTHOP( printDecimalSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintSignedDecimal );
}

FORTHOP( printAllSignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllSigned );
}

FORTHOP( printAllUnsignedOp )
{
    SET_PRINT_SIGNED_NUM_MODE( kPrintAllUnsigned );
}

FORTHOP( getConsoleOutOp )
{
    Engine *pEngine = GET_ENGINE;

	pEngine->PushConsoleOut( pCore );
}

FORTHOP( getDefaultConsoleOutOp )
{
    Engine *pEngine = GET_ENGINE;

	pEngine->PushDefaultConsoleOut( pCore );
}

FORTHOP( setConsoleOutOp )
{
    Engine *pEngine = GET_ENGINE;

	ForthObject conoutObject;
	POP_OBJECT( conoutObject );
	pEngine->SetConsoleOut( pCore, conoutObject );
}

FORTHOP(getErrorOutOp)
{
    Engine* pEngine = GET_ENGINE;

    pEngine->PushErrorOut(pCore);
}

FORTHOP(setErrorOutOp)
{
    Engine* pEngine = GET_ENGINE;

    ForthObject conoutObject;
    POP_OBJECT(conoutObject);
    pEngine->SetErrorOut(pCore, conoutObject);
}

FORTHOP( resetConsoleOutOp )
{
    Engine *pEngine = GET_ENGINE;

	pEngine->ResetConsoleOut( *pCore );
}

FORTHOP(toupperOp)
{
	int c = SPOP;
	SPUSH(toupper(c));
}

FORTHOP(isupperOp)
{
    int c = SPOP;
    SPUSH(isupper(c));
}

FORTHOP(isspaceOp)
{
    int c = SPOP;
    SPUSH(isspace(c));
}

FORTHOP(isalphaOp)
{
    int c = SPOP;
    SPUSH(isalpha(c));
}

FORTHOP(isgraphOp)
{
    int c = SPOP;
    SPUSH(isgraph(c));
}

FORTHOP(tolowerOp)
{
	int c = SPOP;
	SPUSH(tolower(c));
}

FORTHOP(islowerOp)
{
	int c = SPOP;
	SPUSH(islower(c));
}


//##############################
//
//  File ops
//
FORTHOP( opendirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    cell result = (cell) (pCore->pFileFuncs->openDir( pPath ));
	// result is actually a DIR*
    SPUSH( result );
}

FORTHOP( readdirOp )
{
    NEEDS(2);
    void* pResult = (void *)SPOP;
    void* pDir = (void *)SPOP;
    cell result = (cell)(pCore->pFileFuncs->readDir(pDir, pResult));
	// result is actually a struct dirent*
    SPUSH(result == 0 ? 0 : -1);
}

FORTHOP( closedirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    cell result = pCore->pFileFuncs->closeDir( pDir );
    SPUSH( result );
}

FORTHOP( rewinddirOp )
{
    NEEDS(1);
	void* pDir = (void *) SPOP;
    pCore->pFileFuncs->rewindDir( pDir );
}


FORTHOP( removeOp )
{
	const char* pFilename = (const char *) (SPOP);
    cell result = pCore->pFileFuncs->fileRemove( pFilename );
    SPUSH( result );
}

FORTHOP( _dupOp )
{
	int fileHandle = SPOP;
    cell result = pCore->pFileFuncs->fileDup( fileHandle );
    SPUSH( result );
}

FORTHOP( _dup2Op )
{
	int dstFileHandle = SPOP;
	int srcFileHandle = SPOP;
    int result = pCore->pFileFuncs->fileDup2( srcFileHandle, dstFileHandle );
    SPUSH( result );
}

FORTHOP( _filenoOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileNo( pFile );
    SPUSH( result );
}

FORTHOP( fflushOp )
{
	FILE* pFile = (FILE *) SPOP;
	int result = pCore->pFileFuncs->fileFlush( pFile );
    SPUSH( result );
}

FORTHOP( errnoOp)
{
    SPUSH( errno );
}

FORTHOP( strerrorOp)
{
	int32_t errVal = SPOP;
    SPUSH( reinterpret_cast<cell>(strerror(errVal)) );
}

FORTHOP( shellRunOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->runSystem( (char *) SPOP );
    SPUSH( result );
}

FORTHOP( chdirOp )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->setWorkDir( (const char *) SPOP );
    SPUSH( result );
}

FORTHOP( mkdirOp )
{
    NEEDS(2);
	int mode = SPOP;
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->makeDir( pPath, mode );
    SPUSH( result );
}

FORTHOP( rmdirOp )
{
    NEEDS(1);
	const char* pPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->removeDir( pPath );
    SPUSH( result );
}

FORTHOP( renameOp )
{
    NEEDS(2);
	const char* pDstPath = (const char*) SPOP;
	const char* pSrcPath = (const char*) SPOP;
    int result = pCore->pFileFuncs->renameFile( pSrcPath, pDstPath );
    SPUSH( result );
}

FORTHOP(statOp)
{
    NEEDS(2);
    struct stat* pDstStat = (struct stat*)SPOP;
    const char* pSrcPath = (const char*)SPOP;
    int result = stat(pSrcPath, pDstStat);
    SPUSH(result);
}

FORTHOP(fstatOp)
{
    NEEDS(2);
    struct stat* pDstStat = (struct stat*)SPOP;
    int fileHandle = SPOP;
    int result = fstat(fileHandle, pDstStat);
    SPUSH(result);
}

FORTHOP( sourceIdOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    SPUSH( pShell->GetInput()->Top()->GetSourceID() );
}

FORTHOP( saveInputOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    cell* pData = pShell->GetInput()->Top()->GetInputState();
    int32_t nItems = *pData;
    for ( int i = 0; i < nItems; i++ )
    {
        SPUSH( pData[nItems - i] );
    }
    SPUSH( nItems );
}

FORTHOP(restoreInputOp)
{
    Shell *pShell = GET_ENGINE->GetShell();
    cell* pSP = GET_SP;
    int32_t nItems = *pSP;
    pShell->GetInput()->Top()->SetInputState(pSP);
    SET_SP(pSP + (nItems + 1));
}

FORTHOP(getEnvironmentVarOp)
{
    Shell *pShell = GET_ENGINE->GetShell();
    const char* pVarName = (const char *)(SPOP);
    SPUSH((cell)(pShell->GetEnvironmentVar(pVarName)));
}

FORTHOP(getEnvironmentPairOp)
{
    Shell *pShell = GET_ENGINE->GetShell();
    const char* pName = nullptr;
    const char* pValue = nullptr;
    cell varIndex = SPOP;

    const std::vector<std::string>& names = pShell->GetEnvironmentVarNames();
    const std::vector<std::string>& values = pShell->GetEnvironmentVarValues();
    
    if (varIndex < names.size())
    {
        pName = names[varIndex].c_str();
        pValue = values[varIndex].c_str();
    }

    SPUSH((cell)pName);
    SPUSH((cell)pValue);
}

FORTHOP(getCwdOp)
{
	int maxChars = (SPOP) - 1;
	char* pDst = (char *)(SPOP);
	pDst[maxChars] = '\0';
#if defined(WINDOWS_BUILD)
    DWORD result = GetCurrentDirectory(maxChars, pDst);
	if (result == 0)
	{
		pDst[0] = '\0';
	}
#elif defined(LINUX) || defined(MACOSX)
	if (getcwd(pDst, maxChars) == NULL)
	{
		// failed to get current directory
		strcpy(pDst, ".");
	}
#endif
}

//##############################
//
// Default input/output file ops
//

FORTHOP( stdinOp )
{
    cell result = (cell) (pCore->pFileFuncs->getStdIn());
    SPUSH( result );
}

FORTHOP( stdoutOp )
{
    cell result = (cell) (pCore->pFileFuncs->getStdOut());
    SPUSH( result );
}

FORTHOP( stderrOp )
{
    cell result = (cell) (pCore->pFileFuncs->getStdErr());
    SPUSH( result );
}

FORTHOP( dstackOp )
{
    cell *pSP = GET_SP;
    int nItems = GET_SDEPTH;
    int i;
    char buff[64];

    SNPRINTF( buff, sizeof(buff), "stack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_CHAR_OUT( ' ' );
#if defined(FORTH64)
        printLongNumInCurrentBase(pCore, *pSP++);
#else
        printNumInCurrentBase(pCore, *pSP++);
#endif
    }
    CONSOLE_CHAR_OUT( '\n' );
}


FORTHOP( drstackOp )
{
    cell *pRP = GET_RP;
    int nItems = GET_RDEPTH;
    int i;
    char buff[64];

    SNPRINTF( buff, sizeof(buff), "rstack[%d]:", nItems );
    CONSOLE_STRING_OUT( buff );
    for ( i = 0; i < nItems; i++ )
    {
        CONSOLE_STRING_OUT( " " );
        printNumInCurrentBase( pCore, *pRP++ );
    }
    CONSOLE_CHAR_OUT( '\n' );
}


FORTHOP( describeOp )
{
    char buff[512];


    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pSym = pOuter->GetNextSimpleToken();
	strcpy( buff, pSym );

	// allow stuff like CLASS.METHOD or VOCAB:OP
	char* pMethod = strchr( buff, '.' );
	if (pMethod == NULL)
	{
		pMethod = strchr(buff, ':');
	}
	if ( pMethod != NULL )
	{
		if ((pMethod == &(buff[0])) || (pMethod[1] == '\0'))
		{
			// don't split if ./: is at beginning or end of word
			pMethod = NULL;
		}
		else
		{
			// replace ./: with a null to split into 2 strings
			*pMethod++ = '\0';
		}
	}
    TypesManager* pManager = TypesManager::GetInstance();
    StructVocabulary* pVocab = pManager->GetStructVocabulary( buff );
	bool verbose = (GET_VAR_OPERATION != VarOperation::kVarDefaultOp);

    if ( pVocab )
    {
		if ( pMethod != NULL )
		{
			Vocabulary* pFoundVocab = NULL;
            forthop* pEntry = pVocab->FindSymbol( pMethod );
			if (pEntry != NULL) 
			{
                forthop typeCode = pEntry[1];
				if (pVocab->IsClass())
				{
					ClassVocabulary* pClassVocab = (ClassVocabulary*)pVocab;
					if (CODE_IS_METHOD(typeCode))
					{
						// TODO: support secondary interfaces
						pOuter->DescribeOp(pSym, pClassVocab->GetInterface(0)->GetMethod(pEntry[0]), pEntry[1]);
					}
					else if (CODE_TO_BASE_TYPE(typeCode) == BaseType::kUserDefinition)
					{
						pOuter->DescribeOp(pSym, pEntry[0], pEntry[1]);
					}
					else
					{
						pVocab->PrintEntry(pEntry);
					}
				}
				else
				{
					if (CODE_TO_BASE_TYPE(typeCode) == BaseType::kUserDefinition)
					{
						pOuter->DescribeOp(pSym, pEntry[0], pEntry[1]);
					}
					else
					{
						pVocab->PrintEntry(pEntry);
					}
				}
			}
			else
			{
				SNPRINTF( buff, sizeof(buff), "Failed to find method %s\n", pSym );
				CONSOLE_STRING_OUT( buff );
			}
		}
		else
		{
			// show structure vocabulary entries
			while ( pVocab )
			{
                if (pVocab->IsClass())
                {
                    forthop* pMethods = ((ClassVocabulary *)pVocab)->GetMethods();
                    SNPRINTF(buff, sizeof(buff), "class vocabulary %s:  methods at 0x%08x, size %d\n",
                        pVocab->GetName(), static_cast<int>(reinterpret_cast<intptr_t>(pMethods)), pVocab->GetSize());
                }
                else
                {
                    SNPRINTF(buff, sizeof(buff), "struct vocabulary %s:\n", pVocab->GetName() );
                }
				CONSOLE_STRING_OUT( buff );
				char quit = ShowVocab( pEngine->GetCoreState(), pVocab, !verbose );
				if ( quit == 'q' )
				{
					break;
				}
				else
				{
					pVocab = pVocab->BaseVocabulary();
				}
			}
		}
    }
    else
    {
        pOuter->DescribeSymbol( pSym );
    }
	CLEAR_VAR_OPERATION;
}

FORTHOP( describeAtOp )
{
	NEEDS( 2 );
	char* pString = reinterpret_cast<char *>(SPOP);
    forthop* pOp = reinterpret_cast<forthop* >(SPOP);
    //TODO: are length fields int32 or cell?
    int32_t* pLengths = reinterpret_cast<int32_t* >(pString) - 2;
	// pLengths[0] is max length, pLength[1] is current length
    pLengths[1] = 0;
	if ( pLengths[0] < 1 )
	{
		return;
	}
    *pString = '\0';

    Engine *pEngine = GET_ENGINE;

	pEngine->DescribeOp( pOp, pString, pLengths[0], true );
	pLengths[1] = strlen( pString );
	CLEAR_VAR_OPERATION;
}

FORTHOP( DLLVocabularyOp )
{
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary *pDefinitionsVocab = pOuter->GetDefinitionVocabulary();
    // get next symbol, add it to vocabulary with type "user op"
    char* pDLLOpName = pOuter->AddTempString(pOuter->GetNextSimpleToken());
    pOuter->StartOpDefinition( pDLLOpName );
    char* pDLLName = pOuter->GetNextSimpleToken();
    DLLVocabulary* pVocab = new DLLVocabulary( pDLLOpName,
                                                         pDLLName,
                                                         NUM_FORTH_VOCAB_VALUE_LONGS,
                                                         512,
                                                         GET_DP,
                                                         Vocabulary::GetEntryValue( pDefinitionsVocab->GetNewestEntry() ) );
    pOuter->CompileBuiltinOpcode( OP_DO_VOCAB );
    pVocab->UnloadDLL();
    pVocab->LoadDLL();
    pOuter->CompileCell((cell) pVocab);
}

FORTHOP( addDLLEntryOp )
{
    NEEDS( 2 );

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pProcName = (char *) SPOP;
    DLLVocabulary* pVocab = (DLLVocabulary *) (pOuter->GetDefinitionVocabulary());
    if (pVocab->GetType() != VocabularyType::kDLL)
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( ForthError::kBadParameter, " is not a DLL vocabulary - addDllEntry" );
    }
    uint32_t numArgs = SPOP;
	pVocab->AddEntry(pProcName, pProcName, numArgs);
}

FORTHOP(addDLLEntryExOp)
{
	NEEDS(2);

	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pProcName = (char *)SPOP;
	char* pEntryName = (char *)SPOP;
	DLLVocabulary* pVocab = (DLLVocabulary *)(pOuter->GetDefinitionVocabulary());
	if (pVocab->GetType() != VocabularyType::kDLL)
	{
		pEngine->AddErrorText(pVocab->GetName());
		pEngine->SetError(ForthError::kBadParameter, " is not a DLL vocabulary - addDllEntry");
	}
	uint32_t numArgs = SPOP;
	pVocab->AddEntry(pProcName, pEntryName, numArgs);
}

FORTHOP(DLLVoidOp)
{
	NEEDS( 0 );

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    DLLVocabulary* pVocab = (DLLVocabulary *) (pOuter->GetDefinitionVocabulary());
    if (pVocab->GetType() != VocabularyType::kDLL)
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( ForthError::kBadParameter, " is not a DLL vocabulary - DLLVoidOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_VOID );
	}
}

FORTHOP( DLLLongOp )
{
	NEEDS( 0 );

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    DLLVocabulary* pVocab = (DLLVocabulary *) (pOuter->GetDefinitionVocabulary());
    if (pVocab->GetType() != VocabularyType::kDLL)
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( ForthError::kBadParameter, " is not a DLL vocabulary - DLLLongOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_RETURN_64BIT );
	}
}

FORTHOP( DLLStdCallOp )
{
	NEEDS( 0 );

    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    DLLVocabulary* pVocab = (DLLVocabulary *) (pOuter->GetDefinitionVocabulary());
    if (pVocab->GetType() != VocabularyType::kDLL)
    {
        pEngine->AddErrorText( pVocab->GetName() );
        pEngine->SetError( ForthError::kBadParameter, " is not a DLL vocabulary - DLLStdCallOp" );
    }
	else
	{
		pVocab->SetFlag( DLL_ENTRY_FLAG_STDCALL );
	}
}

struct userCallbackData {
    void*               pUserData;
    CoreState*     pCore;
    forthop             callbackOp;
    uint32_t            numArgs;
};

cell genericCallback(struct userCallbackData* pCallbackData, cell arg1)
{
    // this callback is shared by many user defined ops to interface to DLL routines which need a callback
    // the DLL callback signature must start with a void* userdata parameter (pUserData)
    // pUserData points to a data block which holds:
    // cell     userData pointer supplied by the user defined op
    // cell     pointer to CoreState for fiber which is getting the callback
    // forthop  user-defined callback op
    // int      number of arguments callback has
    cell result = 0;
    CoreState* pCore = pCallbackData->pCore;

    Engine* pEngine = (Engine*)(pCore->pEngine);

    cell* args = &arg1;
    for (int i = 0; i < pCallbackData->numArgs; ++i)
    {
        SPUSH(*args);
        args++;
    }
    SPUSH((cell)pCallbackData->numArgs);
    SPUSH((cell)(pCallbackData->pUserData));

    pEngine->FullyExecuteOp(pCore, pCallbackData->callbackOp);

    result = SPOP;
    return result;
}

FORTHOP(createCallbackOp)
{
    userCallbackData* pCB = (userCallbackData*)ALLOCATE_BYTES(sizeof(userCallbackData));
    pCB->numArgs = (uint32_t)(SPOP);
    pCB->pUserData = (void*)(SPOP);
    pCB->callbackOp = (forthop)(SPOP);
    pCB->pCore = pCore;

    SPUSH((cell)genericCallback);
    SPUSH((cell)pCB);
}

FORTHOP(destroyCallbackOp)
{
    void* pCB = (void*)(SPOP);
    DEALLOCATE_BYTES(pCB, sizeof(userCallbackData));
}

FORTHOP( blwordOp )
{
    NEEDS( 0 );
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
	char *pSrc = pShell->GetNextSimpleToken();
	char *pDst = pOuter->AddTempString(pSrc);
    SPUSH( (cell) pDst );
}

FORTHOP( strWordOp )
{
    NEEDS( 1 );
	Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Shell *pShell = pEngine->GetShell();
    char delim = (char) (SPOP);
    // leave an unused byte below string so string len can be stuck there in ANSI compatability mode
	char *pSrc = pShell->GetToken( delim, false );
	char *pDst = pOuter->AddTempString(pSrc);
    SPUSH( (cell) pDst );
}

// has precedence!
// comment to end of line using "\"
FORTHOP( slashCommentOp )
{
    NEEDS( 0 );
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->GetInput()->Top()->SeekToLineEnd();
}

// has precedence!
// inline comment using parens
// strictly for ease of porting old style forth code
FORTHOP( parenCommentOp )
{
    NEEDS( 0 );
    Shell *pShell = GET_ENGINE->GetShell();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
	while ( strchr( pBuffer, ')' ) == NULL )
	{
        if ( pShell->GetInput()->Top()->IsInteractive() )
        {
            // in interactive mode, comment can't span lines
            pShell->GetToken( 0 );
            return;
        }
		const char *pLine = pShell->GetInput()->GetLine("");
		if ( pLine == NULL )
		{
			break;
		}
		pBuffer = pShell->GetInput()->GetBufferPointer();
	}

	char *pToken = pShell->GetToken( ')' );
}

// fake variable used to turn on/off features
FORTHOP( featuresOp )
{
    NEEDS( 1 );
    Engine* pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

    switch ( GET_VAR_OPERATION )
    {
    case VarOperation::kVarSet:
        pOuter->SetFeatures( SPOP );
        break;

    case VarOperation::kVarDefaultOp:
    case VarOperation::kVarGet:
        SPUSH(pOuter->GetFeatures() );
        break;

    case VarOperation::kVarRef:
        SPUSH( (cell)(&(pOuter->GetFeatures())) );
        break;

    case VarOperation::kVarSetPlus:
        pOuter->SetFeature( SPOP );
        break;

    case VarOperation::kVarSetMinus:
        pOuter->ClearFeature( SPOP );
        break;

    default:
        pEngine->SetError( ForthError::kBadVarOperation, "features: unkown var operation" );
        break;
    }
    CLEAR_VAR_OPERATION;
}

FORTHOP( dumpCrashStateOp )
{
	Engine* pEngine = GET_ENGINE;
	pEngine->DumpCrashState();
}

FORTHOP( sourceOp )
{
    InputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (cell) (pInput->Top()->GetReportedBufferBasePointer()) );
    SPUSH( pInput->GetWriteOffset() );
}

FORTHOP( getInOffsetPointerOp )
{
    InputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    SPUSH( (cell) (pInput->GetReadOffsetPointer()) );
}

FORTHOP( fillInBufferOp )
{
    InputStack* pInput = GET_ENGINE->GetShell()->GetInput();
    char* pBuffer = (char *) (SPOP);
    SPUSH( (cell) (pInput->GetLine(pBuffer)) );
}

FORTHOP( keyOp )
{
	int key = getch();
    SPUSH( key );
}

FORTHOP( keyHitOp )
{
	int keyHit = kbhit();
    SPUSH( keyHit );
}

FORTHOP(vocChainHeadOp)
{
	SPUSH((cell)(Vocabulary::GetVocabularyChainHead()));
}

FORTHOP(vocChainNextOp)
{
	Vocabulary* pVocab = (Vocabulary *)(SPOP);
	SPUSH((cell)(pVocab->GetNextChainVocabulary()));
}

FORTHOP( turboOp )
{
    GET_ENGINE->ToggleFastMode();
    SET_STATE( OpResult::kDone );
}

FORTHOP( errorOp )
{
    Engine *pEngine = GET_ENGINE;
    pEngine->SetError( ForthError::kUserDefined, (char *) (SPOP) );
}

FORTHOP( addErrorTextOp )
{
    Engine *pEngine = GET_ENGINE;
    pEngine->AddErrorText( (char *) (SPOP) );
}

FORTHOP( timeOp )
{
    time_t rawtime;
    time( &rawtime );
    DPUSH( *((double *) &rawtime) );
}

FORTHOP( strftimeOp )
{
    double dtime = DPOP;
    time_t rawtime = *((time_t*) &dtime);
    struct tm * timeinfo;
    const char* fmt = (const char *)(SPOP);
    size_t bufferSize = (size_t)(SPOP);
    char* buffer = (char *)(SPOP);

    timeinfo = localtime ( &rawtime );
    // Www Mmm dd yyyy (weekday, month, day, year)
	errno = 0;
    strftime(buffer, bufferSize, fmt, timeinfo);
	if (errno)
	{
		GET_ENGINE->SetError(ForthError::kBadParameter);
	}
}

FORTHOP( splitTimeOp )
{
    double dtime = DPOP;
    time_t rawtime = *((time_t*)&dtime);
    struct tm* brokenDownTime = localtime(&rawtime);
    SPUSH( brokenDownTime->tm_year + 1900 );
    SPUSH( brokenDownTime->tm_mon + 1 );
    SPUSH( brokenDownTime->tm_mday );
    SPUSH( brokenDownTime->tm_hour );
    SPUSH( brokenDownTime->tm_min );
    SPUSH( brokenDownTime->tm_sec );
}

FORTHOP(unsplitTimeOp)
{
    time_t rawtime;
    struct tm brokenDownTime;

    brokenDownTime.tm_sec = SPOP;
    brokenDownTime.tm_min = SPOP;
    brokenDownTime.tm_hour = SPOP;
    brokenDownTime.tm_mday = SPOP;
    brokenDownTime.tm_mon = SPOP - 1;
    brokenDownTime.tm_year = SPOP - 1900;

    rawtime = mktime(&brokenDownTime);
    double dtime = *((double *)&rawtime);
    DPUSH(dtime);
}

FORTHOP( millitimeOp )
{
    Engine *pEngine = GET_ENGINE;
    SPUSH( (cell) pEngine->GetElapsedTime() );
}

FORTHOP( millisleepOp )
{
#if defined(WINDOWS_BUILD)
    DWORD dwMilliseconds = (DWORD)(SPOP);
    ::Sleep( dwMilliseconds );
#else
    int milliseconds = SPOP;
#if defined(LINUX) || defined(MACOSX)
    usleep( milliseconds * 1000 );
#endif
#endif
}


//##############################
//
//  computation utilities - randoms, hashing, sorting and searcing
//

FORTHOP( randOp )
{
    SPUSH( rand() );
}

FORTHOP( srandOp )
{
    srand( (uint32_t) (SPOP) );
}

// Paul Hsieh's fast hash    http://www.azillionmonkeys.com/qed/hash.html
#define get16bits(d) ((((uint32_t)(((const unsigned char *)(d))[1])) << 8)\
						+(uint32_t)(((const unsigned char *)(d))[0]) )

uint32_t
SuperFastHash (const char * data, int len, uint32_t hash)
{

	uint32_t tmp;
	int rem;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (unsigned short);
		hash  += hash >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: hash += get16bits (data);
			hash ^= hash << 16;
			hash ^= data[sizeof (unsigned short)] << 18;
			hash += hash >> 11;
			break;
		case 2: hash += get16bits (data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
		case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
}

// data len hashIn ... hashOut
FORTHOP( phHashContinueOp )
{
    uint32_t hashVal = SPOP;
    int len = SPOP;
    const char* pData = (const char*) (SPOP);
    hashVal = SuperFastHash( pData, len, hashVal );
    SPUSH( hashVal );
}

// data len ... hashOut
FORTHOP(phHashOp)
{
	uint32_t hashVal = 0;
	int len = SPOP;
	const char* pData = (const char*)(SPOP);
	hashVal = SuperFastHash(pData, len, hashVal);
	SPUSH(hashVal);
}

// Fowler/Noll/Vo hash from http://www.isthe.com/chongo/src/fnv/hash_32.c and http://www.isthe.com/chongo/src/fnv/fnv.h
#define FNV_32_PRIME ((Fnv32_t)0x01000193)
#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)
typedef uint32_t Fnv32_t;

Fnv32_t
fnv_32_buf(const void *buf, size_t len, Fnv32_t hval)
{
	unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
	unsigned char *be = bp + len;		/* beyond end of buffer */

	/*
	* FNV-1 hash each octet in the buffer
	*/
	while (bp < be) {

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
#endif

		/* xor the bottom with the current octet */
		hval ^= (Fnv32_t)*bp++;
	}

	/* return our new hash value */
	return hval;
}

// data len ... hashOut
FORTHOP(fnvHashOp)
{
	uint32_t hashVal = FNV1_32_INIT;
	int len = SPOP;
	const char* pData = (const char*)(SPOP);
	hashVal = fnv_32_buf(pData, len, hashVal);
	SPUSH(hashVal);
}

// data len hashIn ... hashOut
FORTHOP(fnvHashContinueOp)
{
	uint32_t hashVal = SPOP;
	int len = SPOP;
	const char* pData = (const char*)(SPOP);
	hashVal = fnv_32_buf(pData, len, hashVal);
	SPUSH(hashVal);
}

namespace
{
    struct qsInfo
    {
        void*   temp;
        int     offset;
        int     elementSize;
        int     compareSize;
        int     (*compare)( const void* a, const void* b, int );
    };

    int u8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned char *) pA);
        int b = (int) *((unsigned char *) pB);
        return a - b;
    }

    int s8Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((char *) pA);
        int b = (int) *((char *) pB);
        return a - b;
    }

    int u16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((unsigned short *) pA);
        int b = (int) *((unsigned short *) pB);
        return a - b;
    }

    int s16Compare( const void* pA, const void* pB, int )
    {
        int a = (int) *((short *) pA);
        int b = (int) *((short *) pB);
        return a - b;
    }

    int u32Compare( const void* pA, const void* pB, int )
    {
        uint32_t a = *((uint32_t *) pA);
        uint32_t b = *((uint32_t *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s32Compare( const void* pA, const void* pB, int )
    {
        int a = *((int *) pA);
        int b = *((int *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f32Compare( const void* pA, const void* pB, int )
    {
        float a = *((float *) pA);
        float b = *((float *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int f64Compare( const void* pA, const void* pB, int )
    {
        double a = *((double *) pA);
        double b = *((double *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int u64Compare( const void* pA, const void* pB, int )
    {
        uint64_t a = *((uint64_t *) pA);
        uint64_t b = *((uint64_t *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int s64Compare( const void* pA, const void* pB, int )
    {
        int64_t a = *((int64_t *) pA);
        int64_t b = *((int64_t *) pB);
        if ( a > b ) 
        {
            return 1;
        }
        return (a == b) ? 0 : -1;
    }

    int strCompare( const void* pA, const void* pB, int numChars )
    {
        char* a = (char *) pA;
        char* b = (char *) pB;
        return strncmp( a, b, numChars );
    }

    int qsPartition( char* pData, qsInfo* pQS, int left, int right )
    {
        int elementSize = pQS->elementSize;
		int elementOffset = pQS->offset;
        char* pLeft = pData + (pQS->elementSize * left) + elementOffset;
		char* pRight = pData + (pQS->elementSize * right) + elementOffset;

        // pivot = pData[left]
        char* pivot = (char *)(pQS->temp);
        int compareSize = pQS->compareSize;
        memcpy( pivot, pLeft, compareSize );

#if 1
		int64_t tmp;
		left--;
		pLeft -= elementSize;
		right++;
		pRight += elementSize;
		while (true)
		{
			do
			{
				left++;
				pLeft += elementSize;
			} while (pQS->compare(pLeft, pivot, compareSize) < 0);

			do
			{
				right--;
				pRight -= elementSize;
			} while (pQS->compare(pRight, pivot, compareSize) > 0);

			if (left >= right)
			{
				return right;
			}

			// swap(a[left], a[right]);
			char* pLeftBase = pLeft - elementOffset;
			char* pRightBase = pRight - elementOffset;
			memcpy(&tmp, pLeftBase, pQS->elementSize);
			memcpy(pLeftBase, pRightBase, pQS->elementSize);
			memcpy(pRightBase, &tmp, pQS->elementSize);
		}
#else
		// this should be more efficient, but it doesn't actually work
		while (true)
		{
			// while (a[left] < pivot) left++;
            while ( pQS->compare( pLeft, pivot, compareSize ) < 0 )
            {
                left++;
                pLeft += elementSize;
            }

		    // while (a[right] > pivot) right--;
            while ( pQS->compare( pRight, pivot, compareSize ) > 0 )
            {
                right--;
                pRight -= elementSize;
            }
            if ( pQS->compare( pLeft, pRight, compareSize ) == 0 )
            {
                left++;
                pLeft += elementSize;
            }
		    else if ( left < right )
		    {
			    // swap(a[left], a[right]);
                memcpy( pivot, pLeft, pQS->elementSize );
                memcpy( pLeft, pRight, pQS->elementSize );
                memcpy( pRight, pivot, pQS->elementSize );
		    }
		    else
		    {
			    return right;
		    }
	    }
#endif
	}

    void qsStep( void* pData, qsInfo* pQS, int left, int right )
    {
	    if ( left < right )
	    {
		    int pivot = qsPartition( (char *) pData, pQS, left, right );
			//qsStep(pData, pQS, left, (pivot - 1));
			qsStep(pData, pQS, left, (pivot));
			qsStep(pData, pQS, (pivot + 1), right);
	    }
    }

}

FORTHOP( qsortOp )
{
    // qsort( ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(6);
    qsInfo qs;

    qs.offset = SPOP;
    int32_t compareType = SPOP;
    qs.elementSize = SPOP;
    int32_t numElements = SPOP;
    void* pArrayBase = (void *)(SPOP);
	qs.temp = __MALLOC(qs.elementSize);

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case BaseType::kByte:
        qs.compareSize = 1;
        qs.compare = s8Compare;
        break;

    case BaseType::kUByte:
        qs.compareSize = 1;
        qs.compare = u8Compare;
        break;

    case BaseType::kShort:
        qs.compareSize = 2;
        qs.compare = s16Compare;
        break;

    case BaseType::kUShort:
        qs.compareSize = 2;
        qs.compare = u16Compare;
        break;

    case BaseType::kInt:
        qs.compareSize = 4;
        qs.compare = s32Compare;
        break;

    case BaseType::kUInt:
        qs.compareSize = 4;
        qs.compare = u32Compare;
        break;

    case BaseType::kLong:
        qs.compareSize = 8;
        qs.compare = s64Compare;
        break;

    case BaseType::kULong:
        qs.compareSize = 8;
        qs.compare = u64Compare;
        break;

    case BaseType::kFloat:
        qs.compareSize = 4;
        qs.compare = f32Compare;
        break;

    case BaseType::kDouble:
        qs.compareSize = 8;
        qs.compare = f64Compare;
        break;

    case BaseType::kString:
        qs.compareSize = CODE_TO_STRING_BYTES(compareType);
        qs.compare = strCompare;
        break;

    default:
		GET_ENGINE->SetError(ForthError::kBadParameter, " failure in qsort - unknown sort element type");
		__FREE(qs.temp);
		return;
		break;
    }

    qsStep( pArrayBase, &qs, 0, (numElements - 1) );

    __FREE( qs.temp );
}

FORTHOP(bsearchOp)
{
    // bsearch( KEY_ADDR ARRAY_ADDR NUM_ELEMENTS ELEMENT_SIZE COMPARE_TYPE COMPARE_OFFSET )
    NEEDS(7);

    int32_t offset = SPOP;
    int32_t compareType = SPOP;
    int32_t elementSize = SPOP;
    int32_t numElements = SPOP;
    char* pData = (char *)(SPOP);
    char* pKey = (char *)(SPOP);
    int32_t compareSize;
    int (*compare)( const void* a, const void* b, int ) = s8Compare;

    switch ( CODE_TO_BASE_TYPE(compareType) )
    {
    case BaseType::kByte:
        compareSize = 1;
        compare = s8Compare;
        break;

    case BaseType::kUByte:
        compareSize = 1;
        compare = u8Compare;
        break;

    case BaseType::kShort:
        compareSize = 2;
        compare = s16Compare;
        break;

    case BaseType::kUShort:
        compareSize = 2;
        compare = u16Compare;
        break;

    case BaseType::kInt:
        compareSize = 4;
        compare = s32Compare;
        break;

    case BaseType::kUInt:
        compareSize = 4;
        compare = u32Compare;
        break;

    case BaseType::kLong:
        compareSize = 8;
        compare = s64Compare;
        break;

    case BaseType::kULong:
        compareSize = 8;
        compare = u64Compare;
        break;

    case BaseType::kFloat:
        compareSize = 4;
        compare = f32Compare;
        break;

    case BaseType::kDouble:
        compareSize = 8;
        compare = f64Compare;
        break;

    case BaseType::kString:
        compareSize = CODE_TO_STRING_BYTES(compareType);
        compare = strCompare;
        break;

    default:
        break;
    }

    int first = 0;
    int last = numElements - 1;
    while ( first <= last )
    {
        int mid = (first + last) / 2;  // compute mid point.
        char* pMid = pData + (elementSize * mid) + offset;
        int result = compare( pKey, pMid, compareSize );
        if ( result > 0 )           //if (key > sortedArray[mid]) 
        {
            first = mid + 1;  // repeat search in top half.
        }
        else if ( result < 0 )                 // if (key < sortedArray[mid]) 
        {
            last = mid - 1; // repeat search in bottom half.
        }
        else
        {
            SPUSH( mid );     // found it. return position
            return;
        }
    }
    // key is not in array, return negative of insertion point+1
    //  the one is added so we can distinguish between an exact match with element 0 (return 0)
    //  and the key value being less than element 0 (return -1)
    SPUSH( -(first + 1) );    // failed to find key
}

///////////////////////////////////////////
//  conditional compilation
///////////////////////////////////////////
FORTHOP( poundIfOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIf();
}

FORTHOP( poundIfdefOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( true );
}

FORTHOP( poundIfndefOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->PoundIfdef( false );
}

FORTHOP( poundElseOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->PoundElse();
}

FORTHOP( poundEndifOp )
{
    Shell *pShell = GET_ENGINE->GetShell();
    pShell->PoundEndif();
}

bool checkBracketToken( const char*& pStart, const char* pToken )
{
    int len = strlen( pToken );
#if defined(WIN32)
    if ( !_strnicmp( pStart, pToken, len ) )
#elif defined(LINUX) || defined(MACOSX)
    if ( !strncasecmp( pStart, pToken, len ) )
#endif
    {
        char delim = pStart[len];
        if ((delim == ' ') || (delim == '\t') || (delim == '\0'))
        {
            pStart += len;
            return true;
        }
    }
    return false;
}

FORTHOP( bracketIfOp )
{
    NEEDS( 1 );
    Shell *pShell = GET_ENGINE->GetShell();
    InputStack* pInput = pShell->GetInput();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
    int depth = 0;
    cell takeIfBranch = SPOP;
    //SPEW_ENGINE("bracketIf");

    if ( !takeIfBranch )
    {
        // skip until you hit [else] or [endif], ignore nested [if][endif] pairs
	    while ( true )
	    {
            const char* pBracket = pBuffer;
            while ( (pBracket = strchr( pBuffer, '[' )) != NULL )
            {
                //SPEW_ENGINE("bracketIf {%s}", pBracket);
                if ( checkBracketToken( pBracket, "[else]" ) )
                {
                    if ( depth == 0 )
                    {
                        pInput->SetBufferPointer( pBracket );
                        return;
                    }
                }
                else if ( checkBracketToken( pBracket, "[endif]" )  || checkBracketToken( pBracket, "[then]" ))
                {
                    if ( depth == 0 )
                    {
                        pInput->SetBufferPointer( pBracket );
                        return;
                    }
                    else
                    {
                        --depth;
                        //SPEW_ENGINE("bracketIf depth %d", depth);
                    }
                }
                else if ( checkBracketToken( pBracket, "[if]" ) || checkBracketToken( pBracket, "[ifdef]" ) || checkBracketToken( pBracket, "[ifundef]" ) )
                {
                    ++depth;
                    //SPEW_ENGINE("bracketIf depth %d", depth);
                }
                else
                {
                    pBracket++;
                }
                pBuffer = pBracket;
            }
		    const char *pLine = pShell->GetInput()->GetLine("<in [if]>");
		    if ( pLine == NULL )
		    {
			    break;
		    }
		    pBuffer = pShell->GetInput()->GetBufferPointer();
	    }
    }
}

FORTHOP( bracketIfDefOp )
{
    NEEDS( 0 );
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pToken = pOuter->GetNextSimpleToken();
    Vocabulary* pFoundVocab;
    forthop* pSymbol = pOuter->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    SPUSH ( (pSymbol != NULL) ? ~0 : 0 );
    bracketIfOp( pCore );
}

FORTHOP( bracketIfUndefOp )
{
    NEEDS( 0 );
    Engine *pEngine = GET_ENGINE;
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char* pToken = pOuter->GetNextSimpleToken();
    Vocabulary* pFoundVocab;
    forthop* pSymbol = pOuter->GetVocabularyStack()->FindSymbol( pToken, &pFoundVocab );
    SPUSH ( (pSymbol != NULL) ? 0 : ~0 );
    bracketIfOp( pCore );
}

FORTHOP( bracketElseOp )
{
    NEEDS( 0 );
    Shell *pShell = GET_ENGINE->GetShell();
    InputStack* pInput = pShell->GetInput();
	const char* pBuffer = pShell->GetInput()->GetBufferPointer();
    int depth = 0;

    // skip until you hit [endif], ignore nested [if][endif] pairs
    while ( true )
    {
        const char* pBracket = pBuffer;
        while ( (pBracket = strchr( pBuffer, '[' )) != NULL )
        {
            if ( checkBracketToken( pBracket, "[endif]" )  || checkBracketToken( pBracket, "[then]" ))
            {
                if ( depth == 0 )
                {
                    pInput->SetBufferPointer( pBracket );
                    return;
                }
                else
                {
                    --depth;
                }
            }
            else if ( checkBracketToken( pBracket, "[if]" ) || checkBracketToken( pBracket, "[ifdef]" ) || checkBracketToken( pBracket, "[ifundef]" ) )
            {
                ++depth;
            }
            else
            {
                ++pBracket;
            }
            pBuffer = pBracket;
        }
	    const char *pLine = pShell->GetInput()->GetLine("<in [else]>");
	    if ( pLine == NULL )
	    {
		    break;
	    }
	    pBuffer = pShell->GetInput()->GetBufferPointer();
    }
}

FORTHOP( bracketEndifOp )
{
}

FORTHOP(getTraceOp)
{
	int traceFlags = GET_ENGINE->GetTraceFlags();
	SPUSH(traceFlags);
}

#ifndef ASM_INNER_INTERPRETER

FORTHOP(setTraceBop)
{
    int traceFlags = SPOP;
    GET_ENGINE->SetTraceFlags(traceFlags);
}

#endif


FORTHOP(ssPushBop)
{
	NEEDS(1);
	int32_t v = SPOP;
	GET_ENGINE->GetShell()->GetShellStack()->Push(v);
}

FORTHOP(ssPopBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->Pop());
}

FORTHOP(ssPeekBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->Peek());
}

FORTHOP(ssDepthBop)
{
	SPUSH(GET_ENGINE->GetShell()->GetShellStack()->GetDepth());
}

///////////////////////////////////////////
//  block i/o
///////////////////////////////////////////

FORTHOP( blkOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    SPUSH((cell)(pBlockManager->GetBlockPtr()));
}

FORTHOP( blockOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    char* pBlock = pBlockManager->GetBlock(SPOP, true);
    SPUSH( (cell)(pBlock) );
}

FORTHOP( bufferOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    char* pBlock = pBlockManager->GetBlock(SPOP, false);
    SPUSH( (cell)(pBlock) );
}

FORTHOP( emptyBuffersOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->EmptyBuffers();
}

FORTHOP( flushOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->SaveBuffers( true );
}

FORTHOP( saveBuffersOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->SaveBuffers( false );
}

FORTHOP( updateOp )
{
    BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
    pBlockManager->UpdateCurrentBuffer();
}

FORTHOP( thruOp )
{
    uint32_t lastBlock = (uint32_t) SPOP;
    uint32_t firstBlock = (uint32_t) SPOP;
    Engine* pEngine = GET_ENGINE;
    if ( lastBlock < firstBlock )
    {
        pEngine->SetError( ForthError::kIO, "thru - last block less than first block" );
    }
    else
    {
        BlockFileManager*  pBlockManager = GET_ENGINE->GetBlockFileManager();
        if ( lastBlock < pBlockManager->GetNumBlocksInFile())
        {
            GET_ENGINE->PushInputBlocks(pBlockManager, firstBlock, lastBlock );
        }
        else
        {
            pEngine->SetError( ForthError::kIO, "thru - last block beyond end of block file" );
        }
    }
}


///////////////////////////////////////////
//  threads
///////////////////////////////////////////

FORTHOP(thisFiberOp)
{
	PUSH_OBJECT(((Fiber *)(pCore->pFiber))->GetFiberObject());
}

FORTHOP(thisThreadOp)
{
	Fiber* pFiber = (Fiber*)(pCore->pFiber);
	Thread* pThread = pFiber->GetParent();
	PUSH_OBJECT(pThread->GetThreadObject());
}

#ifdef WIN32
///////////////////////////////////////////
//  Windows support
///////////////////////////////////////////

FORTHOP( createEventOp )
{
    LPCTSTR lpName = (LPCTSTR)(SPOP);
    BOOL bInitialState = (BOOL)(SPOP);
    BOOL bManualReset = (BOOL)(SPOP);
    LPSECURITY_ATTRIBUTES lpEventAttributes = (LPSECURITY_ATTRIBUTES) (SPOP);
    HANDLE result = ::CreateEvent( lpEventAttributes, bManualReset, bInitialState, lpName );
    SPUSH( (cell) result );
}

FORTHOP( closeHandleOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::CloseHandle( hObject );
    SPUSH( (cell) result );
}

FORTHOP( setEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::SetEvent( hObject );
    SPUSH( (cell) result );
}

FORTHOP( resetEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::ResetEvent( hObject );
    SPUSH( (cell) result );
}

FORTHOP( pulseEventOp )
{
    HANDLE hObject = (HANDLE)(SPOP);
    BOOL result = ::PulseEvent( hObject );
    SPUSH( (cell) result );
}

FORTHOP( getLastErrorOp )
{
    DWORD result = ::GetLastError();
    SPUSH( (cell) result );
}

FORTHOP( waitForSingleObjectOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    HANDLE hHandle = (HANDLE)(SPOP);

    DWORD result = ::WaitForSingleObject( hHandle, dwMilliseconds );
    SPUSH( (cell) result );
}

FORTHOP( waitForMultipleObjectsOp )
{
    DWORD dwMilliseconds = (DWORD)(SPOP);
    BOOL bWaitAll = (BOOL)(SPOP);
    const HANDLE* lpHandles = (const HANDLE*)(SPOP);
    DWORD nCount = (DWORD)(SPOP);

    DWORD result = ::WaitForMultipleObjects( nCount, lpHandles, bWaitAll, dwMilliseconds );
    SPUSH( (cell) result );
}

FORTHOP( initializeCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::InitializeCriticalSection( pCriticalSection );
}

FORTHOP( deleteCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::DeleteCriticalSection( pCriticalSection );
}

FORTHOP( enterCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::EnterCriticalSection( pCriticalSection );
}

FORTHOP( leaveCriticalSectionOp )
{
    LPCRITICAL_SECTION pCriticalSection = (LPCRITICAL_SECTION)(SPOP);
    ::LeaveCriticalSection( pCriticalSection );
}

FORTHOP( findFirstFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    LPCTSTR pPath = (LPCTSTR)(SPOP);
    HANDLE result = ::FindFirstFile( pPath, pOutData );
    SPUSH( (cell) result );
}

FORTHOP( findNextFileOp )
{
    LPWIN32_FIND_DATA pOutData = (LPWIN32_FIND_DATA)(SPOP);
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindNextFile( searchHandle, pOutData );
    SPUSH( (cell) result );
}

FORTHOP( findCloseOp )
{
    HANDLE searchHandle = (HANDLE)(SPOP);
    BOOL result = ::FindClose( searchHandle );
    SPUSH( (cell) result );
}

FORTHOP( showConsoleOp )
{
	NEEDS(1);
	int32_t showIt = SPOP;
	ShowWindow( GetConsoleWindow(), (showIt != 0) ? SW_RESTORE : SW_HIDE );
}

FORTHOP( windowsConstantsOp )
{
    static int winConstants[5] =
    {
        (sizeof(winConstants)/sizeof(int)) - 1,
        sizeof(TCHAR),
        MAX_PATH,
        sizeof(WIN32_FIND_DATA),
        sizeof(CRITICAL_SECTION)
    };
    SPUSH( (cell) (&(winConstants[0])) );
}

#endif

FORTHOP(windowsOp)
{
#ifdef WIN32
	SPUSH(~0);
#else
	SPUSH(0);
#endif
}

FORTHOP(linuxOp)
{
#ifdef LINUX
	SPUSH(~0);
#else
	SPUSH(0);
#endif
}

FORTHOP(macosxOp)
{
#ifdef MACOSX
    SPUSH(~0);
#else
    SPUSH(0);
#endif
}

FORTHOP(forth64Op)
{
#ifdef FORTH64
    SPUSH(~0);
#else
    SPUSH(0);
#endif
}

FORTHOP(pathSeparatorOp)
{
    char sep = PATH_SEPARATOR[0];

    SPUSH(((cell)sep));
}

FORTHOP( setConsoleCursorOp )
{
	NEEDS( 2 );
#ifdef WIN32
	COORD screenPos;
	screenPos.Y = (SHORT) SPOP;
	screenPos.X = (SHORT) SPOP;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleCursorPosition( hConsole, screenPos );
#else
	int line = SPOP;
	int column = SPOP;
	printf("%c[%d;%dH", 0x1b, line, column);
#endif
}

FORTHOP( getConsoleCursorOp )
{
	NEEDS( 0 );
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hConsole, &consoleInfo );
	SPUSH( consoleInfo.dwCursorPosition.X );
	SPUSH( consoleInfo.dwCursorPosition.Y );
#else
	SPUSH(0);
	SPUSH(0);
#endif
}

FORTHOP( setConsoleColorOp )
{
	NEEDS( 1 );
#ifdef WIN32
	WORD attributes = (WORD) SPOP;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleTextAttribute( hConsole, attributes );
#else
	int attributes = SPOP;
	printf("%c[%dm", 0x1b, attributes);
#endif
}

FORTHOP( getConsoleColorOp )
{
#ifdef WIN32
	NEEDS( 0 );
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	GetConsoleScreenBufferInfo( hConsole, &consoleInfo );
	SPUSH( consoleInfo.wAttributes );
#else
	SPUSH(0xF0);
#endif
}

FORTHOP(clearConsoleOp)
{
	NEEDS(0);
#ifdef WIN32
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; 
    DWORD dwConSize;

	// Get the number of character cells in the current buffer. 
    if ( !GetConsoleScreenBufferInfo( hConsole, &csbi ) )
    {
		return;
    }

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.
	if( !FillConsoleOutputCharacter( hConsole,        // Handle to console screen buffer 
                                    (TCHAR) ' ',     // Character to write to the buffer
                                    dwConSize,       // Number of cells to write 
                                    coordScreen,     // Coordinates of first cell 
                                    &cCharsWritten ))// Receive number of characters written
	{
		return;
	}

	// Get the current text attribute.
	if ( !GetConsoleScreenBufferInfo( hConsole, &csbi ) )
	{
		return;
	}

	// Set the buffer's attributes accordingly.

	if( !FillConsoleOutputAttribute( hConsole,         // Handle to console screen buffer 
                                    csbi.wAttributes, // Character attributes to use
                                    dwConSize,        // Number of cells to set attribute 
                                    coordScreen,      // Coordinates of first cell 
                                    &cCharsWritten )) // Receive number of characters written
	{
		return;
	}
	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition( hConsole, coordScreen );
#else
	printf("%c[2J%c[0;0H", 0x1b, 0x1b);
#endif
}


FORTHOP(getConsoleSizesOp)
{
    NEEDS(0);
#ifdef WIN32
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    SPUSH((consoleInfo.srWindow.Right - consoleInfo.srWindow.Left) + 1);
    SPUSH((consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top) + 1);
#else
    struct winsize ws;
    ioctl(0, TIOCGWINSZ, &ws);
    SPUSH(ws.ws_col);
    SPUSH(ws.ws_row);
#endif
}

FORTHOP(findResourceOp)
{
    Engine* pEngine = GET_ENGINE;
    const char* pResourceName = (const char*)(SPOP);
    Shell* pShell = pEngine->GetShell();
    std::string containingDir;
    const char* pResult = nullptr;

    if (pShell->FindFileInPaths(pResourceName, pShell->GetResourcePaths(), containingDir))
    {
        pResult = pEngine->GetOuterInterpreter()->AddTempString(containingDir.c_str());
    }

    SPUSH((cell)pResult);
}

///////////////////////////////////////////
//  Network support
///////////////////////////////////////////

FORTHOP( clientOp )
{
	const char* pServerStr = (const char*)(SPOP);
	Engine *pEngine = GET_ENGINE;
	int result = ClientMainLoop( pEngine, pServerStr, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( serverOp )
{
	Engine *pEngine = GET_ENGINE;
	int result = ServerMainLoop( pEngine, false, FORTH_SERVER_PORT );
	SPUSH( result );
}

FORTHOP( shutdownOp )
{
    SET_STATE( OpResult::kShutdown );
}


FORTHOP( abortOp )
{
    SET_FATAL_ERROR( ForthError::kAbort );
}

FORTHOP(bkptOp)
{
    CONSOLE_STRING_OUT("<<<BREAKPOINT HIT>>>\n");
}

FORTHOP(dumpProfileOp)
{
    Engine *pEngine = GET_ENGINE;
    pEngine->DumpExecutionProfile();
}

FORTHOP(resetProfileOp)
{
    Engine *pEngine = GET_ENGINE;
    pEngine->ResetExecutionProfile();
}

FORTHOP(showMemoryOp)
{
    Engine* pEngine = GET_ENGINE;
    pEngine->ShowMemoryInfo();
}

FORTHOP(readOp)
{
    NEEDS(3);

    int numBytes = SPOP;
    char* buffer = (char *)(SPOP);
    int fd = SPOP;

    int result = read(fd, buffer, numBytes);
    SPUSH(result);
}

FORTHOP(writeOp)
{
    NEEDS(3);

    int numBytes = SPOP;
    char* buffer = (char *)(SPOP);
    int fd = SPOP;

    int result = write(fd, buffer, numBytes);
    SPUSH(result);
}

FORTHOP(openOp)
{
    NEEDS(2);
    int flags = SPOP;
    char* filename = (char *)(SPOP);

    int result = open(filename, flags);
    SPUSH(result);
}

FORTHOP(closeOp)
{
    NEEDS(1);
    int fd = SPOP;

    int result = close(fd);
    SPUSH(result);
}


#ifndef ASM_INNER_INTERPRETER


// done is used by the outer interpreter to make the inner interpreter
//  exit after interpreting a single opcode
FORTHOP( doneBop )
{
    SET_STATE( OpResult::kDone );
}

// abort is a user command which causes the entire forth engine to exit,
//   and indicates that a fatal error has occured
FORTHOP( abortBop )
{
    SET_FATAL_ERROR( ForthError::kAbort );
}

FORTHOP( doVariableBop )
{
    // IP points to data field
    SPUSH( (cell) GET_IP );
    SET_IP( (forthop* ) (RPOP) );
}

//##############################
//
// math ops
//

FORTHOP( plusBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a + b );
}

FORTHOP( minusBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a - b );
}

FORTHOP( timesBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a * b );
}

FORTHOP( times2Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a << 1 );
}

FORTHOP( times4Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a << 2 );
}

FORTHOP( times8Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a << 3 );
}

FORTHOP( divideBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a / b );
}

FORTHOP( divide2Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a >> 1 );
}

FORTHOP( divide4Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a >> 2 );
}

FORTHOP( divide8Bop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( a >> 3 );
}

FORTHOP( divmodBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
#if defined(FORTH64)
    lldiv_t v;
    v = lldiv(a, b);
#else

#if defined(WIN32)

    div_t v;
#if defined(MSDEV)
    v = div(a, b);
#else
    v = ldiv(a, b);
#endif

#elif defined(RASPI)

    div_t v;
    v = div(a, b);

#elif defined(LINUX) || defined(MACOSX)

    div_t v;
    v = ldiv(a, b);

#endif
#endif
    SPUSH( v.rem );
    SPUSH( v.quot );
}

FORTHOP( mtimesBop )
{
    NEEDS(2);
    doubleCell result;
    cell b = SPOP;
    cell a = SPOP;
#if defined(FORTH64)
#if defined(WINDOWS_BUILD)
    result.cells[0] = Multiply128(a, b, &result.cells[1]);
#else
    result.sdcell = ((__int128)a) * b;
#endif
#else
    result.sdcell = ((int64_t)a) * b;
#endif
    DCPUSH(result);
}

FORTHOP( umtimesBop )
{
    NEEDS(2);
    doubleCell result;
    ucell b = SPOP;
    ucell a = SPOP;
#if defined(FORTH64)
#if defined(WINDOWS_BUILD)
    result.cells[0] = UnsignedMultiply128(a, b, &result.ucells[1]);
#else
    result.sdcell = ((__uint128)a) * b;
#endif
#else
    result.sdcell = ((uint64_t)a) * b;
#endif
    DCPUSH(result);
}

#if 0
FORTHOP( ummodBop )
{
    NEEDS(2);
    ldiv_t v;
    uint32_t b = SPOP;
    stackInt64 a;
    LPOP( a );
    uint32_t quotient = a.64 / b;
    uint32_t remainder = a.64 % b;
    SPUSH( remainder );
    SPUSH( quotient );
}

FORTHOP( smdivremBop )
{
    NEEDS(2);
    lldiv_t v;
    int64_t b = (int64_t) SPOP;
    stackInt64 a;
    LPOP( a );

    v = lldiv(a.s64, b);

    SPUSH( (cell) v.rem );
    SPUSH( (cell) v.quot );
}
#endif

FORTHOP( modBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a % b );
}

FORTHOP( negateBop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( -a );
}

FORTHOP(fplusBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH(a + b);
}

FORTHOP(fminusBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH(a - b);
}

FORTHOP(ftimesBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH(a * b);
}

FORTHOP(fdivideBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH(a / b);
}


FORTHOP( dplusBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a + b );
}

FORTHOP( dminusBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a - b );
}

FORTHOP( dtimesBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a * b );
}

FORTHOP( ddivideBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( a / b );
}


FORTHOP( dsinBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sin(a) );
}

FORTHOP( dasinBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( asin(a) );
}

FORTHOP( dcosBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( cos(a) );
}

FORTHOP( dacosBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( acos(a) );
}

FORTHOP( dtanBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( tan(a) );
}

FORTHOP( datanBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( atan(a) );
}

FORTHOP( datan2Bop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( atan2(a, b) );
}

FORTHOP( dexpBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( exp(a) );
}

FORTHOP( dlnBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log(a) );
}

FORTHOP( dlog10Bop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( log10(a) );
}

FORTHOP( dpowBop )
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH( pow(a, b) );
}

FORTHOP( dsqrtBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( sqrt(a) );
}

FORTHOP( dceilBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( ceil(a) );
}

FORTHOP( dfloorBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( floor(a) );
}

FORTHOP( dabsBop )
{
    NEEDS(2);
    double a = DPOP;
    DPUSH( fabs(a) );
}

FORTHOP(dldexpBop)
{
    NEEDS(3);
    int n = SPOP;
    double a = DPOP;
    DPUSH(ldexp(a, n));
}

FORTHOP(dfrexpBop)
{
    NEEDS(2);
    int n;
    double a = DPOP;
    DPUSH(frexp(a, &n));
    SPUSH(n);
}

FORTHOP(dmodfBop)
{
    NEEDS(2);
    double b;
    double a = DPOP;
    a = modf(a, &b);
    DPUSH(b);
    DPUSH(a);
}

FORTHOP(dfmodBop)
{
    NEEDS(4);
    double b = DPOP;
    double a = DPOP;
    DPUSH(fmod(a, b));
}

FORTHOP( fsinBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( sinf(a) );
}

FORTHOP( fasinBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( asinf(a) );
}

FORTHOP( fcosBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( cosf(a) );
}

FORTHOP( facosBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( acosf(a) );
}

FORTHOP( ftanBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( tanf(a) );
}

FORTHOP( fatanBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( atanf(a) );
}

FORTHOP( fatan2Bop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( atan2f(a, b) );
}

FORTHOP( fexpBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( expf(a) );
}

FORTHOP( flnBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( logf(a) );
}

FORTHOP( flog10Bop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( log10f(a) );
}

FORTHOP( fpowBop )
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( powf(a, b) );
}

FORTHOP( fsqrtBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( sqrtf(a) );
}

FORTHOP( fceilBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( ceilf(a) );
}

FORTHOP( ffloorBop )
{
    NEEDS(1);
    float a = FPOP;
    FPUSH( floorf(a) );
}

FORTHOP(fabsBop)
{
    NEEDS(1);
    float a = FPOP;
    FPUSH(fabs(a));
}

FORTHOP(fldexpBop)
{
    NEEDS(3);
    int n = SPOP;
    float a = FPOP;
    FPUSH(ldexpf(a, n));
}

FORTHOP(ffrexpBop)
{
    NEEDS(1);
    int n;
    float a = FPOP;
    FPUSH(frexpf(a, &n));
    SPUSH(n);
}

FORTHOP(fmodfBop)
{
    NEEDS(1);
    float b;
    float a = FPOP;
    a = modff(a, &b);
    FPUSH(b);
    FPUSH(a);
}

FORTHOP(ffmodBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH(fmodf(a, b));
}

FORTHOP( lplusBop )
{
#ifdef FORTH64
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH(a + b);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 result;
    LPOP(b);
    LPOP(a);
    result.s64 = a.s64 + b.s64;
    LPUSH(result);
#endif
}

FORTHOP( lminusBop )
{
#ifdef FORTH64
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH(a - b);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 result;
    LPOP(b);
    LPOP(a);
    result.s64 = a.s64 - b.s64;
    LPUSH(result);
#endif
}

FORTHOP( ltimesBop )
{
#ifdef FORTH64
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH(a * b);
#else
    NEEDS(4);
    stackInt64 a;
    stackInt64 b;
    stackInt64 result;
    LPOP(b);
    LPOP(a);
    result.s64 = a.s64 * b.s64;
    LPUSH(result);
#endif
}

FORTHOP( i2fBop )
{
    NEEDS(1);
    int ii = (int)SPOP;
    float a = (float) ii;
    FPUSH( a );
}

FORTHOP( i2dBop )
{
    NEEDS(1);
    int ii = (int)SPOP;
    double a = (double) ii;
    DPUSH( a );
}

FORTHOP( f2iBop )
{
    NEEDS(1);
    int a = (int) FPOP;
    SPUSH( a );
}

FORTHOP( f2dBop )
{
    NEEDS(1);
    double a = FPOP;
    DPUSH( a );
}

FORTHOP( d2iBop )
{
    NEEDS(2);
    int a = (int) DPOP;
    SPUSH( a );
}

FORTHOP( d2fBop )
{
    NEEDS(2);
    float a = (float) DPOP;
    FPUSH( a );
}

//##############################
//
// control ops
//
// TODO: replace branch, tbranch, fbranch with immediate ops
// exit normal op with no local vars
FORTHOP(doExitBop)
{
    // rstack: oldIP
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( ForthError::kReturnStackUnderflow );
    }
    else if (GET_SDEPTH < 0)
    {
        SET_ERROR(ForthError::kParamStackUnderflow);
    }
    else
    {
        forthop* newIP = (forthop* )RPOP;
        SET_IP( newIP );
		if (newIP == nullptr)
		{
			SET_STATE(OpResult::kDone);
		}
    }
}

// exit normal op with local vars
FORTHOP(doExitLBop)
{
    // rstack: local_var_storage oldFP oldIP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (cell *) (RPOP) );
    if ( GET_RDEPTH < 1 )
    {
        SET_ERROR( ForthError::kReturnStackUnderflow );
    }
    else if (GET_SDEPTH < 0)
    {
        SET_ERROR(ForthError::kParamStackUnderflow);
    }
    else
    {
        forthop* newIP = (forthop* )RPOP;
		SET_IP(newIP);
		if (newIP == nullptr)
		{
			SET_STATE(OpResult::kDone);
		}
	}
}

// exit method op with no local vars
FORTHOP(doExitMBop)
{
    // rstack: oldIP oldTP
    if ( GET_RDEPTH < 2 )
    {
        SET_ERROR( ForthError::kReturnStackUnderflow );
    }
    else if (GET_SDEPTH < 0)
    {
        SET_ERROR(ForthError::kParamStackUnderflow);
    }
    else
    {
        forthop* newIP = (forthop* )RPOP;
		SET_IP(newIP);
        SET_TP((ForthObject)(RPOP));

		if (newIP == nullptr)
		{
			SET_STATE(OpResult::kDone);
		}
	}
}

// exit method op with local vars
FORTHOP(doExitMLBop)
{
    // rstack: local_var_storage oldFP oldIP oldTP
    // FP points to oldFP
    SET_RP( GET_FP );
    SET_FP( (cell *) (RPOP) );
    if ( GET_RDEPTH < 2 )
    {
        SET_ERROR( ForthError::kReturnStackUnderflow );
    }
    else if (GET_SDEPTH < 0)
    {
        SET_ERROR(ForthError::kParamStackUnderflow);
    }
    else
    {
        forthop* newIP = (forthop* )RPOP;
		SET_IP(newIP);
        SET_TP((ForthObject)(RPOP));

		if (newIP == nullptr)
		{
			SET_STATE(OpResult::kDone);
		}
	}
}

FORTHOP(callBop)
{
    RPUSH( (cell) GET_IP );
    SET_IP( (forthop* ) SPOP );
}

FORTHOP(gotoBop)
{
    SET_IP( (forthop* ) SPOP );
}

FORTHOP(doDoBop)
{
    NEEDS(2);
    cell startIndex = SPOP;
    // skip over loop exit IP right after this op
    forthop* newIP = (GET_IP) + 1;
    SET_IP( newIP );

    RPUSH( (cell) newIP );
    RPUSH( SPOP );
    RPUSH( startIndex );
    // top of rstack is current index, next is end index,
    //  next is looptop IP
}

FORTHOP(doCheckDoBop)
{
    NEEDS(2);
    cell startIndex = SPOP;
    cell endIndex = SPOP;
    // skip over loop exit IP right after this op
	if ( startIndex < endIndex)
	{
	    forthop* newIP = (GET_IP + 1);
		RPUSH( (cell) newIP );
		RPUSH( endIndex );
		RPUSH( startIndex );
		// top of rstack is current index, next is end index,
		//  next is looptop IP
	    SET_IP( newIP );
	}
}

FORTHOP(doLoopBop)
{
    RNEEDS(3);

    cell *pRP = GET_RP;
    cell newIndex = (*pRP) + 1;

    if ( newIndex >= pRP[1] )
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP( pRP + 3 );
    }
    else
    {
        // loop again
        *pRP = newIndex;
        SET_IP( (forthop* ) (pRP[2]) );
    }
}

FORTHOP(doLoopNBop)
{
    RNEEDS(3);
    NEEDS(1);

    cell increment = SPOP;
    cell* pRP = GET_RP;
    cell index = *pRP;
    cell limit = pRP[1];
    cell newIndex = index + increment;
    bool done = false;

    cell oldDiff = index - limit;
    done = !(((oldDiff ^ (oldDiff + increment)) & (oldDiff ^ increment)) >= 0);

    if (done)
    {
        // loop has ended, drop end, current indices, loopIP
        SET_RP(pRP + 3);
    }
    else
    {
        // loop again
        *pRP = newIndex;
        SET_IP((forthop*)(pRP[2]));
    }
}

FORTHOP(iBop)
{
    SPUSH( *(GET_RP) );
}

FORTHOP(jBop)
{
    SPUSH( GET_RP[3] );
}


// discard stuff "do" put on rstack in preparation for an "exit"
//   of the current op from inside a do loop
FORTHOP( unloopBop )
{
    RNEEDS(3);
    // loop has ended, drop end, current indices, loopIP
    SET_RP( GET_RP + 3 );
}


// exit loop immediately
FORTHOP( leaveBop )
{
    RNEEDS(3);
    cell *pRP = GET_RP;
    forthop* newIP = (forthop* ) (pRP[2]) - 1;

    // loop has ended, drop end, current indices, loopIP
    SET_RP( pRP + 3 );
    // point IP back to the branch op just after the _do opcode
    SET_IP( newIP );
}

///////////////////////////////////////////
//  bit-vector logic ops
///////////////////////////////////////////
FORTHOP( orBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a | b );
}

FORTHOP( andBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a & b );
}

FORTHOP( xorBop )
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( a ^ b );
}

FORTHOP( invertBop )
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ~a );
}

FORTHOP(ilshiftBop)
{
    NEEDS(2);
    uint32_t b = (uint32_t)(SPOP);
    uint32_t a = (uint32_t)(SPOP);
    a <<= b;
    SPUSH(a);
}

FORTHOP(llshiftBop)
{
#if defined(FORTH64)
    NEEDS(2);
    uint32_t b = (uint32_t)(SPOP);
    ucell a = SPOP;
    a <<= b;
    SPUSH(a);
#else
    NEEDS(3);
    uint32_t b = (uint32_t)(SPOP);
    stackInt64 a;
    LPOP(a);
    a.u64 <<= b;
    LPUSH(a);
#endif
}

FORTHOP( irshiftBop )
{
    NEEDS(2);
    uint32_t b = (uint32_t)(SPOP);
    uint32_t a = (uint32_t)(SPOP);
    a >>= b;
    SPUSH(a);
}

FORTHOP(lrshiftBop)
{
#if defined(FORTH64)
    NEEDS(2);
    uint32_t b = (uint32_t)(SPOP);
    ucell a = SPOP;
    a >>= b;
    SPUSH(a);
#else
    NEEDS(3);
    uint32_t b = (uint32_t)(SPOP);
    stackInt64 a;
    LPOP(a);
    a.u64 >>= b;
    LPUSH(a);
#endif
}


FORTHOP( arshiftBop )
{
    NEEDS(2);
    ucell b = SPOP;
    cell a = SPOP;
    SPUSH( a >> b );
}


FORTHOP(irotateBop)
{
    NEEDS(2);
    uint32_t b = ((uint32_t)(SPOP) & (32 - 1));
    uint32_t a = (uint32_t)(SPOP);
    uint32_t result = (a << b) | (a >> (32 - b));
    SPUSH(result);
}

FORTHOP(lrotateBop)
{
#if defined(FORTH64)
    NEEDS(2);
    uint32_t b = ((uint32_t)(SPOP) & (64 - 1));
    ucell a = SPOP;
    ucell result = (a << b) | (a >> (64 - b));
    SPUSH(result);
#else
    NEEDS(3);
    uint32_t b = ((uint32_t)(SPOP) & 63);
    stackInt64 a;
    LPOP(a);
    a.u64 = (a.u64 << b) | (a.u64 >> (64 - b));
    LPUSH(a);
#endif
}

FORTHOP(ireverseBop)
{
    NEEDS(1);
    //Knuth's algorithm from http://www.hackersdelight.org/revisions.pdf
    uint32_t a = SPOP;
    uint32_t t;
    a = (a << 15) | (a >> 17);
    t = (a ^ (a >> 10)) & 0x003f801f;
    a = (t + (t << 10)) ^ a;
    t = (a ^ (a >> 4)) & 0x0e038421;
    a = (t + (t << 4)) ^ a;
    t = (a ^ (a >> 2)) & 0x22488842;
    a = (t + (t << 2)) ^ a;
    SPUSH(a);
}

FORTHOP(countLeadingZerosBop)
{
    NEEDS(1);
    ucell a = SPOP;
    cell result = 0;
    // for some reason, the following line doesn't work in 64-bit mode:
    //ucell mask = 1L << (CELL_BITS - 1);
#if defined(FORTH64)
    ucell mask = 0x8000000000000000L;
#else
    ucell mask = 0x80000000;
#endif
    while (mask != 0)
    {
        if ((a & mask) != 0)
        {
            break;
        }
        mask >>= 1;
        result++;
    }
    SPUSH(result);
}

FORTHOP(countTrailingZerosBop)
{
    NEEDS(1);
    ucell a = SPOP;
    cell result = 0;
    ucell mask = 1;
    while (mask != 0)
    {
        if ((a & mask) != 0)
        {
            break;
        }
        mask <<= 1;
        result++;
    }
    SPUSH(result);
}

///////////////////////////////////////////
//  boolean ops
///////////////////////////////////////////
FORTHOP( notBop )
{
    NEEDS(1);
    ucell a = SPOP;
    SPUSH( ~a );
}

FORTHOP( trueBop )
{
    SPUSH( ~0 );
}

FORTHOP( falseBop )
{
    SPUSH( 0 );
}

FORTHOP( nullBop )
{
    SPUSH( 0 );
}

FORTHOP( dnullBop )
{
    SPUSH( 0 );
    SPUSH( 0 );
}

//##############################
//
// integer comparison ops
//

FORTHOP(equalsBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(notEqualsBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(greaterThanBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(greaterEqualsBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(lessThanBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(lessEqualsBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(equals0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a == 0 ) ? -1L : 0 );
}

FORTHOP(notEquals0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a != 0 ) ? -1L : 0 );
}

FORTHOP(greaterThan0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a > 0 ) ? -1L : 0 );
}

FORTHOP(greaterEquals0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a >= 0 ) ? -1L : 0 );
}

FORTHOP(lessThan0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a < 0 ) ? -1L : 0 );
}

FORTHOP(lessEquals0Bop)
{
    NEEDS(1);
    cell a = SPOP;
    SPUSH( ( a <= 0 ) ? -1L : 0 );
}

FORTHOP(unsignedGreaterThanBop)
{
    NEEDS(2);
    ucell b = (ucell) SPOP;
    ucell a = (ucell) SPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(unsignedLessThanBop)
{
    NEEDS(2);
    ucell b = (ucell) SPOP;
    ucell a = (ucell) SPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(withinBop)
{
    NEEDS(3);
    cell hiLimit = SPOP;
    cell loLimit = SPOP;
    cell val = SPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(minBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a < b ) ? a : b );
}

FORTHOP(maxBop)
{
    NEEDS(2);
    cell b = SPOP;
    cell a = SPOP;
    SPUSH( ( a > b ) ? a : b );
}

FORTHOP(icmpBop)
{
    NEEDS(2);
    int32_t b = (int)(SPOP);
    int32_t a = (int)(SPOP);
    SPUSH((a == b) ? 0 : ((a > b) ? 1 : -1));
}

FORTHOP(uicmpBop)
{
    NEEDS(2);
    uint32_t b = (int)(SPOP);
    uint32_t a = (int)(SPOP);
    SPUSH((a == b) ? 0 : ((a > b) ? 1 : -1));
}

FORTHOP(lcmpBop)
{
	NEEDS(2);
#if defined(FORTH64)
    cell b = SPOP;
    cell a = SPOP;
    SPUSH((a == b) ? 0 : ((a > b) ? 1 : -1));
#else
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    SPUSH((a.s64 == b.s64) ? 0 : ((a.s64 > b.s64) ? 1 : -1));
#endif
}

FORTHOP(ulcmpBop)
{
	NEEDS(2);
#if defined(FORTH64)
    ucell b = (ucell)SPOP;
    ucell a = (ucell)SPOP;
    SPUSH((a == b) ? 0 : ((a > b) ? 1 : -1));
#else
    stackInt64 a;
    stackInt64 b;
    LPOP(b);
    LPOP(a);
    SPUSH((a.u64 == b.u64) ? 0 : ((a.u64 > b.u64) ? 1 : -1));
#endif
}


//##############################
//
// single-precision fp comparison ops
//

FORTHOP(fEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(fNotEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(fGreaterThanBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(fGreaterEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(fLessThanBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(fLessEqualsBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(fEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a == 0.0f ) ? -1L : 0 );
}

FORTHOP(fNotEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a != 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterThan0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a > 0.0f ) ? -1L : 0 );
}

FORTHOP(fGreaterEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a >= 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessThan0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a < 0.0f ) ? -1L : 0 );
}

FORTHOP(fLessEquals0Bop)
{
    NEEDS(1);
    float a = FPOP;
    SPUSH( ( a <= 0.0f ) ? -1L : 0 );
}

FORTHOP(fWithinBop)
{
    NEEDS(3);
    float hiLimit = FPOP;
    float loLimit = FPOP;
    float val = FPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(fMinBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a < b ) ? a : b );
}

FORTHOP(fMaxBop)
{
    NEEDS(2);
    float b = FPOP;
    float a = FPOP;
    FPUSH( ( a > b ) ? a : b );
}

FORTHOP(fcmpBop)
{
	NEEDS(2);
	float b = FPOP;
	float a = FPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}

//##############################
//
// double-precision fp comparison ops
//

FORTHOP(dEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a == b ) ? -1L : 0 );
}

FORTHOP(dNotEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a != b ) ? -1L : 0 );
}

FORTHOP(dGreaterThanBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a > b ) ? -1L : 0 );
}

FORTHOP(dGreaterEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a >= b ) ? -1L : 0 );
}

FORTHOP(dLessThanBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a < b ) ? -1L : 0 );
}

FORTHOP(dLessEqualsBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    SPUSH( ( a <= b ) ? -1L : 0 );
}

FORTHOP(dEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a == 0.0 ) ? -1L : 0 );
}

FORTHOP(dNotEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a != 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterThan0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a > 0.0 ) ? -1L : 0 );
}

FORTHOP(dGreaterEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a >= 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessThan0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a < 0.0 ) ? -1L : 0 );
}

FORTHOP(dLessEquals0Bop)
{
    NEEDS(1);
    double a = DPOP;
    SPUSH( ( a <= 0.0 ) ? -1L : 0 );
}

FORTHOP(dWithinBop)
{
    NEEDS(3);
    double hiLimit = DPOP;
    double loLimit = DPOP;
    double val = DPOP;
    SPUSH( ( (loLimit <= val) && (val < hiLimit) ) ? -1L : 0 );
}

FORTHOP(dMinBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a < b ) ? a : b );
}

FORTHOP(dMaxBop)
{
    NEEDS(2);
    double b = DPOP;
    double a = DPOP;
    DPUSH( ( a > b ) ? a : b );
}

FORTHOP(dcmpBop)
{
	NEEDS(2);
	double b = DPOP;
	double a = DPOP;
	SPUSH( (a == b) ? 0 : (( a > b ) ? 1 : -1 ) );
}

//##############################
//
// stack manipulation
//

FORTHOP(rpushBop)
{
    NEEDS(1);
    RPUSH( SPOP );
}

FORTHOP(rpopBop)
{
    RNEEDS(1);
    SPUSH( RPOP );
}

FORTHOP(rpeekBop)
{
    SPUSH( *(GET_RP) );
}

FORTHOP(rdropBop)
{
    RNEEDS(1);
    RPOP;
}

FORTHOP(rpBop)
{
	intVarAction( pCore, (int *)&(pCore->RP) );
}

FORTHOP(r0Bop)
{
    cell pR0 = (cell) (pCore->RT);
    SPUSH( pR0 );
}

FORTHOP(dupBop)
{
    NEEDS(1);
    cell a = *(GET_SP);
    SPUSH( a );
}

FORTHOP(checkDupBop)
{
    NEEDS(1);
    cell a = *(GET_SP);
    if ( a != 0 )
    {
        SPUSH( a );
    }
}

FORTHOP(swapBop)
{
    NEEDS(2);
    cell a = *(GET_SP);
    *(GET_SP) = (GET_SP)[1];
    (GET_SP)[1] = a;
}

FORTHOP(dropBop)
{
    NEEDS(1);
    SPOP;
}

FORTHOP(overBop)
{
    NEEDS(1);
    cell a = (GET_SP)[1];
    SPUSH( a );
}

FORTHOP(rotBop)
{
    NEEDS(3);
    cell a, b, c;
    cell *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = b;
    pSP[1] = c;
    pSP[0] = a;
}

FORTHOP(reverseRotBop)
{
    NEEDS(3);
    cell a, b, c;
    cell *pSP = GET_SP;
    a = pSP[2];
    b = pSP[1];
    c = pSP[0];
    pSP[2] = c;
    pSP[1] = a;
    pSP[0] = b;
}

FORTHOP(nipBop)
{
    NEEDS(2);
    cell a = SPOP;
    SPOP;
    SPUSH( a );
}

FORTHOP(tuckBop)
{
    NEEDS(2);
    cell *pSP = GET_SP;
    cell a = *pSP;
    cell b = pSP[1];
    pSP[1] = a;
    *pSP = b;
    SPUSH( a );
}

FORTHOP(pickBop)
{
    NEEDS(1);
    cell *pSP = GET_SP;
    cell n = *pSP;
    cell a = pSP[n + 1];
    *pSP = a;
}

FORTHOP(rollBop)
{
    // TODO: moves the Nth element to TOS
    // 1 roll is the same as swap
    // 2 roll is the same as rot
    cell n = (SPOP);
    cell *pSP = GET_SP;
    cell a;
	if ( n != 0 )
	{
		if ( n > 0 )
		{
			a = pSP[n];
			for ( int i = n; i != 0; i-- )
			{
				pSP[i] = pSP[i - 1];
			}
			*pSP = a;
		}
		else
		{
			a = *pSP;
			n = -n;
			for ( int i = 0; i < n; i++ )
			{
				pSP[i] = pSP[i + 1];
			}
			pSP[n] = a;
		}
	}
}

FORTHOP(spBop)
{
    VarOperation varOp = GET_VAR_OPERATION;
#if defined(FORTH64)
    longVarAction(pCore, (int *)&(pCore->SP));
#else
    intVarAction(pCore, (int *)&(pCore->SP));
#endif
}

FORTHOP(s0Bop)
{
    cell pS0 = (cell) (pCore->ST);
    SPUSH( pS0 );
}

FORTHOP(fpBop)
{
#if defined(FORTH64)
    longVarAction(pCore, (int*)&(pCore->FP));
#else
    intVarAction(pCore, (int*)&(pCore->FP));
#endif
}

FORTHOP(ipBop)
{
#if defined(FORTH64)
    longVarAction(pCore, (int*)&(pCore->IP));
#else
    intVarAction(pCore, (int*)&(pCore->IP));
#endif
}

FORTHOP(ddupBop)
{
    NEEDS(2);
    cell *pSP = GET_SP;
    cell a = pSP[0];
    cell b = pSP[1];
    SPUSH(b);
    SPUSH(a);
}

FORTHOP(dswapBop)
{
    NEEDS(4);
    cell *pSP = GET_SP;
    cell a = pSP[0];
    pSP[0] = pSP[2];
    pSP[2] = a;
    a = pSP[1];
    pSP[1] = pSP[3];
    pSP[3] = a;
}

FORTHOP(ddropBop)
{
    NEEDS(2);
    SET_SP( GET_SP + 2 );
}

FORTHOP(doverBop)
{
    NEEDS(4);
    cell *pSP = GET_SP;
    cell a = pSP[2];
    cell b = pSP[3];
    SPUSH(b);
    SPUSH(a);
}

FORTHOP(drotBop)
{
    NEEDS(6);
    cell *pSP = GET_SP;
    cell a = pSP[0];
    cell b = pSP[1];
    pSP[0] = pSP[4];
    pSP[1] = pSP[5];
    pSP[4] = pSP[2];
    pSP[5] = pSP[3];
    pSP[2] = a;
    pSP[3] = b;
}
#endif

#if defined(FORTH64)
FORTHOP(dreverseRotBop)
{
    NEEDS(6);
    cell *pSP = GET_SP;
    cell a = pSP[4];
    cell b = pSP[5];
    pSP[4] = pSP[0];
    pSP[5] = pSP[1];
    pSP[0] = pSP[2];
    pSP[1] = pSP[3];
    pSP[2] = a;
    pSP[3] = b;
}

FORTHOP(dnipBop)
{
    NEEDS(4);
    cell *pSP = GET_SP;
    pSP[2] = pSP[0];
    pSP[3] = pSP[1];
    SET_SP(pSP + 2);
}

FORTHOP(dtuckBop)
{
    NEEDS(4);
    cell *pSP = GET_SP;
    cell a = pSP[0];
    cell b = pSP[1];
    pSP[0] = pSP[2];
    pSP[1] = pSP[3];
    pSP[2] = a;
    pSP[3] = b;
    SPUSH(b);
    SPUSH(a);
}

FORTHOP(dpickBop)
{
    NEEDS(1);
    cell n = SPOP;
    cell *pSP = (GET_SP + (n << 1));
    cell a = *pSP++;
    SPUSH(a);
    a = *pSP++;
    SPUSH(a);
}

FORTHOP(drollBop)
{
    // TODO: moves the Nth element to TOS
    // 1 droll is the same as dswap
    // 2 droll is the same as drot
    cell n = SPOP;
    cell* pSP = GET_SP;
    cell a;
    cell b;
	if ( n != 0 )
	{
		if ( n > 0 )
		{
            n <<= 1;
            a = pSP[n];
            b = pSP[n+1];
            for (int i = n; i != 0; i -= 2)
			{
                pSP[i] = pSP[i - 2];
                pSP[i + 1] = pSP[i - 1];
			}
            pSP[0] = a;
            pSP[1] = b;
		}
		else
		{
			n = (-n) << 1;
            a = pSP[0];
            b = pSP[1];
            for ( int i = 0; i < n; i += 2 )
			{
                pSP[i] = pSP[i + 2];
                pSP[i + 1] = pSP[i + 3];
            }
            pSP[n] = a;
            pSP[n + 1] = b;
		}
	}
}

FORTHOP(ndropBop)
{
    NEEDS(1);
    cell n = SPOP;
    SET_SP( GET_SP + n );
}

FORTHOP(ndupBop)
{
    NEEDS(1);
    cell n = (SPOP) - 1;
    cell* pSP = (GET_SP) + n;
	for (cell i = 0; i <= n; i++ )
	{
        cell v = *pSP--;
		SPUSH( v );
	}
}

FORTHOP(npickBop)
{
    NEEDS(1);
    cell n = (SPOP) - 1;
    cell offset = (SPOP);
    cell* pSP = (GET_SP) + offset + n;
	for (cell i = 0; i <= n; i++ )
	{
        cell v = *pSP--;
		SPUSH( v );
	}
	/*
	for ( int i = 0; i <= n; i++ )
	{
      int32_t* pSP = (GET_SP);
	  SPUSH( pSP[offset] );
	}
	*/
}
#endif

#ifndef ASM_INNER_INTERPRETER
FORTHOP(startTupleBop)
{
    cell pSP = (cell)(GET_SP);
    RPUSH(pSP);
}

FORTHOP(endTupleBop)
{
    cell* pSP = (GET_SP);
    cell* pOldSP = (cell *)(RPOP);
    cell count = pOldSP - pSP;
    SPUSH(count);
}

FORTHOP( noopBop )
{
}

FORTHOP(devolveBop)
{
    Engine* pEngine = GET_ENGINE;
    InterfaceObject interfaceObj = (InterfaceObject) pCore->TP;
    pCore->TP = interfaceObj->pWrappedObject;
}

FORTHOP(odropBop)
{
    Engine *pEngine = GET_ENGINE;
    ForthObject obj;

    // if object on TOS has refcount 0, invoke its delete method
    //  otherwise just drop it
    POP_OBJECT(obj);
    if ((obj != nullptr) && (obj->refCount == 0))
    {
        RPUSH(((cell)GET_TP));
        SET_TP(obj);
        pEngine->ExecuteOp(pCore, obj->pMethods[0]);
    }
}

//##############################
//
// loads & stores
//

FORTHOP(istoreBop)
{
    NEEDS(2);
    int* pB = (int*)(SPOP);
    int a = (int)SPOP;
    *pB = a;
}

FORTHOP(ifetchBop)
{
    NEEDS(1);
    int* pA = (int* )(SPOP); 
    SPUSH( *pA );
}

FORTHOP(uifetchBop)
{
    NEEDS(1);
    unsigned int* pA = (unsigned int*)(SPOP);
    SPUSH(*pA);
}

FORTHOP(istoreNextBop)
{
    NEEDS(2);
    int32_t **ppB = (int32_t **)(SPOP);
    int32_t *pB = *ppB;
    int32_t a = (int32_t)SPOP;
    *pB++ = a;
    *ppB = pB;
}

FORTHOP(ifetchNextBop)
{
    NEEDS(1);
    int32_t **ppA = (int32_t **)(SPOP);
    int32_t *pA = *ppA;
    int32_t a = *pA++;
    SPUSH(a);
    *ppA = pA;
}

FORTHOP(ubfetchBop)
{
    NEEDS(1);
    unsigned char *pA = (unsigned char *)(SPOP);
    SPUSH((*pA));
}

FORTHOP(bstoreBop)
{
    NEEDS(2);
    char *pB = (char *) (SPOP);
    int32_t a = SPOP;
    *pB = (char) a;
}

FORTHOP(bfetchBop)
{
    NEEDS(1);
    signed char *pA = (signed char *)(SPOP);
    SPUSH((*pA));
}

FORTHOP(bstoreNextBop)
{
    NEEDS(2);
    char **ppB = (char **)(SPOP);
    char *pB = *ppB;
    int32_t a = SPOP;
    *pB++ = (char) a;
    *ppB = pB;
}

FORTHOP(bfetchNextBop)
{
    NEEDS(1);
    signed char **ppA = (signed char **)(SPOP); 
    signed char *pA = *ppA;
    signed char a = *pA++;
    SPUSH( a );
    *ppA = pA;
}

FORTHOP(sstoreBop)
{
    NEEDS(2);
    short *pB = (short *) (SPOP);
    int32_t a = SPOP;
    *pB = (short) a;
}

FORTHOP(sfetchBop)
{
    NEEDS(1);
    short *pA = (short *) (SPOP);
    SPUSH( *pA );
}

FORTHOP(usfetchBop)
{
    NEEDS(1);
    unsigned short* pA = (unsigned short*)(SPOP);
    SPUSH(*pA);
}

FORTHOP(sstoreNextBop)
{
    NEEDS(2);
    short **ppB = (short **)(SPOP);
    short *pB = *ppB;
    int32_t a = SPOP;
    *pB++ = (short) a;
    *ppB = pB;
}

FORTHOP(sfetchNextBop)
{
    NEEDS(1);
    short **ppA = (short **)(SPOP); 
    short *pA = *ppA;
    short a = *pA++;
    SPUSH( a );
    *ppA = pA;
}

FORTHOP(lstoreBop)
{
    NEEDS(3);
	int64_t *pB = (int64_t *) (SPOP);
	stackInt64 a;
	LPOP(a);
    *pB = a.s64;
}

FORTHOP(lfetchBop)
{
    NEEDS(1);
	int64_t *pA = (int64_t *) (SPOP);
	stackInt64 b;
	b.s64 = *pA;
    LPUSH( b );
}

FORTHOP(lstoreNextBop)
{
    NEEDS(2);
	int64_t **ppB = (int64_t **) (SPOP);
    int64_t *pB = *ppB;
	stackInt64 a;
	LPOP(a);
    *pB++ = a.s64;
    *ppB = pB;
}

FORTHOP(lfetchNextBop)
{
    NEEDS(1);
	int64_t **ppA = (int64_t **) (SPOP);
	int64_t *pA = *ppA;
	stackInt64 a;
	a.s64 = *pA++;
	LPUSH( a );
    *ppA = pA;
}

FORTHOP(plusStoreCellBop)
{
    NEEDS(2);
    cell* p = (cell*)(SPOP);
    cell v = SPOP;
    *p += v;
}

FORTHOP(plusStoreAtomicCellBop)
{
    NEEDS(2);
    std::atomic<cell>* p = (std::atomic<cell>*)(SPOP);
    cell v = SPOP;
    cell result = p->fetch_add(v) + v;
    SPUSH(result);
}

FORTHOP(memcmpBop)
{
    NEEDS(3);
    size_t nBytes = (size_t)(SPOP);
    const void* pStr2 = (const void *)(SPOP);
    void* pStr1 = (void *)(SPOP);
    int result = memcmp(pStr1, pStr2, nBytes);
    SPUSH(result);
}

FORTHOP(moveBop)
{
    NEEDS(3);
    size_t nBytes = (size_t) (SPOP);
    void* pDst = (void *) (SPOP);
    const void* pSrc = (const void *) (SPOP);
    memmove( pDst, pSrc, nBytes );
}

FORTHOP(fillBop)
{
    NEEDS(3);
    int byteVal = (int) (SPOP);
    size_t nBytes = (size_t) (SPOP);
    void* pDst = (void *) (SPOP);
    memset( pDst, byteVal, nBytes );
}

FORTHOP(fetchVaractionBop)
{
	SET_VAR_OPERATION( VarOperation::kVarGet );
}

FORTHOP(intoVaractionBop)
{
    SET_VAR_OPERATION( VarOperation::kVarSet );
}

FORTHOP(addToVaractionBop)
{
    SET_VAR_OPERATION( VarOperation::kVarSetPlus );
}

FORTHOP(subtractFromVaractionBop)
{
    SET_VAR_OPERATION( VarOperation::kVarSetMinus );
}

FORTHOP(refVaractionBop)
{
    SET_VAR_OPERATION( VarOperation::kVarRef );
}

FORTHOP(setVaropBop)
{
    SET_VAR_OPERATION((VarOperation)(SPOP));
}

FORTHOP(getVaropBop)
{
    SPUSH((cell)(GET_VAR_OPERATION));
}

//##############################
//
// string ops
//

FORTHOP( strcpyBop )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcpy( pDst, pSrc );
}

FORTHOP( strncpyBop )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncpy( pDst, pSrc, maxBytes );
}

FORTHOP( strlenBop )
{
    char *pSrc = (char *) SPOP;
    SPUSH(  strlen( pSrc ) );
}

FORTHOP( strcatBop )
{
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strcat( pDst, pSrc );
}

FORTHOP( strncatBop )
{
	size_t maxBytes = (size_t) SPOP;
    char *pSrc = (char *) SPOP;
    char *pDst = (char *) SPOP;
    strncat( pDst, pSrc, maxBytes );
}


FORTHOP( strchrBop )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (cell) strchr( pStr, c ) );
}

FORTHOP( strrchrBop )
{
    int c = (int) SPOP;
    char *pStr = (char *) SPOP;
    SPUSH( (cell) strrchr( pStr, c ) );
}

FORTHOP( strcmpBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
	int result = strcmp( pStr1, pStr2 );
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (cell) result );
}

FORTHOP( stricmpBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
#if defined(WIN32)
	int result = stricmp( pStr1, pStr2 );
#elif defined(LINUX) || defined(MACOSX)
	int result = strcasecmp( pStr1, pStr2 );
#endif
	// only return 1, 0 or -1
	if ( result != 0 )
	{
		result = (result > 0) ? 1 : -1;
	}
	SPUSH( (cell) result );
}

FORTHOP(strncmpBop)
{
    int numChars = SPOP;
    char *pStr2 = (char *)SPOP;
    char *pStr1 = (char *)SPOP;
    int result = strncmp(pStr1, pStr2, numChars);
    // only return 1, 0 or -1
    if (result != 0)
    {
        result = (result > 0) ? 1 : -1;
    }
    SPUSH((cell)result);
}


FORTHOP( strstrBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (cell) strstr( pStr1, pStr2 ) );
}


FORTHOP( strtokBop )
{
    char *pStr2 = (char *) SPOP;
    char *pStr1 = (char *) SPOP;
    SPUSH( (cell) strtok( pStr1, pStr2 ) );
}

FORTHOP( initStringBop )
{
    int32_t len;
    int32_t* pStr;

    // TOS: maximum length, ptr to first char
    len = SPOP;
	pStr = (int32_t *) (SPOP);
	pStr[-2] = len;
    pStr[-1] = 0;
    *((char *) pStr) = 0;
}

FORTHOP( strFixupBop )
{
    int32_t *pSrcLongs = (int32_t *) SPOP;
    char* pSrc = (char *) pSrcLongs;
    pSrc[pSrcLongs[-2]] = '\0';
    int len = strlen( pSrc );
    if (len > pSrcLongs[-2])
    {
        // characters have been written past string storage end
        SET_ERROR( ForthError::kStringOverflow );
    }
    else
    {
        pSrcLongs[-1] = len;
    }
}

// push the immediately following literal signed 32-bit constant
FORTHOP(litBop)
{
    int32_t* pV = (int32_t*)(GET_IP);
    SPUSH(*pV);
    SET_IP( (forthop*)(pV + 1));
}

#if defined(FORTH64)
// push the immediately following literal unsigned 32-bit constant
FORTHOP(ulitBop)
{
    uint32_t* pV = (uint32_t*)(GET_IP);
    SPUSH(*pV);
    SET_IP((forthop*)(pV + 1));
}
#endif

// push the immediately following literal 64-bit constant
FORTHOP(dlitBop)
{
    double *pV = (double *) GET_IP;
    DPUSH( *pV++ );
    SET_IP( (forthop*) pV );
}

// doDoes
// - does not have precedence
// - is executed while executing the user word
// - it gets the parameter address of the user word
// 
FORTHOP( doDoesBop )
{
    SPUSH( RPOP );
}

FORTHOP( doIConstantBop )
{
    // IP points to data field
    SPUSH( *GET_IP );
    SET_IP( (forthop*) (RPOP) );
}

FORTHOP( doLConstantBop )
{
    // IP points to data field
    DPUSH( *((double *) (GET_IP)) );
    SET_IP( (forthop*) (RPOP) );
}

#endif

// push the immediately following literal 128-bit constant
FORTHOP(i128LitOp)
{
    double* pV = (double*)GET_IP;
    DPUSH(*pV++);
    DPUSH(*pV++);
    SET_IP((forthop*)pV);
}

FORTHOP(doI128ConstantOp)
{
    // IP points to data field
    DPUSH(*((double*)(GET_IP + 2)));
    DPUSH(*((double*)(GET_IP)));
    SET_IP((forthop*)(RPOP));
}

FORTHOP(dc2fOp)
{
    doubleCell inVal;
    double outVal;
    DCPOP(inVal);

#if defined(FORTH64) && defined(WINDOWS_BUILD)
    // double-cell (128-bit int) to 64-bit float conversion
    // turn into unsigned double + signFlag
    // then use modf to split it into integral multiple of 2^64 (hi part)
    // and fractional part of 2^64 (lo part)
    // and finally reapply the sign if needed
    bool isNegative = (inVal.cells[0] < 0);
    uint64_t hi = inVal.ucells[0];
    uint64_t lo = inVal.ucells[1];
    if (isNegative)
    {
        hi = ~hi;
        lo = (~lo) + 1;
        if (lo == 0)
        {
            hi++;
        }
    }
    double scale = 1.0;
    while (hi > 0)
    {
        lo >>= 1;
        if (hi & 1 != 0)
        {
            lo += 0x8000000000000000;
        }
        hi >>= 1;
        scale *= 2.0;
    }

    outVal = ((double)lo) * scale;

    if (isNegative)
    {
        outVal = -outVal;
    }
#else
    // on all 32-bit builds, or non-Windows builds, compiler can do all the work for us
    outVal = inVal.sdcell;
#endif
    DPUSH(outVal);
}

FORTHOP(f2dcOp)
{
    doubleCell outVal;
    double inVal = DPOP;

#if defined(FORTH64) && defined(WINDOWS_BUILD)
    // 64-bit float to double-cell (128-bit int) conversion
    // turn into unsigned int + signFlag
    // then shift 128-bits right until only 64-bits are needed,
    // keeping track of shifts in power-of-2 scale factor, which
    // is applied after we do 64-bit-int to 64-bit-float conversion,
    // and finally reapply the sign if needed
    bool isNegative = (inVal < 0.0);
    double hiWordValue = 18446744073709551616.0;    // 2^64
    if (isNegative)
    {
        inVal = -inVal;
    }
    double hiPart;
    double loPart = modf(inVal, &hiPart);
    uint64_t lo = loPart;
    uint64_t hi = hiPart;
    if (isNegative)
    {
        hi = ~hi;
        lo = (~lo) + 1;
        if (lo == 0)
        {
            hi++;
        }
    }
    outVal.ucells[1] = lo;
    outVal.ucells[0] = hi;
#else
    // on all 32-bit builds, or non-Windows builds, compiler can do all the work for us
    outVal.sdcell = inVal;
#endif
    DCPUSH(outVal);
}


#ifndef ASM_INNER_INTERPRETER


FORTHOP( thisBop )
{
    SPUSH( ((cell) GET_TP) );
}

// doStructOp is compiled at the start of each user-defined global structure instance
FORTHOP( doStructBop )
{
    // IP points to data field
    SPUSH( (cell) GET_IP );
    SET_IP( (forthop*) (RPOP) );
}

// doStructArrayOp is compiled as the only op of each user-defined global structure array instance
// the word after this op is the padded element length
// after that word is storage for structure 0 of the array
FORTHOP( doStructArrayBop )
{
    // IP points to data field
    forthop*pA = GET_IP;
    int32_t nBytes = (*pA++) * SPOP;

    SPUSH( ((cell) pA) + nBytes );
    SET_IP( (forthop*) (RPOP) );
}

FORTHOP( executeBop )
{
	forthop op = SPOP;
    Engine *pEngine = GET_ENGINE;
	pEngine->ExecuteOp(pCore,  op );
#if 0
    int32_t prog[2];
    forthop*oldIP = GET_IP;

	// execute the opcode
    prog[0] = SPOP;
    prog[1] = gCompiledOps[OP_DONE] ;
    
    SET_IP( prog );
    InnerInterpreter( pCore );
    SET_IP( oldIP );
#endif
}

FORTHOP(executeMethodBop)
{
    // TOS is object pointer, next is method opcode (not index)
    // this is compiled to implement super.xxx, and shouldn't be
    // used in normal code
    Engine* pEngine = GET_ENGINE;
    ForthObject obj;

    RPUSH(((cell)GET_TP));

    POP_OBJECT(obj);
    forthop methodOpcode = (forthop)(SPOP);
    if (obj == nullptr || obj->pMethods == nullptr)
    {
        SET_ERROR(ForthError::kBadObject);
        return;
    }

    SET_TP(obj);
    pEngine->ExecuteOp(pCore, methodOpcode);
}

//##############################
//
//  File ops
//
FORTHOP( fopenBop )
{
    NEEDS(2);
    char *pAccess = (char *) SPOP;
    char *pName = (char *) SPOP;

    FILE *pFP = pCore->pFileFuncs->fileOpen( pName, pAccess );
    SPUSH( (cell) pFP );
}

FORTHOP( fcloseBop )
{
    NEEDS(1);
    
    int result = pCore->pFileFuncs->fileClose( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fseekBop )
{
    int ctrl = SPOP;
    int offset = SPOP;
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileSeek( pFP, offset, ctrl );
    SPUSH( result );
}

FORTHOP( freadBop )
{
    NEEDS(4);
    
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pDst = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fwriteBop )
{
    
    NEEDS(4);
    FILE *pFP = (FILE *) SPOP;
    int numItems = SPOP;
    int itemSize = SPOP;
    void *pSrc = (void *) SPOP;
    
    int result = pCore->pFileFuncs->fileWrite( pSrc, itemSize, numItems, pFP );
    SPUSH( result );
}

FORTHOP( fgetcBop )
{    
    NEEDS(1);
    FILE *pFP = (FILE *) SPOP;
    
    int result = pCore->pFileFuncs->fileGetChar( pFP );
    SPUSH( result );
}

FORTHOP( fputcBop )
{    
    NEEDS(2);
    FILE *pFP = (FILE *) SPOP;
    int outChar = SPOP;
    
    int result = pCore->pFileFuncs->filePutChar( outChar, pFP );
    SPUSH( result );
}

FORTHOP( feofBop )
{
    NEEDS(1);

    int result = pCore->pFileFuncs->fileAtEnd( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( fexistsBop )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileExists( (const char*) SPOP );
    SPUSH( result );
}

FORTHOP( ftellBop )
{
    NEEDS(1);
    int result = pCore->pFileFuncs->fileTell( (FILE *) SPOP );
    SPUSH( result );
}

FORTHOP( flenBop )
{
    NEEDS(1);
    FILE* pFile = (FILE *) SPOP;
    int result = pCore->pFileFuncs->fileGetLength( pFile );
    SPUSH( result );
}

FORTHOP( fgetsBop )
{
    NEEDS( 3 );
    FILE* pFile = (FILE *) SPOP;
    int maxChars = SPOP;
    char* pBuffer = (char *) SPOP;
    cell result = (cell) pCore->pFileFuncs->fileGetString( pBuffer, maxChars, pFile );
    SPUSH( result );
}

FORTHOP( fputsBop )
{
    NEEDS( 2 );
    FILE* pFile = (FILE *) SPOP;
    char* pBuffer = (char *) SPOP;
    cell result = (cell) pCore->pFileFuncs->filePutString( pBuffer, pFile );
    SPUSH( result );
}

FORTHOP( hereBop )
{
    Engine *pEngine = GET_ENGINE;
	SPUSH((cell)(pCore->pDictionary->pCurrent));
}

FORTHOP( dpBop )
{
    Engine *pEngine = GET_ENGINE;
    int *pDP = (int *)&(pCore->pDictionary->pCurrent);
    SPUSH((cell)pDP);
}

FORTHOP(yieldBop)
{
    SET_STATE(OpResult::kYield);
}

FORTHOP( archARMBop )
{
#ifdef X86_ARCHITECTURE
    SPUSH( 0 );
#else
    SPUSH( ~0 );
#endif
}

FORTHOP( archX86Bop )
{
#ifdef X86_ARCHITECTURE
    SPUSH( ~0 );
#else
    SPUSH( 0 );
#endif
}

FORTHOP( oclearVaractionBop )
{
	SET_VAR_OPERATION( VarOperation::kVarClear );
}

FORTHOP(faddBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float *pDst = (float *)SPOP;
    float *pSrcB = (float *)SPOP;
    float *pSrcA = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) + (*pSrcB++);
    }
}

FORTHOP(fsubBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float *pDst = (float *)SPOP;
    float *pSrcB = (float *)SPOP;
    float *pSrcA = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) - (*pSrcB++);
    }
}

FORTHOP(fmulBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float *pDst = (float *)SPOP;
    float *pSrcB = (float *)SPOP;
    float *pSrcA = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) * (*pSrcB++);
    }
}

FORTHOP(fdivBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float *pDst = (float *)SPOP;
    float *pSrcB = (float *)SPOP;
    float *pSrcA = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) / (*pSrcB++);
    }
}

FORTHOP(fscaleBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float scale = FPOP;
    float *pDst = (float *)SPOP;
    float *pSrc = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrc++) * scale;
    }
}

FORTHOP(foffsetBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float offset = FPOP;
    float *pDst = (float *)SPOP;
    float *pSrc = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrc++) + offset;
    }
}

FORTHOP(fmixBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    float scale = FPOP;
    float *pDst = (float *)SPOP;
    float *pSrc = (float *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        float oldDst = *pDst;
        *pDst++ = ((*pSrc++) * scale) + oldDst;
    }
}

FORTHOP(daddBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    double *pDst = (double *)SPOP;
    double *pSrcB = (double *)SPOP;
    double *pSrcA = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) + (*pSrcB++);
    }
}

FORTHOP(dsubBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    double *pDst = (double *)SPOP;
    double *pSrcB = (double *)SPOP;
    double *pSrcA = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) - (*pSrcB++);
    }
}

FORTHOP(dmulBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    double *pDst = (double *)SPOP;
    double *pSrcB = (double *)SPOP;
    double *pSrcA = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) * (*pSrcB++);
    }
}

FORTHOP(ddivBlockBop)
{
    NEEDS(4);
    int num = SPOP;
    double *pDst = (double *)SPOP;
    double *pSrcB = (double *)SPOP;
    double *pSrcA = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrcA++) / (*pSrcB++);
    }
}

FORTHOP(dscaleBlockBop)
{
    NEEDS(5);
    int num = SPOP;
    double scale = DPOP;
    double *pDst = (double *)SPOP;
    double *pSrc = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrc++) * scale;
    }
}

FORTHOP(doffsetBlockBop)
{
    NEEDS(5);
    int num = SPOP;
    double offset = DPOP;
    double *pDst = (double *)SPOP;
    double *pSrc = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        *pDst++ = (*pSrc++) + offset;
    }
}

FORTHOP(dmixBlockBop)
{
    NEEDS(5);
    int num = SPOP;
    double scale = DPOP;
    double *pDst = (double *)SPOP;
    double *pSrc = (double *)SPOP;
    for (int i = 0; i < num; ++i)
    {
        double oldDst = *pDst;
        *pDst++ = ((*pSrc++) * scale) + oldDst;
    }
}

#endif

//##############################
//
// ops used to implement object show methods
//

FORTHOP(scHandleAlreadyShownOp)
{
	int result = -1;
	if (ForthShowAlreadyShownObject(GET_TP, pCore, true))
	{
		result = 0;
	}
	SPUSH(result);
}

FORTHOP(scBeginIndentOp)
{
    GET_SHOW_CONTEXT;
    pShowContext->BeginIndent();
}

FORTHOP(scEndIndentOp)
{
    GET_SHOW_CONTEXT;
    pShowContext->EndIndent();
}

FORTHOP(scShowHeaderOp)
{
    GET_SHOW_CONTEXT;
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(GET_TP);
    pShowContext->ShowHeader(pCore, pClassObject->pVocab->GetName(), GET_TP);
}

FORTHOP(scShowIndentOp)
{
    GET_SHOW_CONTEXT;
    const char* pStr = (const char *)(SPOP);
    pShowContext->ShowIndent(pStr);
}

FORTHOP(scBeginFirstElementOp)
{
    GET_SHOW_CONTEXT;
    const char* pStr = (const char *)(SPOP);
    pShowContext->BeginFirstElement(pStr);
}

FORTHOP(scBeginNextElementOp)
{
    GET_SHOW_CONTEXT;
    const char* pStr = (const char *)(SPOP);
    pShowContext->BeginNextElement(pStr);
}

FORTHOP(scEndElementOp)
{
    GET_SHOW_CONTEXT;
    const char* pStr = (const char *)(SPOP);
    pShowContext->EndElement(pStr);
}

FORTHOP(scShowObjectOp)
{
	ForthObject obj;
	POP_OBJECT(obj);
	ForthShowObject(obj, pCore);
}

FORTHOP(scSetShowIDOp)
{
    GET_SHOW_CONTEXT;
    int show = SPOP;
    pShowContext->SetShowIDElement(show != 0);
}

FORTHOP(scGetShowIDOp)
{
    GET_SHOW_CONTEXT;
    bool show = pShowContext->GetShowIDElement();
	SPUSH(show ? -1 : 0);
}

FORTHOP(scSetShowRefCountOp)
{
    GET_SHOW_CONTEXT;
    int show = SPOP;
    pShowContext->SetShowRefCount(show != 0);
}

FORTHOP(scGetShowRefCountOp)
{
    GET_SHOW_CONTEXT;
    bool show = pShowContext->GetShowRefCount();
	SPUSH(show ? -1 : 0);
}

// NOTE: the order of the first few entries in this table must agree
// with the list near the top of the file!  (look for COMPILED_OP)

extern GFORTHOP( doByteBop );
extern GFORTHOP( doUByteBop );
extern GFORTHOP( doShortBop );
extern GFORTHOP( doUShortBop );
extern GFORTHOP( doIntBop );
extern GFORTHOP( doUIntBop );
extern GFORTHOP( doFloatBop );
extern GFORTHOP( doDoubleBop );
extern GFORTHOP( doStringBop );
extern GFORTHOP( doOpBop );
extern GFORTHOP( doLongBop );
extern GFORTHOP( doObjectBop );
extern GFORTHOP( doByteArrayBop );
extern GFORTHOP( doUByteArrayBop );
extern GFORTHOP( doShortArrayBop );
extern GFORTHOP( doUShortArrayBop );
extern GFORTHOP( doIntArrayBop );
extern GFORTHOP( doUIntArrayBop );
extern GFORTHOP( doFloatArrayBop );
extern GFORTHOP( doDoubleArrayBop );
extern GFORTHOP( doStringArrayBop );
extern GFORTHOP( doOpArrayBop );
extern GFORTHOP( doLongArrayBop );
extern GFORTHOP( doObjectArrayBop );

#ifdef ASM_INNER_INTERPRETER

#define OPREF extern GFORTHOP

OPREF( abortBop );          OPREF( dropBop );           OPREF( doDoesBop );
OPREF( litBop );            OPREF( dlitBop );           OPREF( ulitBop );
OPREF(doVariableBop);       OPREF( doIConstantBop );    OPREF( doneBop );
OPREF(doLConstantBop);
OPREF( doByteBop );         OPREF( doUByteBop );        OPREF( doShortBop );
OPREF( doUShortBop );       OPREF( doIntBop );          OPREF( doLongBop );
OPREF( doFloatBop );        OPREF( doDoubleBop );       OPREF( doStringBop );
OPREF( doOpBop );           OPREF( doObjectBop );       OPREF( doExitBop );
OPREF( doExitLBop );        OPREF( doExitMBop );        OPREF( doExitMLBop );
OPREF( doByteArrayBop );    OPREF( doUByteArrayBop );   OPREF( doShortArrayBop );
OPREF( doUShortArrayBop );  OPREF( doIntArrayBop );     OPREF( doIntArrayBop );
OPREF( doLongArrayBop );    OPREF( doLongArrayBop );    OPREF( doFloatArrayBop );
OPREF( doDoubleArrayBop );  OPREF( doStringArrayBop );  OPREF( doOpArrayBop );
OPREF( doObjectArrayBop );  OPREF( initStringBop );     OPREF( plusBop );
OPREF( strFixupBop );       OPREF( fetchVaractionBop );	OPREF( noopBop );
OPREF(odropBop);            OPREF(devolveBop);          OPREF(executeMethodBop);

OPREF( ifetchBop );         OPREF( doStructBop );       OPREF( doStructArrayBop );
OPREF(uifetchBop);          OPREF(usfetchBop);
OPREF( doDoBop );           OPREF( doLoopBop );         OPREF( doLoopNBop );
OPREF( dfetchBop );         OPREF( doCheckDoBop );
OPREF( thisBop );           OPREF( dpBop );
OPREF( executeBop );        OPREF( callBop );           OPREF( gotoBop );
OPREF( iBop );              OPREF( jBop );              OPREF( unloopBop );
OPREF( leaveBop );          OPREF( hereBop );           OPREF( refVaractionBop );
OPREF( intoVaractionBop );  OPREF( addToVaractionBop ); OPREF( subtractFromVaractionBop );
OPREF( minusBop );          OPREF( timesBop );          OPREF( times2Bop );
OPREF( times4Bop );         OPREF( times8Bop );         OPREF( divideBop );
OPREF( divide2Bop );        OPREF( divide4Bop );        OPREF( divide8Bop );
OPREF( divmodBop );         OPREF( modBop );            OPREF( negateBop );
OPREF( fplusBop );          OPREF( fminusBop );         OPREF( ftimesBop );
OPREF( fdivideBop );        OPREF( fEqualsBop );        OPREF( fNotEqualsBop );
OPREF( fGreaterThanBop );   OPREF( fGreaterEqualsBop ); OPREF( fLessThanBop );
OPREF( fLessEqualsBop );    OPREF( fEquals0Bop );       OPREF( fNotEquals0Bop );
OPREF( fLessThan0Bop );     OPREF( fGreaterThan0Bop );  OPREF( fGreaterEquals0Bop );
OPREF( fLessEquals0Bop );   OPREF( fWithinBop );        OPREF( fMinBop );
OPREF( fMaxBop );           OPREF( dplusBop );          OPREF( dminusBop );
OPREF( dtimesBop );         OPREF( ddivideBop );        OPREF( dEqualsBop );
OPREF( dNotEqualsBop );     OPREF( dGreaterThanBop );   OPREF( dGreaterEqualsBop );
OPREF( dLessThanBop );      OPREF( dLessEqualsBop );    OPREF( dEquals0Bop );
OPREF( dNotEquals0Bop );    OPREF( dGreaterThan0Bop );  OPREF( dGreaterEquals0Bop );
OPREF( dLessThan0Bop );     OPREF( dLessEquals0Bop );   OPREF( dWithinBop );
OPREF( dMinBop );           OPREF( dMaxBop );           OPREF( dsinBop );
OPREF( dasinBop );          OPREF( dcosBop );           OPREF( dacosBop );
OPREF( dtanBop );           OPREF( datanBop );          OPREF( datan2Bop );
OPREF( dexpBop );           OPREF( dlnBop );            OPREF( dlog10Bop );
OPREF( dpowBop );           OPREF( dsqrtBop );          OPREF( dceilBop );
OPREF( dfloorBop );         OPREF( dabsBop );           OPREF( dldexpBop );
OPREF( dfrexpBop );         OPREF( dmodfBop );          OPREF( dfmodBop );
OPREF( mtimesBop );         OPREF( umtimesBop);
OPREF( i2fBop );            OPREF( i2dBop );            OPREF( f2iBop );
OPREF( f2dBop );            OPREF( d2iBop );            OPREF( d2fBop );
OPREF( orBop );             OPREF( andBop );            OPREF( xorBop );
OPREF( invertBop );         OPREF( ilshiftBop );        OPREF( irshiftBop );
#if !defined(FORTH64)
OPREF(lplusBop);            OPREF(lminusBop);           OPREF(ltimesBop);
#endif
OPREF(llshiftBop);        OPREF(lrshiftBop);          OPREF(lrotateBop);
OPREF(icmpBop);             OPREF(uicmpBop);
OPREF(lcmpBop);			    OPREF(ulcmpBop);
OPREF( arshiftBop );        OPREF( irotateBop );         OPREF( trueBop );
OPREF( falseBop );          OPREF( nullBop );           OPREF( dnullBop );
OPREF( equalsBop );         OPREF( notEqualsBop );      OPREF( greaterThanBop );
OPREF( greaterEqualsBop );  OPREF( lessThanBop );       OPREF( lessEqualsBop );
OPREF( equals0Bop );        OPREF( notEquals0Bop );     OPREF( greaterThan0Bop );
OPREF( greaterEquals0Bop ); OPREF( lessThan0Bop );      OPREF( lessEquals0Bop );
OPREF( unsignedGreaterThanBop );                        OPREF( unsignedLessThanBop );
OPREF( withinBop );         OPREF( minBop );            OPREF( maxBop );
OPREF( fcmpBop );           OPREF( dcmpBop );
OPREF(ireverseBop);          OPREF(countLeadingZerosBop); OPREF(countTrailingZerosBop);

#if !defined(FORTH64)
OPREF( lEqualsBop );        OPREF( lNotEqualsBop );     OPREF( lEquals0Bop );
OPREF( lNotEquals0Bop );    OPREF( lGreaterThanBop );   OPREF( lGreaterThan0Bop );
OPREF( lLessThanBop );      OPREF( lLessThan0Bop );     OPREF( fcmpBop );
OPREF( lGreaterEqualsBop ); OPREF( lGreaterEquals0Bop ); OPREF( lLessEqualsBop );
OPREF( lLessEquals0Bop );   OPREF(doDConstantBop);
#endif
OPREF(lfetchBop);           OPREF(plusStoreCellBop);  OPREF(plusStoreAtomicCellBop);
OPREF(lstoreBop);           OPREF(lstoreNextBop);     OPREF(lfetchNextBop);

OPREF( rpushBop );          OPREF( rpopBop );           OPREF( rpeekBop );
OPREF( rdropBop );          OPREF( rpBop );             OPREF( r0Bop );
OPREF( dupBop );            OPREF( checkDupBop );       OPREF( swapBop );
OPREF( overBop );           OPREF( rotBop );            OPREF( reverseRotBop );
OPREF( nipBop );            OPREF( tuckBop );           OPREF( pickBop );
OPREF( spBop );             OPREF( s0Bop );             OPREF( fpBop );
OPREF( ipBop );             OPREF( ddupBop );           OPREF( dswapBop );
OPREF( ddropBop );          OPREF( doverBop );          OPREF( drotBop );
OPREF( startTupleBop );     OPREF( endTupleBop );
OPREF( istoreBop );         OPREF( istoreNextBop);      OPREF( ifetchNextBop);
OPREF(bstoreBop);           OPREF(ubfetchBop);
OPREF( bfetchBop );         OPREF( bstoreNextBop );     OPREF( bfetchNextBop );
OPREF( sbfetchBop );        OPREF( sstoreBop );
OPREF( sfetchBop );         OPREF( sstoreNextBop );     OPREF( sfetchNextBop );

OPREF( moveBop );           OPREF( fillBop );           OPREF( setVaropBop );
OPREF(memcmpBop);
OPREF( getVaropBop );   OPREF( byteVarActionBop );  OPREF( ubyteVarActionBop );
OPREF( shortVarActionBop ); OPREF( ushortVarActionBop ); OPREF( intVarActionBop );
#ifdef FORTH64
OPREF(uintVarActionBop);
#endif
OPREF( longVarActionBop );  OPREF( floatVarActionBop ); OPREF( doubleVarActionBop );
OPREF( stringVarActionBop ); OPREF( opVarActionBop );   OPREF( objectVarActionBop );
OPREF( strcpyBop );         OPREF( strncpyBop );        OPREF( strlenBop );
OPREF( strcatBop );         OPREF( strncatBop );        OPREF( strchrBop );
OPREF( strrchrBop );        OPREF( strcmpBop );         OPREF( stricmpBop );
OPREF( strstrBop );         OPREF( strtokBop );         OPREF( fopenBop );
OPREF( fcloseBop );         OPREF( fseekBop );          OPREF( freadBop );
OPREF( fwriteBop );         OPREF( fgetcBop );          OPREF( fputcBop );
OPREF( feofBop );           OPREF( fexistsBop );        OPREF( ftellBop );
OPREF( flenBop );           OPREF( fgetsBop );          OPREF( fputsBop );
OPREF( archX86Bop );        OPREF( archARMBop );        OPREF( oclearVaractionBop );
OPREF( fsinBop );           OPREF( fasinBop );          OPREF( fcosBop );
OPREF( facosBop );          OPREF( ftanBop );           OPREF( fatanBop );
OPREF( fatan2Bop );         OPREF( fexpBop );           OPREF( flnBop );
OPREF( flog10Bop );         OPREF( fpowBop );           OPREF( fsqrtBop );
OPREF( fceilBop );          OPREF( ffloorBop );         OPREF( fabsBop );
OPREF( fldexpBop );         OPREF( ffrexpBop );         OPREF( fmodfBop );
OPREF( ffmodBop );          OPREF( setTraceBop );       OPREF( strncmpBop );
OPREF(faddBlockBop);        OPREF(fsubBlockBop);        OPREF(fmulBlockBop);
OPREF(fdivBlockBop);        OPREF(fscaleBlockBop);      OPREF(foffsetBlockBop);
OPREF(fmixBlockBop);
OPREF(daddBlockBop);        OPREF(dsubBlockBop);        OPREF(dmulBlockBop);
OPREF(ddivBlockBop);        OPREF(dscaleBlockBop);      OPREF(doffsetBlockBop);
OPREF(dmixBlockBop);
OPREF(yieldBop);
#endif

baseDictionaryCompiledEntry baseCompiledDictionary[] =
{
	//NATIVE_COMPILED_DEF(    abortBop,                "abort",			OP_ABORT ),
	OP_COMPILED_DEF(    abortOp,                     "abort",			OP_ABORT ),
    NATIVE_COMPILED_DEF(    dropBop,                 "drop",			OP_DROP ),
    NATIVE_COMPILED_DEF(    doDoesBop,               "_doDoes",			OP_DO_DOES ),
    NATIVE_COMPILED_DEF(    litBop,                  "ilit",			OP_INT_VAL ),
#if defined(FORTH64)
    NATIVE_COMPILED_DEF(    ulitBop,                  "uilit",			OP_UINT_VAL),
#else
    NATIVE_COMPILED_DEF(    litBop,                   "uilit",			OP_UINT_VAL),
#endif
    NATIVE_COMPILED_DEF(    litBop,                  "sflit",			OP_FLOAT_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "flit",			OP_DOUBLE_VAL ),
    NATIVE_COMPILED_DEF(    dlitBop,                 "llit",			OP_LONG_VAL ),
    NATIVE_COMPILED_DEF(    doVariableBop,           "_doVariable",	    OP_DO_VAR ),
    NATIVE_COMPILED_DEF(    doIConstantBop,          "_doIConstant",	OP_DO_ICONSTANT ),
    NATIVE_COMPILED_DEF(    doLConstantBop,          "_doLConstant",	OP_DO_LCONSTANT ),
    NATIVE_COMPILED_DEF(    doneBop,                 "done",			OP_DONE ),
    NATIVE_COMPILED_DEF(    doByteBop,               "_doByte",			OP_DO_BYTE ),
    NATIVE_COMPILED_DEF(    doUByteBop,              "_doUByte",		OP_DO_UBYTE ),
    NATIVE_COMPILED_DEF(    doShortBop,              "_doShort",		OP_DO_SHORT ),
	NATIVE_COMPILED_DEF(    doUShortBop,             "_doUShort",		OP_DO_USHORT ),
    NATIVE_COMPILED_DEF(    doIntBop,                "_doInt",			OP_DO_INT ),
#if defined(FORTH64)
    NATIVE_COMPILED_DEF(    doUIntBop,               "_doUInt",			OP_DO_UINT),
#else
    NATIVE_COMPILED_DEF(    doIntBop,                "_doUInt",			OP_DO_UINT),
#endif
    NATIVE_COMPILED_DEF(    doLongBop,               "_doLong",			OP_DO_LONG ),
    NATIVE_COMPILED_DEF(    doLongBop,               "_doULong",		OP_DO_ULONG ),
    NATIVE_COMPILED_DEF(    doFloatBop,              "_doSFloat",		OP_DO_FLOAT ),
    NATIVE_COMPILED_DEF(    doDoubleBop,             "_doFloat",		OP_DO_DOUBLE ),
    NATIVE_COMPILED_DEF(    doStringBop,             "_doString",		OP_DO_STRING ),
    NATIVE_COMPILED_DEF(    doOpBop,                 "_doOp",			OP_DO_OP ),
    NATIVE_COMPILED_DEF(    doObjectBop,             "_doObject",		OP_DO_OBJECT ),
    NATIVE_COMPILED_DEF(    doExitBop,               "_exit",			OP_DO_EXIT ),      // exit normal op with no vars
    NATIVE_COMPILED_DEF(    doExitLBop,              "_exitL",			OP_DO_EXIT_L ),    // exit normal op with local vars
    NATIVE_COMPILED_DEF(    doExitMBop,              "_exitM",			OP_DO_EXIT_M ),    // exit method op with no vars
    NATIVE_COMPILED_DEF(    doExitMLBop,             "_exitML",			OP_DO_EXIT_ML ),   // exit method op with local vars
	NATIVE_COMPILED_DEF(    doByteArrayBop,          "_doByteArray",	OP_DO_BYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doUByteArrayBop,         "_doUByteArray",	OP_DO_UBYTE_ARRAY ),
    NATIVE_COMPILED_DEF(    doShortArrayBop,         "_doShortArray",	OP_DO_SHORT_ARRAY ),
    NATIVE_COMPILED_DEF(    doUShortArrayBop,        "_doUShortArray",	OP_DO_USHORT_ARRAY ),	// 32
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doIntArray",		OP_DO_INT_ARRAY ),
    NATIVE_COMPILED_DEF(    doIntArrayBop,           "_doUIntArray",	OP_DO_UINT_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doLongArray",	OP_DO_LONG_ARRAY ),
    NATIVE_COMPILED_DEF(    doLongArrayBop,          "_doULongArray",	OP_DO_ULONG_ARRAY ),	// 36
    NATIVE_COMPILED_DEF(    doFloatArrayBop,         "_doSFloatArray",	OP_DO_FLOAT_ARRAY ),
    NATIVE_COMPILED_DEF(    doDoubleArrayBop,        "_doFloatArray",	OP_DO_DOUBLE_ARRAY ),
    NATIVE_COMPILED_DEF(    doStringArrayBop,        "_doStringArray",	OP_DO_STRING_ARRAY ),
    NATIVE_COMPILED_DEF(    doOpArrayBop,            "_doOpArray",		OP_DO_OP_ARRAY ),		// 40
    NATIVE_COMPILED_DEF(    doObjectArrayBop,        "_doObjectArray",	OP_DO_OBJECT_ARRAY ),
	NATIVE_COMPILED_DEF(    initStringBop,           "initString",		OP_INIT_STRING ),
    NATIVE_COMPILED_DEF(    plusBop,                 "+",				OP_PLUS ),				// 44
#if defined(FORTH64)
    NATIVE_COMPILED_DEF(    lfetchBop,               "@",				OP_IFETCH ),
#else
    NATIVE_COMPILED_DEF(    ifetchBop,               "@",				OP_IFETCH ),
#endif
    NATIVE_COMPILED_DEF(    doStructBop,             "_doStruct",		OP_DO_STRUCT ),
    NATIVE_COMPILED_DEF(    doStructArrayBop,        "_doStructArray",	OP_DO_STRUCT_ARRAY ),	// 48
    NATIVE_COMPILED_DEF(    doDoBop,                 "_do",				OP_DO_DO ),				// 52
    NATIVE_COMPILED_DEF(    doLoopBop,               "_loop",			OP_DO_LOOP ),
    NATIVE_COMPILED_DEF(    doLoopNBop,              "_+loop",			OP_DO_LOOPN ),
    // the order of the next four opcodes has to match the order of kVarRef...kVarSetMinus
    NATIVE_COMPILED_DEF(    fetchVaractionBop,       "fetch",			OP_FETCH ),				// 56
    NATIVE_COMPILED_DEF(    refVaractionBop,         "ref",				OP_REF ),
	NATIVE_COMPILED_DEF(    intoVaractionBop,        "->",				OP_INTO ),				// 59
    NATIVE_COMPILED_DEF(    addToVaractionBop,       "->+",				OP_INTO_PLUS ),
    NATIVE_COMPILED_DEF(    subtractFromVaractionBop, "->-",			OP_INTO_MINUS ),
    NATIVE_COMPILED_DEF(    oclearVaractionBop,      "oclear",          OP_OCLEAR ),
    NATIVE_COMPILED_DEF(    setVaropBop,             "setVarop",        OP_SETVAROP ),
    NATIVE_COMPILED_DEF(    doCheckDoBop,            "_?do",			OP_DO_CHECKDO ),

	OP_COMPILED_DEF(		doVocabOp,              "_doVocab",			OP_DO_VOCAB ),
	OP_COMPILED_DEF(		getClassByIndexOp,      "getClassByIndex",	OP_GET_CLASS_BY_INDEX ),
	OP_COMPILED_DEF(		initStringArrayOp,      "initStringArray",	OP_INIT_STRING_ARRAY ),
    OP_COMPILED_DEF(		badOpOp,                "badOp",			OP_BAD_OP ),
	OP_COMPILED_DEF(		doStructTypeOp,         "_doStructType",	OP_DO_STRUCT_TYPE ),
    OP_COMPILED_DEF(		doClassTypeOp,          "_doClassType",		OP_DO_CLASS_TYPE ),
    OP_COMPILED_DEF(	    doEnumOp,               "_doEnum",			OP_DO_ENUM ),
    OP_COMPILED_DEF(		doNewOp,                "_doNew",			OP_DO_NEW ),
    OP_COMPILED_DEF(		allocObjectOp,          "_allocObject",		OP_ALLOC_OBJECT ),
    OP_COMPILED_DEF(		endBuildsOp,            "_endBuilds",		OP_END_BUILDS ),
    OP_COMPILED_DEF(		compileOp,              "compile",		    OP_COMPILE ),
	OP_COMPILED_DEF(		initStructArrayOp,      "initStructArray",	OP_INIT_STRUCT_ARRAY ),
	NATIVE_COMPILED_DEF(    dupBop,					"dup",				OP_DUP ),
	NATIVE_COMPILED_DEF(    overBop,				"over",				OP_OVER ),
    OP_COMPILED_DEF(        doTryOp,                "_doTry",           OP_DO_TRY ),
    OP_COMPILED_DEF(        doFinallyOp,            "_doFinally",       OP_DO_FINALLY ),
    OP_COMPILED_DEF(        doEndtryOp,             "_doEndtry",        OP_DO_ENDTRY ),
    OP_COMPILED_DEF(        raiseOp,                "raise",            OP_RAISE),
    NATIVE_COMPILED_DEF(    rdropBop,               "rdrop",            OP_RDROP),
    NATIVE_COMPILED_DEF(    noopBop,                "noop",             OP_NOOP),
    NATIVE_COMPILED_DEF(    devolveBop,             "_devo",            OP_DEVOLVE),
    OP_COMPILED_DEF(unimplementedMethodOp,          "unimplementedMethod", OP_UNIMPLEMENTED),
    NATIVE_COMPILED_DEF(    executeMethodBop,       "_executeMethod",   OP_EXECUTE_METHOD),
    NATIVE_COMPILED_DEF(    thisBop,                "this",             OP_THIS),
    OP_COMPILED_DEF(        doI128ConstantOp,       "_doI128Constant",	OP_DO_I128_CONSTANT),
    OP_COMPILED_DEF(        i128LitOp,              "i128lit",			OP_I128_VAL),
};



baseDictionaryEntry baseDictionary[] =
{
    NATIVE_DEF(    executeBop,              "execute" ),
    NATIVE_DEF(    callBop,                 "call" ),
    NATIVE_DEF(    gotoBop,                 "setIP" ),
    NATIVE_DEF(    iBop,                    "i" ),
    NATIVE_DEF(    jBop,                    "j" ),
    NATIVE_DEF(    unloopBop,               "unloop" ),
    NATIVE_DEF(    leaveBop,                "leave" ),
    NATIVE_DEF(    hereBop,                 "here" ),
    NATIVE_DEF(    dpBop,                   "dp" ),
    NATIVE_DEF(    yieldBop,                "yield" ),

    NATIVE_DEF(    fetchVaractionBop,       "fetch" ),
    NATIVE_DEF(    odropBop,                "odrop"),

	// object varActions
    NATIVE_DEF(    subtractFromVaractionBop, "unref" ),

    ///////////////////////////////////////////
    //  integer math
    ///////////////////////////////////////////
    NATIVE_DEF(    minusBop,                "-" ),
    NATIVE_DEF(    timesBop,                "*" ),
    NATIVE_DEF(    times2Bop,               "2*" ),
    NATIVE_DEF(    times4Bop,               "4*" ),
    NATIVE_DEF(    times8Bop,               "8*" ),
    NATIVE_DEF(    divideBop,               "/" ),
    NATIVE_DEF(    divide2Bop,              "2/" ),
    NATIVE_DEF(    divide4Bop,              "4/" ),
    NATIVE_DEF(    divide8Bop,              "8/" ),
    NATIVE_DEF(    divmodBop,               "/mod" ),
    NATIVE_DEF(    modBop,                  "mod" ),
    NATIVE_DEF(    negateBop,               "negate" ),
    
    ///////////////////////////////////////////
    //  single-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    fplusBop,                "sf+" ),
    NATIVE_DEF(    fminusBop,               "sf-" ),
    NATIVE_DEF(    ftimesBop,               "sf*" ),
    NATIVE_DEF(    fdivideBop,              "sf/" ),
    
    ///////////////////////////////////////////
    //  single-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    fEqualsBop,              "sf=" ),
    NATIVE_DEF(    fNotEqualsBop,           "sf<>" ),
    NATIVE_DEF(    fGreaterThanBop,         "sf>" ),
    NATIVE_DEF(    fGreaterEqualsBop,       "sf>=" ),
    NATIVE_DEF(    fLessThanBop,            "sf<" ),
    NATIVE_DEF(    fLessEqualsBop,          "sf<=" ),
    NATIVE_DEF(    fEquals0Bop,             "sf0=" ),
    NATIVE_DEF(    fNotEquals0Bop,          "sf0<>" ),
    NATIVE_DEF(    fGreaterThan0Bop,        "sf0>" ),
    NATIVE_DEF(    fGreaterEquals0Bop,      "sf0>=" ),
    NATIVE_DEF(    fLessThan0Bop,           "sf0<" ),
    NATIVE_DEF(    fLessEquals0Bop,         "sf0<=" ),
    NATIVE_DEF(    fWithinBop,              "sfwithin" ),
    NATIVE_DEF(    fMinBop,                 "sfmin" ),
    NATIVE_DEF(    fMaxBop,                 "sfmax" ),
    NATIVE_DEF(    fcmpBop,                 "sfcmp" ),

    ///////////////////////////////////////////
    //  double-precision fp math
    ///////////////////////////////////////////
    NATIVE_DEF(    lfetchBop,               "f@"),
    NATIVE_DEF(    lstoreBop,               "f!"),
    NATIVE_DEF(    dplusBop,                "f+" ),
    NATIVE_DEF(    dminusBop,               "f-" ),
    NATIVE_DEF(    dtimesBop,               "f*" ),
    NATIVE_DEF(    ddivideBop,              "f/" ),


    ///////////////////////////////////////////
    //  double-precision fp comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    dEqualsBop,              "f=" ),
    NATIVE_DEF(    dNotEqualsBop,           "f<>" ),
    NATIVE_DEF(    dGreaterThanBop,         "f>" ),
    NATIVE_DEF(    dGreaterEqualsBop,       "f>=" ),
    NATIVE_DEF(    dLessThanBop,            "f<" ),
    NATIVE_DEF(    dLessEqualsBop,          "f<=" ),
    NATIVE_DEF(    dEquals0Bop,             "f0=" ),
    NATIVE_DEF(    dNotEquals0Bop,          "f0<>" ),
    NATIVE_DEF(    dGreaterThan0Bop,        "f0>" ),
    NATIVE_DEF(    dGreaterEquals0Bop,      "f0>=" ),
    NATIVE_DEF(    dLessThan0Bop,           "f0<" ),
    NATIVE_DEF(    dLessEquals0Bop,         "f0<=" ),
    NATIVE_DEF(    dWithinBop,              "fwithin" ),
    NATIVE_DEF(    dMinBop,                 "fmin" ),
    NATIVE_DEF(    dMaxBop,                 "fmax" ),
    NATIVE_DEF(    dcmpBop,                 "fcmp" ),

    ///////////////////////////////////////////
    //  integer/int32_t/float/double conversion
    ///////////////////////////////////////////
    NATIVE_DEF(    i2fBop,                  "i2sf" ), 
    NATIVE_DEF(    i2dBop,                  "i2f" ),
    NATIVE_DEF(    f2iBop,                  "sf2i" ),
    NATIVE_DEF(    f2dBop,                  "sf2f" ),
    NATIVE_DEF(    d2iBop,                  "f2i" ),
    NATIVE_DEF(    d2fBop,                  "f2sf" ),
    OP_DEF(    i2lOp,                  "i2l" ), 
    OP_DEF(    l2fOp,                  "l2sf" ), 
    OP_DEF(    l2dOp,                  "l2f" ), 
    OP_DEF(    f2lOp,                  "sf2l" ),
    OP_DEF(    d2lOp,                  "f2l" ),
    OP_DEF(    dc2fOp,                  "d>f" ),
    OP_DEF(    f2dcOp,                  "f>d" ),

    ///////////////////////////////////////////
    //  bit-vector logic
    ///////////////////////////////////////////
    NATIVE_DEF(    orBop,                   "or" ),
    NATIVE_DEF(    andBop,                  "and" ),
    NATIVE_DEF(    xorBop,                  "xor" ),
    NATIVE_DEF(    invertBop,               "invert" ),
    NATIVE_DEF(    ilshiftBop,              "ilshift" ),
    NATIVE_DEF(    irshiftBop,              "irshift" ),
    NATIVE_DEF(    arshiftBop,              "arshift" ),
    NATIVE_DEF(    irotateBop,              "irotate" ),
    NATIVE_DEF(    ireverseBop,             "ireverse" ),
    NATIVE_DEF(    countLeadingZerosBop,    "clz" ),
    NATIVE_DEF(    countTrailingZerosBop,   "ctz" ),
    NATIVE_DEF(    llshiftBop,              "llshift" ),
    NATIVE_DEF(    lrshiftBop,              "lrshift" ),
    NATIVE_DEF(    lrotateBop,              "lrotate" ),

    ///////////////////////////////////////////
    //  boolean logic
    ///////////////////////////////////////////
    NATIVE_DEF(    equals0Bop,              "not" ),
    NATIVE_DEF(    trueBop,                 "true" ),
    NATIVE_DEF(    falseBop,                "false" ),
    NATIVE_DEF(    nullBop,                 "null" ),
    NATIVE_DEF(    dnullBop,                "dnull" ),

    ///////////////////////////////////////////
    //  integer comparisons
    ///////////////////////////////////////////
    NATIVE_DEF(    equalsBop,               "=" ),
    NATIVE_DEF(    notEqualsBop,            "<>" ),
    NATIVE_DEF(    greaterThanBop,          ">" ),
    NATIVE_DEF(    greaterEqualsBop,        ">=" ),
    NATIVE_DEF(    lessThanBop,             "<" ),
	NATIVE_DEF(    lessEqualsBop,           "<=" ),
    NATIVE_DEF(    equals0Bop,              "0=" ),
    NATIVE_DEF(    notEquals0Bop,           "0<>" ),
    NATIVE_DEF(    greaterThan0Bop,         "0>" ),
    NATIVE_DEF(    greaterEquals0Bop,       "0>=" ),
    NATIVE_DEF(    lessThan0Bop,            "0<" ),
    NATIVE_DEF(    lessEquals0Bop,          "0<=" ),
    NATIVE_DEF(    unsignedGreaterThanBop,  "u>" ),
    NATIVE_DEF(    unsignedLessThanBop,     "u<" ),
    NATIVE_DEF(    withinBop,               "within" ),
    NATIVE_DEF(    minBop,                  "min" ),
    NATIVE_DEF(    maxBop,                  "max" ),
    NATIVE_DEF(    icmpBop,                 "icmp" ),
    NATIVE_DEF(    uicmpBop,                "uicmp" ),

    ///////////////////////////////////////////
    //  64-bit integer comparisons
    ///////////////////////////////////////////
#if !defined(FORTH64)
	NATIVE_DEF(lEqualsBop,				"l="),
	NATIVE_DEF(lNotEqualsBop,			"l<>"),
	NATIVE_DEF(lGreaterThanBop,			"l>"),
	NATIVE_DEF(lGreaterEqualsBop,		"l>="),
	NATIVE_DEF(lLessThanBop,			"l<"),
	NATIVE_DEF(lLessEqualsBop,			"l<="),
	NATIVE_DEF(lEquals0Bop,				"l0="),
	NATIVE_DEF(lNotEquals0Bop,			"l0<>"),
	NATIVE_DEF(lGreaterThan0Bop,		"l0>"),
	NATIVE_DEF(lGreaterEquals0Bop,		"l0>="),
	NATIVE_DEF(lLessThan0Bop,			"l0<"),
	NATIVE_DEF(lLessEquals0Bop,			"l0<="),
    OP_DEF(    lWithinOp,               "lwithin" ),
    OP_DEF(    lMinOp,                  "lmin" ),
    OP_DEF(    lMaxOp,                  "lmax" ),
#endif
    NATIVE_DEF(lcmpBop, "lcmp"),
    NATIVE_DEF(ulcmpBop, "ulcmp"),

    ///////////////////////////////////////////
    //  stack manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    rpushBop,                ">r" ),
    NATIVE_DEF(    rpopBop,                 "r>" ),
    NATIVE_DEF(    rpeekBop,                "r@" ),
    NATIVE_DEF(    rpBop,                   "rp" ),
    NATIVE_DEF(    r0Bop,                   "r0" ),
    NATIVE_DEF(    checkDupBop,             "?dup" ),
    NATIVE_DEF(    swapBop,                 "swap" ),
    NATIVE_DEF(    rotBop,                  "rot" ),
    NATIVE_DEF(    reverseRotBop,           "-rot" ),
    NATIVE_DEF(    nipBop,                  "nip" ),
    NATIVE_DEF(    nipBop,                  "l2i" ),
    NATIVE_DEF(    tuckBop,                 "tuck" ),
    NATIVE_DEF(    pickBop,                 "pick" ),
    NATIVE_DEF(    spBop,                   "sp" ),
    NATIVE_DEF(    s0Bop,                   "s0" ),
    NATIVE_DEF(    fpBop,                   "fp" ),
    NATIVE_DEF(    ipBop,                   "ip" ),
    NATIVE_DEF(    ddupBop,                 "2dup" ),
    NATIVE_DEF(    dswapBop,                "2swap" ),
    NATIVE_DEF(    ddropBop,                "2drop" ),
    NATIVE_DEF(    doverBop,                "2over" ),
    NATIVE_DEF(    drotBop,                 "2rot" ),
    NATIVE_DEF(    startTupleBop,           "r[" ),
    NATIVE_DEF(    endTupleBop,             "]r" ),
#if defined(FORTH64)
    NATIVE_DEF(    dreverseRotBop,          "-2rot" ),
    NATIVE_DEF(    dnipBop,                 "2nip" ),
    NATIVE_DEF(    dtuckBop,                "2tuck" ),
    NATIVE_DEF(    dpickBop,                "2pick" ),
    NATIVE_DEF(    drollBop,                "2roll" ),
    NATIVE_DEF(    ndropBop,                "ndrop" ),
    NATIVE_DEF(    ndupBop,                 "ndup" ),
    NATIVE_DEF(    npickBop,                "npick" ),
#endif

    ///////////////////////////////////////////
    //  memory store/fetch
    ///////////////////////////////////////////
#if defined(FORTH64)
    NATIVE_DEF(lstoreBop, "!"),
#else
    NATIVE_DEF(istoreBop, "!"),
#endif
    NATIVE_DEF(    ubfetchBop,              "ub@" ),
    NATIVE_DEF(    bstoreBop,               "b!" ),
    NATIVE_DEF(    bfetchBop,               "b@" ),
    NATIVE_DEF(    bstoreNextBop,           "b@!++" ),
    NATIVE_DEF(    bfetchNextBop,           "b@@++" ),
    NATIVE_DEF(    usfetchBop,              "us@" ),
    NATIVE_DEF(    sstoreBop,               "s!" ),
    NATIVE_DEF(    sfetchBop,               "s@" ),
    NATIVE_DEF(    sstoreNextBop,           "s@!++" ),
    NATIVE_DEF(    sfetchNextBop,           "s@@++" ),
    NATIVE_DEF(    uifetchBop,              "ui@" ),
    NATIVE_DEF(    istoreBop,               "i!" ),
    NATIVE_DEF(    ifetchBop,               "i@" ),
    NATIVE_DEF(    istoreNextBop,           "i@!++" ),
    NATIVE_DEF(    ifetchNextBop,           "i@@++" ),
    NATIVE_DEF(    lstoreBop,               "l!"),
    NATIVE_DEF(    lfetchBop,               "l@"),
    NATIVE_DEF(    lstoreNextBop,           "l@!++"),
    NATIVE_DEF(    lfetchNextBop,           "l@@++"),
    NATIVE_DEF(    plusStoreCellBop,        "+!" ),
    NATIVE_DEF(    plusStoreAtomicCellBop,  "+!@" ),
    NATIVE_DEF(    moveBop,                 "move" ),
    NATIVE_DEF(    memcmpBop,               "memcmp"),
    NATIVE_DEF(    fillBop,                 "fill" ),
    NATIVE_DEF(    getVaropBop,             "getVarop" ),
    NATIVE_DEF(    byteVarActionBop,        "byteVarAction" ),
    NATIVE_DEF(    ubyteVarActionBop,       "ubyteVarAction" ),
    NATIVE_DEF(    shortVarActionBop,       "shortVarAction" ),
    NATIVE_DEF(    ushortVarActionBop,      "ushortVarAction" ),
    NATIVE_DEF(    intVarActionBop,         "intVarAction" ),
#ifdef FORTH64
    NATIVE_DEF(     uintVarActionBop,       "uintVarAction"),
#else
    NATIVE_DEF(     intVarActionBop,        "uintVarAction"),
#endif
    NATIVE_DEF(    longVarActionBop,        "longVarAction" ),
    NATIVE_DEF(    floatVarActionBop,       "floatVarAction" ),
    NATIVE_DEF(    doubleVarActionBop,      "doubleVarAction" ),
    NATIVE_DEF(    stringVarActionBop,      "stringVarAction" ),
    NATIVE_DEF(    opVarActionBop,          "opVarAction" ),
    NATIVE_DEF(    objectVarActionBop,      "objectVarAction" ),

    ///////////////////////////////////////////
    //  single-precision fp functions
    ///////////////////////////////////////////
    NATIVE_DEF(fsinBop,                 "sfsin"),
    NATIVE_DEF(fasinBop,                "sfasin"),
    NATIVE_DEF(fcosBop,                 "sfcos"),
    NATIVE_DEF(facosBop,                "sfacos"),
    NATIVE_DEF(ftanBop,                 "sftan"),
    NATIVE_DEF(fatanBop,                "sfatan"),
    NATIVE_DEF(fatan2Bop,               "sfatan2"),
    NATIVE_DEF(fexpBop,                 "sfexp"),
    NATIVE_DEF(flnBop,                  "sfln"),
    NATIVE_DEF(flog10Bop,               "sflog"),
    NATIVE_DEF(fpowBop,                 "sf**"),
    NATIVE_DEF(fsqrtBop,                "sfsqrt"),
    NATIVE_DEF(fceilBop,                "sfceil"),
    NATIVE_DEF(ffloorBop,               "sffloor"),
    NATIVE_DEF(fabsBop,                 "sfabs"),
    NATIVE_DEF(fldexpBop,               "sfldexp"),
    NATIVE_DEF(ffrexpBop,               "sffrexp"),
    NATIVE_DEF(fmodfBop,                "sfmodf"),
    NATIVE_DEF(ffmodBop,                "sffmod"),

    ///////////////////////////////////////////
    //  single-precision fp block ops
    ///////////////////////////////////////////
    NATIVE_DEF(faddBlockBop,           "sfAddBlock"),
    NATIVE_DEF(fsubBlockBop,           "sfSubBlock"),
    NATIVE_DEF(fmulBlockBop,           "sfMulBlock"),
    NATIVE_DEF(fdivBlockBop,           "sfDivBlock"),
    NATIVE_DEF(fscaleBlockBop,         "sfScaleBlock"),
    NATIVE_DEF(foffsetBlockBop,        "sfOffsetBlock"),
    NATIVE_DEF(fmixBlockBop,           "sfMixBlock"),

    ///////////////////////////////////////////
    //  double-precision fp functions
    ///////////////////////////////////////////
    NATIVE_DEF(    dsinBop,                 "fsin" ),
    NATIVE_DEF(    dasinBop,                "fasin" ),
    NATIVE_DEF(    dcosBop,                 "fcos" ),
    NATIVE_DEF(    dacosBop,                "facos" ),
    NATIVE_DEF(    dtanBop,                 "ftan" ),
    NATIVE_DEF(    datanBop,                "fatan" ),
    NATIVE_DEF(    datan2Bop,               "fatan2" ),
    NATIVE_DEF(    dexpBop,                 "fexp" ),
    NATIVE_DEF(    dlnBop,                  "fln" ),
    NATIVE_DEF(    dlog10Bop,               "flog" ),
    NATIVE_DEF(    dpowBop,                 "f**" ),
    NATIVE_DEF(    dsqrtBop,                "fsqrt" ),
    NATIVE_DEF(    dceilBop,                "fceil" ),
    NATIVE_DEF(    dfloorBop,               "floor" ),
    NATIVE_DEF(    dabsBop,                 "fabs" ),
    NATIVE_DEF(    dldexpBop,               "fldexp" ),
    NATIVE_DEF(    dfrexpBop,               "ffrexp" ),
    NATIVE_DEF(    dmodfBop,                "fmodf" ),
    NATIVE_DEF(    dfmodBop,                "ffmod" ),
    
    ///////////////////////////////////////////
    //  single-precision fp block ops
    ///////////////////////////////////////////
    NATIVE_DEF(    daddBlockBop,           "fAddBlock"),
    NATIVE_DEF(    dsubBlockBop,           "fSubBlock"),
    NATIVE_DEF(    dmulBlockBop,           "fMulBlock"),
    NATIVE_DEF(    ddivBlockBop,           "fDivBlock"),
    NATIVE_DEF(    dscaleBlockBop,         "fScaleBlock"),
    NATIVE_DEF(    doffsetBlockBop,        "fOffsetBlock"),
    NATIVE_DEF(    dmixBlockBop,           "fMixBlock"),

    ///////////////////////////////////////////
    //  string manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    strcpyBop,               "strcpy" ),
    NATIVE_DEF(    strncpyBop,              "strncpy" ),
    NATIVE_DEF(    strlenBop,               "strlen" ),
    NATIVE_DEF(    strcatBop,               "strcat" ),
    NATIVE_DEF(    strncatBop,              "strncat" ),
    NATIVE_DEF(    strchrBop,               "strchr" ),
    NATIVE_DEF(    strrchrBop,              "strrchr" ),
    NATIVE_DEF(    strcmpBop,               "strcmp" ),
    NATIVE_DEF(    stricmpBop,              "stricmp" ),
    NATIVE_DEF(    strncmpBop,              "strncmp" ),
    NATIVE_DEF(    strstrBop,               "strstr" ),
    NATIVE_DEF(    strtokBop,               "strtok" ),
    NATIVE_DEF(    strFixupBop,             "$fixup" ),

    ///////////////////////////////////////////
    //  file manipulation
    ///////////////////////////////////////////
    NATIVE_DEF(    fopenBop,                "fopen" ),
    NATIVE_DEF(    fcloseBop,               "fclose" ),
    NATIVE_DEF(    fseekBop,                "fseek" ),
    NATIVE_DEF(    freadBop,                "fread" ),
    NATIVE_DEF(    fwriteBop,               "fwrite" ),
    NATIVE_DEF(    fgetcBop,                "fgetc" ),
    NATIVE_DEF(    fputcBop,                "fputc" ),
    NATIVE_DEF(    feofBop,                 "feof" ),
    NATIVE_DEF(    fexistsBop,              "fexists" ),
    NATIVE_DEF(    ftellBop,                "ftell" ),
    NATIVE_DEF(    flenBop,                 "flen" ),
    NATIVE_DEF(    fgetsBop,                "fgets" ),
    NATIVE_DEF(    fputsBop,                "fputs" ),
#if !defined(FORTH64)
    // ? what are these doing with file manipulation ops ?
    NATIVE_DEF(    lplusBop,                "l+" ),
    NATIVE_DEF(    lminusBop,               "l-" ),
    NATIVE_DEF(    ltimesBop,               "l*" ),
#endif
    NATIVE_DEF(    mtimesBop,               "m*" ),
    NATIVE_DEF(    umtimesBop,              "um*" ),

    OP_DEF(    stdinOp,                "stdin" ),
    OP_DEF(    stdoutOp,               "stdout" ),
    OP_DEF(    stderrOp,               "stderr" ),
    OP_DEF(    chdirOp,                "chdir" ),
    OP_DEF(    mkdirOp,                "mkdir" ),
    OP_DEF(    rmdirOp,                "rmdir" ),
    OP_DEF(    renameOp,               "rename" ),
    OP_DEF(    statOp,                 "stat" ),
    OP_DEF(    fstatOp,                "fstat" ),
    OP_DEF(    sourceIdOp,             "source-id" ),
    OP_DEF(    restoreInputOp,         "restore-input" ),
    OP_DEF(    saveInputOp,            "save-input" ),
	OP_DEF(    opendirOp,              "opendir" ),
	OP_DEF(    readdirOp,              "readdir" ),
	OP_DEF(    closedirOp,             "closedir" ),
	OP_DEF(    rewinddirOp,            "rewinddir" ),
    OP_DEF(    fflushOp,               "fflush" ),
    OP_DEF(    errnoOp,			       "errno" ),
    OP_DEF(    strerrorOp,             "strerror" ),
    OP_DEF(    findResourceOp,         "findResource"),

    OP_DEF(    getEnvironmentPairOp,   "getEnvironmentPair" ),
    OP_DEF(    getEnvironmentVarOp,    "getEnvironmentVar" ),
	OP_DEF(    getCwdOp,               "getCurrentDirectory"),

    ///////////////////////////////////////////
    //  block i/o
    ///////////////////////////////////////////
    OP_DEF(    blkOp,                  "blk" ),
    OP_DEF(    blockOp,                "block" ),
    OP_DEF(    bufferOp,               "buffer" ),
    OP_DEF(    emptyBuffersOp,         "empty-buffers" ),
    OP_DEF(    flushOp,                "flush" ),
    OP_DEF(    saveBuffersOp,          "save-buffers" ),
    OP_DEF(    updateOp,               "update" ),
    OP_DEF(    thruOp,                 "thru" ),

    ///////////////////////////////////////////
    //  64-bit integer math
    ///////////////////////////////////////////
    OP_DEF(    ldivideOp,              "l/" ),
    OP_DEF(    lmodOp,                 "lmod" ),
    OP_DEF(    ldivmodOp,              "l/mod" ),
#ifdef RASPI
    OP_DEF(    timesDivOp,             "*/" ),
    OP_DEF(    timesDivModOp,          "*/mod" ),
    OP_DEF(    umDivModOp,             "um/mod" ),
    OP_DEF(    smRemOp,                "sm/rem" ),
#endif   
    
    ///////////////////////////////////////////
    //  control flow
    ///////////////////////////////////////////
    PRECOP_DEF(doOp,                   "do" ),
    PRECOP_DEF(checkDoOp,              "?do" ),
    PRECOP_DEF(loopOp,                 "loop" ),
    PRECOP_DEF(loopNOp,                "+loop" ),
    PRECOP_DEF(ifOp,                   "if" ),
    PRECOP_DEF(elifOp,                 "]if" ),
    PRECOP_DEF(orifOp,                 "orif" ),
    PRECOP_DEF(andifOp,                "andif" ),
    PRECOP_DEF(elseOp,                 "else" ),
    PRECOP_DEF(endifOp,                "endif" ),
    PRECOP_DEF(beginOp,                "begin" ),
    PRECOP_DEF(untilOp,                "until" ),
    PRECOP_DEF(whileOp,                "while" ),
    PRECOP_DEF(repeatOp,               "repeat" ),
    PRECOP_DEF(againOp,                "again" ),
    PRECOP_DEF(caseOp,                 "case" ),
    PRECOP_DEF(ofOp,                   "of" ),
    PRECOP_DEF(ofifOp,                 "ofif" ),
    PRECOP_DEF(endofOp,                "endof" ),
    PRECOP_DEF(endcaseOp,              "endcase" ),
    PRECOP_DEF(labelOp,                "label" ),
    PRECOP_DEF(gotoOp,                 "goto" ),
    PRECOP_DEF(gotoIfOp,               "gotoIf" ),
    PRECOP_DEF(gotoIfNotOp,            "gotoIfNot" ),
    PRECOP_DEF(continueDefineOp,       "continue:" ),
    PRECOP_DEF(continueOp,             "continue" ),
    PRECOP_DEF(continueIfOp,           "continueIf" ),
    PRECOP_DEF(continueIfNotOp,        "continueIfNot" ),
    PRECOP_DEF(breakOp,                "break" ),
    PRECOP_DEF(breakIfOp,              "breakIf" ),
    PRECOP_DEF(breakIfNotOp,           "breakIfNot" ),

    ///////////////////////////////////////////
    //  op definition
    ///////////////////////////////////////////
    OP_DEF(    buildsOp,               "builds" ),
    PRECOP_DEF(doesOp,                 "does" ),
    PRECOP_DEF(exitOp,                 "exit" ),
    PRECOP_DEF(semiOp,                 ";" ),
    OP_DEF(    colonOp,                ":" ),
    OP_DEF(    colonNoNameOp,          ":noname" ),
	OP_DEF(    pushTokenOp,            "pushToken"),
	OP_DEF(    popTokenOp,             "popToken"),
	PRECOP_DEF(funcOp,                 "f:" ),
	PRECOP_DEF(endfuncOp,               ";f" ),
    OP_DEF(    codeOp,                 "code" ),
    OP_DEF(    createOp,               "create" ),
    OP_DEF(    variableOp,             "variable" ),
    OP_DEF(    iConstantOp,            "iconstant" ),
    OP_DEF(    lConstantOp,            "lconstant" ),
    PRECOP_DEF(byteOp,                 "byte" ),
    PRECOP_DEF(ubyteOp,                "ubyte" ),
    PRECOP_DEF(shortOp,                "short" ),
    PRECOP_DEF(ushortOp,               "ushort" ),
    PRECOP_DEF(intOp,                  "int" ),
    PRECOP_DEF(uintOp,                 "uint" ),
    PRECOP_DEF(longOp,                 "long" ),
    PRECOP_DEF(ulongOp,                "ulong" ),
    PRECOP_DEF(floatOp,                "sfloat" ),
    PRECOP_DEF(doubleOp,               "float" ),
    PRECOP_DEF(stringOp,               "string" ),
    PRECOP_DEF(opOp,                   "op" ),
#if defined(FORTH64)
    PRECOP_DEF(longOp,                 "cell" ),
#else
    PRECOP_DEF(intOp,                  "cell" ),
#endif
    //PRECOP_DEF(objectOp,               "object" ),
    PRECOP_DEF(voidOp,                 "void" ),
    PRECOP_DEF(arrayOfOp,              "arrayOf" ),
    PRECOP_DEF(ptrToOp,                "ptrTo" ),
    OP_DEF(    structOp,               "struct:" ),
    OP_DEF(    endStructOp,            ";struct" ),
    OP_DEF(    classOp,                "class:" ),
    OP_DEF(    endClassOp,             ";class" ),
    OP_DEF(    interfaceOp,            "interface:" ),
    OP_DEF(    endInterfaceOp,         ";interface" ),
    OP_DEF(    defineNewOp,            "new:" ),
    OP_DEF(    methodOp,               "m:" ),
    PRECOP_DEF(endmethodOp,            ";m" ),
    PRECOP_DEF(returnsOp,              "returns" ),
    OP_DEF(    doMethodOp,             "doMethod" ),
    OP_DEF(    implementsOp,           "implements:" ),
    OP_DEF(    endImplementsOp,        ";implements" ),
    PRECOP_DEF(classIdOfOp,            "classIdOf"),
    OP_DEF(    getInterfaceOp,         "getInterface"),
    OP_DEF(    unionOp,                "union" ),
    OP_DEF(    extendsOp,              "extends" ),
    OP_DEF(    strSizeOfOp,            "$sizeOf" ),
    OP_DEF(    strOffsetOfOp,          "$offsetOf" ),
    OP_DEF(    processConstantOp,      "processConstant" ),
    OP_DEF(    processLongConstantOp,  "processLongConstant" ),
    OP_DEF(    showStructOp,           "showStruct" ),
    PRECOP_DEF(newOp,                  "new" ),
    OP_DEF(    strNewOp,               "$new" ),
    PRECOP_DEF(makeObjectOp,           "makeObject" ),
    PRECOP_DEF(initMemberStringOp,     "initMemberString"),
    OP_DEF(    readObjectsOp,          "readObjects" ),
    OP_DEF(    enumOp,                 "enum:" ),
    OP_DEF(    endenumOp,              ";enum" ),
    OP_DEF(    findEnumSymbolOp,       "findEnumSymbol" ),

    PRECOP_DEF(recursiveOp,            "recursive" ),
    OP_DEF(    precedenceOp,           "precedence" ),
    OP_DEF(    strRunFileOp,           "$runFile" ),
    OP_DEF(    strLoadOp,              "$load" ),
    OP_DEF(    strLoadQOp,             "$load?" ),
    OP_DEF(    loadDoneOp,             "loaddone" ),
    OP_DEF(    requiresOp,             "requires" ),
    OP_DEF(    evaluateOp,             "evaluate" ),
    OP_DEF(    strEvaluateOp,          "$evaluate" ),
    PRECOP_DEF(stateInterpretOp,       "[" ),
    OP_DEF(    stateCompileOp,         "]" ),
    OP_DEF(    stateOp,                "state" ),
    OP_DEF(    strTickOp,              "$\'" ),
    PRECOP_DEF(postponeOp,             "postpone" ),
    PRECOP_DEF(bracketTickOp,          "[\']" ),
    OP_DEF(    bodyOp,                 ">body" ),

    ///////////////////////////////////////////
    //  vocabulary/symbol
    ///////////////////////////////////////////
    OP_DEF(    forthVocabOp,           "forth" ),
    OP_DEF(    literalsVocabOp,        "literals" ),
    OP_DEF(    definitionsOp,          "definitions" ),
    OP_DEF(    vocabularyOp,           "vocabulary" ),
    OP_DEF(    alsoOp,                 "also" ),
    OP_DEF(    previousOp,             "previous" ),
    OP_DEF(    onlyOp,                 "only" ),
    OP_DEF(    strForgetOp,            "$forget" ),
    OP_DEF(    vlistOp,                "vlist" ),
    OP_DEF(    strFindOp,              "$find" ),

    ///////////////////////////////////////////
    //  data compilation/allocation
    ///////////////////////////////////////////
    OP_DEF(    ialignOp,               "ialign" ),
    OP_DEF(    lalignOp,               "lalign" ),
    OP_DEF(    allotOp,                "allot" ),
    OP_DEF(    bCommaOp,               "b,"),
    OP_DEF(    sCommaOp,               "s,"),
    OP_DEF(    iCommaOp,               "i,"),
    OP_DEF(    lCommaOp,               "l," ),
    OP_DEF(    here0Op,                "here0" ),
    OP_DEF(    unusedOp,               "unused" ),
    OP_DEF(    mallocOp,               "malloc" ),
    OP_DEF(    reallocOp,              "realloc" ),
    OP_DEF(    freeOp,                 "free" ),

    ///////////////////////////////////////////
    //  text display
    ///////////////////////////////////////////
    OP_DEF(    printNumOp,             "." ),
    OP_DEF(    printLongNumOp,         "l." ),
    OP_DEF(    printNumDecimalOp,      "%d" ),
    OP_DEF(    printNumHexOp,          "%x" ),
    OP_DEF(    printLongDecimalOp,     "%ld" ),
    OP_DEF(    printLongHexOp,         "%lx" ),
    OP_DEF(    printStrOp,             "%s" ),
    OP_DEF(    printCharOp,            "%c" ),
    OP_DEF(    printBytesOp,           "type" ),
    OP_DEF(    print4Op,               "%4c" ),
    OP_DEF(    print8Op,               "%8c" ),
    OP_DEF(    printSpaceOp,           "%bl" ),
    OP_DEF(    printNewlineOp,         "%nl" ),
    OP_DEF(    printFloatOp,           "%sf" ),
    OP_DEF(    printDoubleOp,          "%f" ),
    OP_DEF(    printFloatGOp,          "%sg" ),
    OP_DEF(    printDoubleGOp,         "%g" ),
    OP_DEF(    format32Op,             "format" ),
    OP_DEF(    format64Op,             "lformat" ),
    OP_DEF(    representOp,            "represent" ),
    OP_DEF(    scanIntOp,              "scanInt" ),
    OP_DEF(    scanLongOp,             "scanLong" ),
    OP_DEF(    addTempStringOp,        "addTempString"),
    OP_DEF(    fprintfOp,              "fprintf" ),
    OP_DEF(    snprintfOp,             "snprintf" ),
    OP_DEF(    fscanfOp,               "fscanf" ),
    OP_DEF(    sscanfOp,               "sscanf" ),
    OP_DEF(    atoiOp,                 "atoi" ),
    OP_DEF(    atofOp,                 "atof" ),
    OP_DEF(    baseOp,                 "base" ),
    OP_DEF(    octalOp,                "octal" ),
    OP_DEF(    decimalOp,              "decimal" ),
    OP_DEF(    hexOp,                  "hex" ),
    OP_DEF(    printDecimalSignedOp,   "printDecimalSigned" ),
    OP_DEF(    printAllSignedOp,       "printAllSigned" ),
    OP_DEF(    printAllUnsignedOp,     "printAllUnsigned" ),
 
	OP_DEF(    getConsoleOutOp,        "getConsoleOut" ),
	OP_DEF(    getDefaultConsoleOutOp, "getDefaultConsoleOut" ),
	OP_DEF(    setConsoleOutOp,        "setConsoleOut" ),
	OP_DEF(    resetConsoleOutOp,      "resetConsoleOut" ),
    OP_DEF(    getErrorOutOp,          "getErrorOut"),
    OP_DEF(    setErrorOutOp,          "setErrorOut"),
        
    OP_DEF(    toupperOp,              "toupper" ),
    OP_DEF(    isupperOp,              "isupper" ),
    OP_DEF(    isspaceOp,              "isspace" ),
    OP_DEF(    isalphaOp,              "isalpha" ),
    OP_DEF(    isgraphOp,              "isgraph" ),
    OP_DEF(    tolowerOp,              "tolower" ),
    OP_DEF(    islowerOp,              "islower" ),
	
	///////////////////////////////////////////
	//  object show
	///////////////////////////////////////////
	OP_DEF(    scHandleAlreadyShownOp,  "scHandleAlreadyShown"),
    OP_DEF(    scBeginIndentOp,         "scBeginIndent" ),
    OP_DEF(    scEndIndentOp,           "scEndIndent" ),
    OP_DEF(    scShowHeaderOp,          "scShowHeader" ),
    OP_DEF(    scShowIndentOp,          "scShowIndent" ),
    OP_DEF(    scBeginFirstElementOp,   "scBeginFirstElement" ),
    OP_DEF(    scBeginNextElementOp,    "scBeginNextElement" ),
    OP_DEF(    scEndElementOp,          "scEndElement" ),
    OP_DEF(    scShowObjectOp,          "scShowObject" ),
    OP_DEF(    scSetShowIDOp,			"scSetShowID" ),
    OP_DEF(    scGetShowIDOp,			"scGetShowID" ),
    OP_DEF(    scSetShowRefCountOp,		"scSetShowRefCount" ),
    OP_DEF(    scGetShowRefCountOp,		"scGetShowRefCount" ),

    ///////////////////////////////////////////
    //  input buffer
    ///////////////////////////////////////////
    OP_DEF(    blwordOp,               "blword" ),
    OP_DEF(    strWordOp,              "$word" ),
    PRECOP_DEF(parenCommentOp,         "(" ),
    PRECOP_DEF(slashCommentOp,         "\\" ),
    OP_DEF(    sourceOp,               "source" ),
    OP_DEF(    getInOffsetPointerOp,   ">in" ),
    OP_DEF(    fillInBufferOp,         "fillInputBuffer" ),
    OP_DEF(    keyOp,                  "key" ),
    OP_DEF(    keyHitOp,               "key?" ),

    ///////////////////////////////////////////
    //  vocabulary ops
    ///////////////////////////////////////////
    OP_DEF(    vocChainHeadOp,         "vocChainHead" ),
    OP_DEF(    vocChainNextOp,         "vocChainNext" ),
#if 0
	OP_DEF(    vocNewestEntryOp,       "vocNewestEntry" ),
    OP_DEF(    vocNextEntryOp,         "vocNextEntry" ),
    OP_DEF(    vocNumEntriesOp,        "vocNumEntries" ),
    OP_DEF(    vocNameOp,              "vocName" ),
    OP_DEF(    vocFindEntryOp,         "vocFindEntry" ),
    OP_DEF(    vocFindNextEntryOp,     "vocFindNextEntry" ),
    OP_DEF(    vocFindEntryByValueOp,  "vocFindEntryByValue" ),
    OP_DEF(    vocFindNextEntryByValueOp,  "vocFindNextEntryByValue" ),
    OP_DEF(    vocAddEntryOp,          "vocAddEntry" ),
    OP_DEF(    vocRemoveEntryOp,       "vocRemoveEntry" ),
    OP_DEF(    vocValueLengthOp,       "vocValueLength" ),
#endif

    ///////////////////////////////////////////
    //  DLL support
    ///////////////////////////////////////////
    OP_DEF(    DLLVocabularyOp,        "DLLVocabulary" ),
    OP_DEF(    addDLLEntryOp,          "addDLLEntry" ),
    OP_DEF(    DLLVoidOp,              "DLLVoid" ),
    OP_DEF(    DLLLongOp,              "DLLLong" ),
    OP_DEF(    DLLStdCallOp,           "DLLStdCall" ),
    OP_DEF(    createCallbackOp,       "createCallback"),
    OP_DEF(    destroyCallbackOp,      "destroyCallback"),

    ///////////////////////////////////////////
    //  time and date
    ///////////////////////////////////////////
    OP_DEF(    timeOp,                 "time" ),
    OP_DEF(    strftimeOp,             "strftime" ),
    OP_DEF(    splitTimeOp,            "splitTime" ),
    OP_DEF(    unsplitTimeOp,          "unsplitTime" ),
    OP_DEF(    millitimeOp,            "ms@" ),
    OP_DEF(    millisleepOp,           "ms" ),

    ///////////////////////////////////////////
    //  computation utilities - randoms, hashing, sorting and searcing
    ///////////////////////////////////////////
    OP_DEF(    randOp,                 "rand" ),
    OP_DEF(    srandOp,                "srand" ),
    OP_DEF(    phHashOp,               "phHash" ),
    OP_DEF(    phHashContinueOp,       "phHashContinue" ),
    OP_DEF(    fnvHashOp,              "fnvHash" ),
    OP_DEF(    fnvHashContinueOp,      "fnvHashContinue" ),
    OP_DEF(    qsortOp,                "qsort" ),
    OP_DEF(    bsearchOp,              "bsearch" ),

    ///////////////////////////////////////////
    //  admin/debug/system
    ///////////////////////////////////////////
    OP_DEF(    dstackOp,               "dstack" ),
    OP_DEF(    drstackOp,              "drstack" ),
    OP_DEF(    removeOp,               "remove" ),
    OP_DEF(    _dupOp,                 "_dup" ),
    OP_DEF(    _dup2Op,                "_dup2" ),
    OP_DEF(    _filenoOp,              "_fileno" ),
    OP_DEF(    shellRunOp,             "_shellRun" ),
    OP_DEF(    byeOp,                  "bye" ),
    OP_DEF(    argvOp,                 "argv" ),
    OP_DEF(    argcOp,                 "argc" ),
    OP_DEF(    turboOp,                "turbo" ),
    OP_DEF(    describeOp,             "describe" ),
    OP_DEF(    describeAtOp,           "describe@" ),
    OP_DEF(    errorOp,                "error" ),
    OP_DEF(    addErrorTextOp,         "addErrorText" ),
    OP_DEF(    illegalMethodOp,        "illegalMethod" ),
    NATIVE_DEF( setTraceBop,           "setTrace"),
	OP_DEF(    getTraceOp,             "getTrace" ),
    OP_DEF(    verboseBop,             "verbose" ),
    OP_DEF(    featuresOp,             "features" ),
    OP_DEF(    dumpCrashStateOp,       "dumpCrashState" ),
	OP_DEF(		ssPushBop,				">ss"),
	OP_DEF(		ssPopBop,				"ss>"),
	OP_DEF(		ssPeekBop,				"ss@"),
	OP_DEF(		ssDepthBop,				"ssdepth"),
	OP_DEF(		bkptOp,				    "bkpt"),
    OP_DEF(     dumpProfileOp,          "dumpProfile"),
    OP_DEF(     resetProfileOp,         "resetProfile"),
    OP_DEF(     showMemoryOp,           "showMemory"),

    ///////////////////////////////////////////
    //  conditional compilation
    ///////////////////////////////////////////
    PRECOP_DEF( poundIfOp,              "#if" ),
    PRECOP_DEF( poundIfdefOp,           "#ifdef" ),
    PRECOP_DEF( poundIfndefOp,          "#ifndef" ),
    PRECOP_DEF( poundElseOp,            "#else" ),
    PRECOP_DEF( poundEndifOp,           "#endif" ),
    PRECOP_DEF( bracketIfOp,            "[if]" ),
    PRECOP_DEF( bracketIfDefOp,         "[ifdef]" ),
    PRECOP_DEF( bracketIfUndefOp,       "[ifundef]" ),
    PRECOP_DEF( bracketElseOp,          "[else]" ),
    PRECOP_DEF( bracketEndifOp,         "[endif]" ),
    PRECOP_DEF( bracketEndifOp,         "[then]" ),

    ///////////////////////////////////////////
    //  network/client/server
    ///////////////////////////////////////////
	OP_DEF(		serverOp,				"server" ),
	OP_DEF(		clientOp,				"client" ),
	OP_DEF(		shutdownOp,				"shutdown" ),

    ///////////////////////////////////////////
    //  exception handling
    ///////////////////////////////////////////
    PRECOP_DEF( tryOp,                  "try"),
    PRECOP_DEF( exceptOp,               "except"),
    PRECOP_DEF( finallyOp,              "finally"),
    PRECOP_DEF( endtryOp,               "endtry"),

    ///////////////////////////////////////////
    //  low level file IO ops
    ///////////////////////////////////////////
    OP_DEF(    openOp,                 "open" ),
    OP_DEF(    closeOp,                "close" ),
    OP_DEF(    readOp,                 "read" ),
    OP_DEF(    writeOp,                "write" ),

#ifdef WIN32
    ///////////////////////////////////////////
    //  Windows support
    ///////////////////////////////////////////
    OP_DEF( createEventOp,              "CreateEvent" ),
    OP_DEF( closeHandleOp,              "CloseHandle" ),
    OP_DEF( setEventOp,                 "SetEvent" ),
    OP_DEF( resetEventOp,               "ResetEvent" ),
    OP_DEF( pulseEventOp,               "PulseEvent" ),
    OP_DEF( getLastErrorOp,             "GetLastError" ),
    OP_DEF( waitForSingleObjectOp,      "WaitForSingleObject" ),
    OP_DEF( waitForMultipleObjectsOp,   "WaitForMultipleObjects" ),
    OP_DEF( initializeCriticalSectionOp, "InitializeCriticalSection" ),
    OP_DEF( deleteCriticalSectionOp,    "DeleteCriticalSection" ),
    OP_DEF( enterCriticalSectionOp,     "EnterCriticalSection" ),
    OP_DEF( leaveCriticalSectionOp,     "LeaveCriticalSection" ),
    OP_DEF( findFirstFileOp,            "FindFirstFile" ),
    OP_DEF( findNextFileOp,             "FindNextFile" ),
    OP_DEF( findCloseOp,                "FindClose" ),
    OP_DEF( showConsoleOp,              "showConsole" ),

    OP_DEF( windowsConstantsOp,         "windowsConstants" ),
#endif
    OP_DEF( setConsoleCursorOp,         "setConsoleCursor" ),
    OP_DEF( getConsoleCursorOp,         "getConsoleCursor" ),
    OP_DEF( getConsoleColorOp,          "getConsoleColor" ),
    OP_DEF( setConsoleColorOp,          "setConsoleColor" ),
    OP_DEF( clearConsoleOp,             "clearConsole" ),
    OP_DEF( getConsoleSizesOp,          "getConsoleSizes" ),

	NATIVE_DEF( archARMBop,				"ARCH_ARM" ),
	NATIVE_DEF( archX86Bop,				"ARCH_X86" ),
    OP_DEF( windowsOp,					"WINDOWS" ),
    OP_DEF( linuxOp,					"LINUX" ),
    OP_DEF( macosxOp,					"MACOSX" ),
    OP_DEF( forth64Op,					"FORTH64" ),

    OP_DEF( pathSeparatorOp,            "PATH_SEPARATOR"),
};

baseMethodEntry opsWhichReturnObjects[] =
{
    OP_DEF_RETURNS(thisThreadOp,        "thisThread",   kBCIThread),
    OP_DEF_RETURNS(thisFiberOp,         "thisFiber",    kBCIFiber)
};

//############################################################################
//
//  The end of all things...
//
//############################################################################

void AddForthOps( Engine* pEngine )
{
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    for (baseDictionaryCompiledEntry& compEntry : baseCompiledDictionary)
	{
		forthop* pEntry = pOuter->AddBuiltinOp(compEntry.name, compEntry.flags, compEntry.value );
		gCompiledOps[compEntry.index] = *pEntry;
	}

	for (baseDictionaryEntry& dictEntry : baseDictionary)
	{
		forthop* pEntry = pOuter->AddBuiltinOp( dictEntry.name, dictEntry.flags, dictEntry.value );
	}

    for (baseMethodEntry& objEntry : opsWhichReturnObjects)
    {
        forthop* pEntry = pOuter->AddBuiltinOp(objEntry.name, kOpCCode, objEntry.value);
        pEntry[1] = OBJECT_TYPE_TO_CODE(0, objEntry.returnType);
    }
}

};  // end extern "C"


