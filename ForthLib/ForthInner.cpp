//////////////////////////////////////////////////////////////////////
//
// ForthInner.cpp: implementation of the inner interpreter
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ForthEngine.h"
#include "ForthThread.h"
#include "ForthShell.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"

// for combo optypes which include an op, the op optype is native if
// we are defining (some) ops in assembler, otherwise the op optype is C code.
#ifdef ASM_INNER_INTERPRETER
#define NATIVE_OPTYPE kOpNative
#else
#define NATIVE_OPTYPE kOpCCode
#endif

extern "C"
{

// NativeAction is used to execute user ops which are defined in assembler
extern void NativeAction( ForthCoreState *pCore, forthop opVal );

void NativeActionOuter( ForthCoreState *pCore, forthop opVal )
{
	NativeAction(pCore, opVal);
}
//////////////////////////////////////////////////////////////////////
////
///
//                     byte
// 

// doByte{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doByteOp
VAR_ACTION( doByteFetch ) 
{
    signed char c = *(signed char *)(SPOP);
    SPUSH( (cell) c );
}

VAR_ACTION( doByteRef )
{
}

VAR_ACTION( doByteStore ) 
{
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) (SPOP);
}

VAR_ACTION( doBytePlusStore ) 
{
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) ((*pA) + SPOP);
}

VAR_ACTION( doByteMinusStore ) 
{
    unsigned char *pA = (unsigned char *)(SPOP);
    *pA = (unsigned char) ((*pA) - SPOP);
}

VarAction byteOps[] =
{
    doByteFetch,
    doByteFetch,
    doByteRef,
    doByteStore,
    doBytePlusStore,
    doByteMinusStore
};

void _doByteVarop( ForthCoreState* pCore, signed char* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if ( varOp <= VarOperation::kVarMinusStore )
        {
            SPUSH( (cell) pVar );
            byteOps[ (ucell)varOp ] ( pCore );
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (cell) *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each byte variable
GFORTHOP( doByteBop )
{
    // IP points to data field
    signed char* pVar = (signed char *)(GET_IP);

	_doByteVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( byteVarActionBop )
{
    signed char* pVar = (signed char *)(SPOP);
	_doByteVarop( pCore, pVar );
}
#endif

VAR_ACTION( doUByteFetch ) 
{
    unsigned char c = *(unsigned char *)(SPOP);
    SPUSH( (cell) c );
}

VarAction ubyteOps[] =
{
    doUByteFetch,
    doUByteFetch,
    doByteRef,
    doByteStore,
    doBytePlusStore,
    doByteMinusStore
};

static void _doUByteVarop( ForthCoreState* pCore, unsigned char* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            ubyteOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (cell) *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each unsigned byte variable
GFORTHOP( doUByteBop )
{
    // IP points to data field
    unsigned char* pVar = (unsigned char *)(GET_IP);

	_doUByteVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( ubyteVarActionBop )
{
    unsigned char* pVar = (unsigned char *)(SPOP);
	_doUByteVarop( pCore, pVar );
}
#endif

#define SET_OPVAL uint32_t varMode = opVal >> 21; 	if (varMode != 0) { pCore->varMode = varMode; opVal &= 0x1FFFFF; }

OPTYPE_ACTION( LocalByteAction )
{
	SET_OPVAL;
    signed char* pVar = (signed char *)(GET_FP - opVal);

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( LocalUByteAction )
{
	SET_OPVAL;
    unsigned char* pVar = (unsigned char *)(GET_FP - opVal);

	_doUByteVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldByteAction )
{
	SET_OPVAL;
    signed char* pVar = (signed char *)(SPOP + opVal);

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldUByteAction )
{
	SET_OPVAL;
    unsigned char* pVar = (unsigned char *)(SPOP + opVal);

	_doUByteVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberByteAction )
{
	SET_OPVAL;
    signed char* pVar = (signed char *)(((cell)(GET_TP)) + opVal);

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberUByteAction )
{
	SET_OPVAL;
    unsigned char* pVar = (unsigned char *)(((cell)(GET_TP)) + opVal);

	_doUByteVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each byte array
GFORTHOP( doByteArrayBop )
{
    signed char* pVar = (signed char *)(SPOP + (cell)(GET_IP));

	_doByteVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

// this is an internal op that is compiled before the data field of each unsigned byte array
GFORTHOP( doUByteArrayBop )
{
    unsigned char* pVar = (unsigned char *)(SPOP + (cell)(GET_IP));

	_doUByteVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalByteArrayAction )
{
	SET_OPVAL;
    signed char* pVar = (signed char *)(SPOP + ((cell) (GET_FP - opVal)));

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( LocalUByteArrayAction )
{
	SET_OPVAL;
    unsigned char* pVar = (unsigned char *)(SPOP + ((cell) (GET_FP - opVal)));

	_doUByteVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldByteArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of byte[0]
    signed char* pVar = (signed char *)(SPOP + opVal);
    pVar += SPOP;

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldUByteArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of byte[0]
    unsigned char* pVar = (unsigned char *)(SPOP + opVal);
    pVar += SPOP;

	_doUByteVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberByteArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    signed char* pVar = (signed char *)(((cell)(GET_TP)) + SPOP + opVal);

	_doByteVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberUByteArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    unsigned char* pVar = (unsigned char *)(((cell)(GET_TP)) + SPOP + opVal);

	_doUByteVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     short
// 

// doShort{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doShortOp
VAR_ACTION( doShortFetch )
{
    short s = *(short *)(SPOP);
    SPUSH( (cell) s );
}

VAR_ACTION( doShortRef )
{
}

VAR_ACTION( doShortStore ) 
{
    short *pA = (short *)(SPOP);
    *pA = (short) (SPOP);
}

VAR_ACTION( doShortPlusStore ) 
{
    short *pA = (short *)(SPOP);
    *pA = (short)((*pA) + SPOP);
}

VAR_ACTION( doShortMinusStore ) 
{
    short *pA = (short *)(SPOP);
    *pA = (short)((*pA) - SPOP);
}

VarAction shortOps[] =
{
    doShortFetch,
    doShortFetch,
    doShortRef,
    doShortStore,
    doShortPlusStore,
    doShortMinusStore
};

static void _doShortVarop( ForthCoreState* pCore, short* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            shortOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (cell) *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each short variable
GFORTHOP( doShortBop )
{
    // IP points to data field
    short* pVar = (short *)(GET_IP);

	_doShortVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( shortVarActionBop )
{
    short* pVar = (short *)(SPOP);
	_doShortVarop( pCore, pVar );
}
#endif

VAR_ACTION( doUShortFetch )
{
    unsigned short s = *(unsigned short *)(SPOP);
    SPUSH( (cell) s );
}

VarAction ushortOps[] =
{
    doUShortFetch,
    doUShortFetch,
    doShortRef,
    doShortStore,
    doShortPlusStore,
    doShortMinusStore
};

static void _doUShortVarop( ForthCoreState* pCore, unsigned short* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            ushortOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (cell) *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each unsigned short variable
GFORTHOP( doUShortBop )
{
    // IP points to data field
    unsigned short* pVar = (unsigned short *)(GET_IP);

	_doUShortVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( ushortVarActionBop )
{
    unsigned short* pVar = (unsigned short *)(SPOP);
	_doUShortVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalShortAction )
{
	SET_OPVAL;
    short* pVar = (short *)(GET_FP - opVal);

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( LocalUShortAction )
{
	SET_OPVAL;
    unsigned short* pVar = (unsigned short *)(GET_FP - opVal);

	_doUShortVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldShortAction )
{
	SET_OPVAL;
    short* pVar = (short *)(SPOP + opVal);

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldUShortAction )
{
	SET_OPVAL;
    unsigned short* pVar = (unsigned short *)(SPOP + opVal);

	_doUShortVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberShortAction )
{
	SET_OPVAL;
    short* pVar = (short *)(((cell)(GET_TP)) + opVal);

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberUShortAction )
{
	SET_OPVAL;
    unsigned short* pVar = (unsigned short *)(((cell)(GET_TP)) + opVal);

	_doUShortVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each short array
GFORTHOP( doShortArrayBop )
{
    // IP points to data field
    short* pVar = ((short *) (GET_IP)) + SPOP;

	_doShortVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( doUShortArrayBop )
{
    // IP points to data field
    unsigned short* pVar = ((unsigned short *) (GET_IP)) + SPOP;

	_doUShortVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalShortArrayAction )
{
	SET_OPVAL;
    short* pVar = ((short *) (GET_FP - opVal)) + SPOP;

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( LocalUShortArrayAction )
{
	SET_OPVAL;
    unsigned short* pVar = ((unsigned short *) (GET_FP - opVal)) + SPOP;

	_doUShortVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldShortArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of short[0]
    short* pVar = (short *)(SPOP + opVal);
    pVar += SPOP;

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldUShortArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of short[0]
    unsigned short* pVar = (unsigned short *)(SPOP + opVal);
    pVar += SPOP;

	_doUShortVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberShortArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    short* pVar = ((short *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doShortVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberUShortArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    unsigned short* pVar = ((unsigned short *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doUShortVarop( pCore, pVar );
}



//////////////////////////////////////////////////////////////////////
////
///
//                     int
// 

// doInt{Fetch,Ref,Store,PlusStore,MinusStore} are parts of doIntOp
VAR_ACTION( doIntFetch ) 
{
    int32_t *pA = (int32_t *) (SPOP);
    SPUSH( *pA );
}

VAR_ACTION( doIntRef )
{
}

VAR_ACTION( doIntStore ) 
{
    int32_t *pA = (int32_t *) (SPOP);
    *pA = SPOP;
}

VAR_ACTION( doIntPlusStore ) 
{
    int32_t *pA = (int32_t *) (SPOP);
    *pA += SPOP;
}

VAR_ACTION( doIntMinusStore ) 
{
    int32_t *pA = (int32_t *) (SPOP);
    *pA -= SPOP;
}

VarAction intOps[] =
{
    doIntFetch,
    doIntFetch,
    doIntRef,
    doIntStore,
    doIntPlusStore,
    doIntMinusStore
};

void _doIntVarop( ForthCoreState* pCore, int* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            intOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( (cell) *pVar );
    }
}

void intVarAction( ForthCoreState* pCore, int* pVar )
{
	_doIntVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each int variable
GFORTHOP( doIntBop )
{
    // IP points to data field
    int* pVar = (int *)(GET_IP);

	_doIntVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( intVarActionBop )
{
    int* pVar = (int *)(SPOP);
	intVarAction( pCore, pVar );
}
#endif

#ifdef FORTH64

VAR_ACTION(doUIntFetch)
{
    uint32_t s = *(uint32_t*)(SPOP);
    SPUSH((cell)s);
}

VarAction uintOps[] =
{
    doUIntFetch,
    doUIntFetch,
    doIntRef,
    doIntStore,
    doIntPlusStore,
    doIntMinusStore
};

static void _doUIntVarop(ForthCoreState* pCore, uint32_t* pVar)
{
    ForthEngine* pEngine = (ForthEngine*)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            uintOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError(kForthErrorBadVarOperation);
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH((cell)*pVar);
    }
}
#endif

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each uint32_t variable
GFORTHOP(doUIntBop)
{
    // IP points to data field

#ifdef FORTH64
    uint32_t* pVar = (uint32_t*)(GET_IP);
    _doUIntVarop(pCore, pVar);
#else
    int* pVar = (int*)(GET_IP);
    _doIntVarop(pCore, pVar);
#endif
    SET_IP((forthop*)(RPOP));
}

GFORTHOP(uintVarActionBop)
{
#ifdef FORTH64
    uint32_t* pVar = (uint32_t*)(SPOP);
    _doUIntVarop(pCore, pVar);
#else
    int* pVar = (int*)(SPOP);
    _doIntVarop(pCore, pVar);
#endif
}
#endif

OPTYPE_ACTION( LocalIntAction )
{
	SET_OPVAL;
    int* pVar = (int *)(GET_FP - opVal);

	_doIntVarop( pCore, pVar );
}


OPTYPE_ACTION( FieldIntAction )
{
	SET_OPVAL;
    int* pVar = (int *)(SPOP + opVal);

	_doIntVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberIntAction )
{
	SET_OPVAL;
    int *pVar = (int *) (((cell)(GET_TP)) + opVal);

	_doIntVarop( pCore, pVar );
}

#ifdef FORTH64

OPTYPE_ACTION(LocalUIntAction)
{
    SET_OPVAL;
    uint32_t* pVar = (uint32_t*)(GET_FP - opVal);

    _doUIntVarop(pCore, pVar);
}

OPTYPE_ACTION(FieldUIntAction)
{
    SET_OPVAL;
    uint32_t* pVar = (uint32_t*)(SPOP + opVal);

    _doUIntVarop(pCore, pVar);
}

OPTYPE_ACTION(MemberUIntAction)
{
    SET_OPVAL;
    uint32_t* pVar = (uint32_t*)(((cell)(GET_TP)) + opVal);

    _doUIntVarop(pCore, pVar);
}

#else
#define LocalUIntAction LocalIntAction
#define FieldUIntAction FieldIntAction
#define MemberUIntAction MemberIntAction
#endif

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doIntArrayBop )
{
    // IP points to data field
    int* pVar = ((int *) (GET_IP)) + SPOP;

	_doIntVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalIntArrayAction )
{
	SET_OPVAL;
    int* pVar = ((int *) (GET_FP - opVal)) + SPOP;

	_doIntVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldIntArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of int[0]
    int* pVar = (int *)(SPOP + opVal);
    pVar += SPOP;

	_doIntVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberIntArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    int* pVar = ((int *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doIntVarop( pCore, pVar );
}

#ifdef FORTH64

OPTYPE_ACTION(LocalUIntArrayAction)
{
    SET_OPVAL;
    uint32_t* pVar = ((uint32_t*)(GET_FP - opVal)) + SPOP;

    _doUIntVarop(pCore, pVar);
}

OPTYPE_ACTION(FieldUIntArrayAction)
{
    SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of int[0]
    uint32_t* pVar = (uint32_t*)(SPOP + opVal);
    pVar += SPOP;

    _doUIntVarop(pCore, pVar);
}

OPTYPE_ACTION(MemberUIntArrayAction)
{
    SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    uint32_t* pVar = ((uint32_t*)(((cell)(GET_TP)) + opVal)) + SPOP;

    _doUIntVarop(pCore, pVar);
}

#else
#define LocalUIntArrayAction LocalIntArrayAction
#define FieldUIntArrayAction FieldIntArrayAction
#define MemberUIntArrayAction MemberIntArrayAction
#endif


//////////////////////////////////////////////////////////////////////
////
///
//                     float
// 

VAR_ACTION( doFloatPlusStore ) 
{
    float *pA = (float *) (SPOP);
    *pA += FPOP;
}

VAR_ACTION( doFloatMinusStore ) 
{
    float *pA = (float *) (SPOP);
    *pA -= FPOP;
}

VarAction floatOps[] =
{
    doIntFetch,
    doIntFetch,
    doIntRef,
    doIntStore,
    doFloatPlusStore,
    doFloatMinusStore
};

static void _doFloatVarop( ForthCoreState* pCore, float* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            floatOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        SPUSH( *((int32_t *) pVar) );
    }
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doFloatBop )
{    
    // IP points to data field
    float* pVar = (float *)(GET_IP);

	_doFloatVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( floatVarActionBop )
{
    float* pVar = (float *)(SPOP);
	_doFloatVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalFloatAction )
{
	SET_OPVAL;
    float* pVar = (float *)(GET_FP - opVal);

	_doFloatVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldFloatAction )
{
	SET_OPVAL;
    float* pVar = (float *)(SPOP + opVal);

	_doFloatVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberFloatAction )
{
	SET_OPVAL;
    float *pVar = (float *) (((cell)(GET_TP)) + opVal);

	_doFloatVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doFloatArrayBop )
{
    // IP points to data field
    float* pVar = ((float *) (GET_IP)) + SPOP;

	_doFloatVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalFloatArrayAction )
{
	SET_OPVAL;
    float* pVar = ((float *) (GET_FP - opVal)) + SPOP;

	_doFloatVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldFloatArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of float[0]
    float* pVar = (float *)(SPOP + opVal);
    pVar += SPOP;

	_doFloatVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberFloatArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    float* pVar = ((float *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doFloatVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     double
// 


VAR_ACTION( doDoubleFetch ) 
{
    double *pA = (double *) (SPOP);
    DPUSH( *pA );
}

VAR_ACTION( doDoubleStore ) 
{
    double *pA = (double *) (SPOP);
    *pA = DPOP;
}

VAR_ACTION( doDoublePlusStore ) 
{
    double *pA = (double *) (SPOP);
    *pA += DPOP;
}

VAR_ACTION( doDoubleMinusStore ) 
{
    double *pA = (double *) (SPOP);
    *pA -= DPOP;
}

VarAction doubleOps[] =
{
    doDoubleFetch,
    doDoubleFetch,
    doIntRef,
    doDoubleStore,
    doDoublePlusStore,
    doDoubleMinusStore
};

static void _doDoubleVarop( ForthCoreState* pCore, double* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            doubleOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
        DPUSH( *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doDoubleBop )
{
    // IP points to data field
    double* pVar = (double *)(GET_IP);

	_doDoubleVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( doubleVarActionBop )
{
    double* pVar = (double *)(SPOP);
	_doDoubleVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalDoubleAction )
{
	SET_OPVAL;
    double* pVar = (double *)(GET_FP - opVal);

	_doDoubleVarop( pCore, pVar );
}


OPTYPE_ACTION( FieldDoubleAction )
{
	SET_OPVAL;
    double* pVar = (double *)(SPOP + opVal);

	_doDoubleVarop( pCore, pVar );
}


OPTYPE_ACTION( MemberDoubleAction )
{
	SET_OPVAL;
    double *pVar = (double *) (((cell)(GET_TP)) + opVal);

	_doDoubleVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doDoubleArrayBop )
{
    // IP points to data field
    double* pVar = ((double *) (GET_IP)) + SPOP;

	_doDoubleVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalDoubleArrayAction )
{
	SET_OPVAL;
    double* pVar = ((double *) (GET_FP - opVal)) + SPOP;

	_doDoubleVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldDoubleArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    double* pVar = (double *)(SPOP + opVal);
    pVar += SPOP;

	_doDoubleVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberDoubleArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    double* pVar = ((double *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doDoubleVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     string
// 
// a string has 2 longs at its start:
// - maximum string length
// - current string length

VAR_ACTION( doStringFetch )
{
    // TOS:  ptr to dst maxLen field
    cell a = (cell) (SPOP + 8);
    SPUSH( a );
}

VAR_ACTION( doStringStore ) 
{
    // TOS:  ptr to dst maxLen field, ptr to src first byte
    int32_t *pLen = (int32_t *) (SPOP);
    char *pSrc = (char *) (SPOP);
    int32_t srcLen = strlen( pSrc );
    int32_t maxLen = *pLen++;
    if ( srcLen > maxLen )
    {
        srcLen = maxLen;
    }
    // set current length
    *pLen++ = srcLen;
    char *pDst = (char *) pLen;
    strncpy( pDst, pSrc, srcLen );
    pDst[ srcLen ] = 0;
}

VAR_ACTION( doStringAppend ) 
{
    // TOS:  ptr to dst maxLen field, ptr to src first byte
    int32_t *pLen = (int32_t *) (SPOP);
    char *pSrc = (char *) (SPOP);
    int32_t srcLen = strlen( pSrc );
    int32_t maxLen = *pLen++;
    int32_t curLen = *pLen;
    int32_t newLen = curLen + srcLen;
    if ( newLen > maxLen )
    {
        newLen = maxLen;
        srcLen = maxLen - curLen;
    }
    // set current length
    *pLen++ = newLen;
    char *pDst = (char *) pLen;
    strncat( pDst, pSrc, srcLen );
    pDst[ newLen ] = 0;
}

VarAction stringOps[] =
{
    doStringFetch,
    doStringFetch,
    doIntRef,
    doStringStore,
    doStringAppend
};

static void _doStringVarop( ForthCoreState* pCore, char* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            stringOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - skip the maxLen/curLen fields
        SPUSH( ((cell) pVar) + 8 );
    }
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doStringBop )
{
    char* pVar = (char *) (GET_IP);

	_doStringVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( stringVarActionBop )
{
    char* pVar = (char *)(SPOP);
	_doStringVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalStringAction )
{
	SET_OPVAL;
    char* pVar = (char *) (GET_FP - opVal);

	_doStringVarop( pCore, pVar );
}


OPTYPE_ACTION( FieldStringAction )
{
	SET_OPVAL;
    char* pVar = (char *) (SPOP + opVal);

	_doStringVarop( pCore, pVar );
}


OPTYPE_ACTION( MemberStringAction )
{
	SET_OPVAL;
    char *pVar = (char *) (((cell)(GET_TP)) + opVal);

	_doStringVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doStringArrayBop )
{
    // IP points to data field
    int32_t *pLongs = (int32_t *) GET_IP;
    int index = SPOP;
    int32_t len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));

	_doStringVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalStringArrayAction )
{
	SET_OPVAL;
    cell *pLongs = GET_FP - opVal;
    int index = SPOP;
    int32_t len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));

	_doStringVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldStringArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of string[0]
    int32_t *pLongs = (int32_t *) (SPOP + opVal);
    int index = SPOP;
    int32_t len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));

	_doStringVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberStringArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of string[0]
    int32_t *pLongs = (int32_t *) ((cell)(GET_TP) + opVal);
    int index = SPOP;
    int32_t len = ((*pLongs) >> 2) + 3;      // length of one string in longwords
    char *pVar = (char *) (pLongs + (index * len));

	_doStringVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     op
// 

VAR_ACTION( doOpExecute ) 
{
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  *(int32_t *)(SPOP) );
}

VarAction opOps[] =
{
    doOpExecute,
    doIntFetch,
    doIntRef,
    doIntStore,
};

static void _doOpVarop( ForthCoreState* pCore, int32_t* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            opOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch - execute the op
        pEngine->ExecuteOp(pCore,  *pVar );
    }
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doOpBop )
{    
    // IP points to data field
    int32_t* pVar = (int32_t *)(GET_IP);
    SET_IP((forthop *)(RPOP));

	_doOpVarop( pCore, pVar );
}

GFORTHOP( opVarActionBop )
{
    int32_t* pVar = (int32_t *)(SPOP);
	_doOpVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalOpAction )
{
	SET_OPVAL;
    int32_t* pVar = (int32_t *)(GET_FP - opVal);

	_doOpVarop( pCore, pVar );
}


OPTYPE_ACTION( FieldOpAction )
{
	SET_OPVAL;
    int32_t* pVar = (int32_t *)(SPOP + opVal);

	_doOpVarop( pCore, pVar );
}


OPTYPE_ACTION( MemberOpAction )
{
	SET_OPVAL;
    int32_t *pVar = (int32_t *) (((cell)(GET_TP)) + opVal);

	_doOpVarop( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doOpArrayBop )
{
    // IP points to data field
    int32_t* pVar = ((int32_t *) (GET_IP)) + SPOP;
    SET_IP((forthop *)(RPOP));

	_doOpVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalOpArrayAction )
{
	SET_OPVAL;
    int32_t* pVar = ((int32_t *) (GET_FP - opVal)) + SPOP;

	_doOpVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldOpArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of op[0]
    int32_t* pVar = (int32_t *)(SPOP + opVal);
    pVar += SPOP;

	_doOpVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberOpArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    int32_t* pVar = ((int32_t *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doOpVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     object
// 


static void _doObjectVarop( ForthCoreState* pCore, ForthObject* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;

    VarOperation varOp = GET_VAR_OPERATION;
	CLEAR_VAR_OPERATION;
	switch ( varOp )
	{

	case VarOperation::kVarDefaultOp:
	case VarOperation::kVarFetch:
		PUSH_OBJECT( *pVar );
		break;

	case VarOperation::kVarRef:
		SPUSH( (cell) pVar );
		break;

	case VarOperation::kVarStore:
		{
            ForthObject& oldObj = *pVar;
			ForthObject newObj;
			POP_OBJECT( newObj );
			if ( OBJECTS_DIFFERENT( oldObj, newObj ) )
			{
				SAFE_KEEP( newObj );
				SAFE_RELEASE( pCore, oldObj );
				*pVar = newObj;
			}
		}
		break;

	case VarOperation::kVarPlusStore:
		{
			// store but don't increment refcount
			ForthObject& oldObj = *pVar;
			ForthObject newObj;
			POP_OBJECT(newObj);
			*pVar = newObj;
		}
		break;

	case VarOperation::kVarMinusStore:
		{
			// unref - push object on stack, clear out variable, decrement refcount but don't delete if 0
			ForthObject& oldObj = *pVar;
			PUSH_OBJECT(oldObj);
			if (oldObj != nullptr)
			{
				if (oldObj->refCount > 0)
				{
                    oldObj->refCount -= 1;
				}
				else
				{
					pEngine->SetError(kForthErrorBadReferenceCount);
				}
			}
		}
		break;

	case VarOperation::kVarObjectClear:
		{
			ForthObject& oldObj = *pVar;
			if (oldObj != nullptr)
			{
				SAFE_RELEASE(pCore, oldObj);
				oldObj = nullptr;
			}
		}
		break;

	default:
		// report GET_VAR_OPERATION out of range
		pEngine->SetError( kForthErrorBadVarOperation );
		break;
	}
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doObjectBop )
{
    // IP points to data field
	ForthObject* pVar = (ForthObject *)(GET_IP);
	SET_IP((forthop *)(RPOP));

	_doObjectVarop( pCore, pVar );
}

GFORTHOP( objectVarActionBop )
{
    ForthObject* pVar = (ForthObject *)(SPOP);
	_doObjectVarop( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalObjectAction )
{
	SET_OPVAL;
	ForthObject* pVar = (ForthObject *)(GET_FP - opVal);

	_doObjectVarop( pCore, pVar );
}


OPTYPE_ACTION( FieldObjectAction )
{
	SET_OPVAL;
	ForthObject* pVar = (ForthObject *)(SPOP + opVal);

	_doObjectVarop( pCore, pVar );
}


OPTYPE_ACTION( MemberObjectAction )
{
	SET_OPVAL;
	ForthObject* pVar = (ForthObject *)(((cell)(GET_TP)) + opVal);

	_doObjectVarop( pCore, pVar );
}


#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doObjectArrayBop )
{
    // IP points to data field
	ForthObject* pVar = ((ForthObject *) (GET_IP)) + SPOP;

	_doObjectVarop( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalObjectArrayAction )
{
	SET_OPVAL;
	ForthObject* pVar = ((ForthObject *) (GET_FP - opVal)) + SPOP;

	_doObjectVarop( pCore, pVar );
}

OPTYPE_ACTION( FieldObjectArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
	ForthObject* pVar = (ForthObject *) (SPOP + opVal);

	_doObjectVarop( pCore, pVar );
}

OPTYPE_ACTION( MemberObjectArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    ForthObject* pVar = ((ForthObject *) (((cell)(GET_TP)) + opVal)) + SPOP;

	_doObjectVarop( pCore, pVar );
}


//////////////////////////////////////////////////////////////////////
////
///
//                     long - 64 bit integer
// 


VAR_ACTION( doLongFetch ) 
{
#ifdef FORTH64
    int64_t* pA = (int64_t *)(SPOP);
    SPUSH(*pA);
#else
    stackInt64 val64;
    int64_t *pA = (int64_t *)(SPOP);
    val64.s64 = *pA;
    LPUSH(val64);
#endif
}

VAR_ACTION( doLongStore ) 
{
#ifdef FORTH64
    int64_t* pA = (int64_t *)(SPOP);
    int64_t val = SPOP;
    *pA = val;
#else
    stackInt64 val64;
    int64_t *pA = (int64_t *)(SPOP);
    LPOP(val64);
    *pA = val64.s64;
#endif
}

VAR_ACTION( doLongPlusStore ) 
{
#ifdef FORTH64
    int64_t* pA = (int64_t *)(SPOP);
    int64_t val = SPOP;
    *pA += val;
#else
    stackInt64 val64;
    int64_t *pA = (int64_t *)(SPOP);
    LPOP(val64);
    *pA += val64.s64;
#endif
}

VAR_ACTION( doLongMinusStore ) 
{
#ifdef FORTH64
    int64_t* pA = (int64_t *)(SPOP);
    int64_t val = SPOP;
    *pA -= val;
#else
    stackInt64 val64;
    int64_t *pA = (int64_t *)(SPOP);
    LPOP(val64);
    *pA -= val64.s64;
#endif
}

VarAction longOps[] =
{
    doLongFetch,
    doLongFetch,
    doIntRef,
    doLongStore,
    doLongPlusStore,
    doLongMinusStore
};

void longVarAction( ForthCoreState* pCore, int64_t* pVar )
{
    ForthEngine *pEngine = (ForthEngine *)pCore->pEngine;
    VarOperation varOp = GET_VAR_OPERATION;
    if (varOp != VarOperation::kVarDefaultOp)
    {
        if (varOp <= VarOperation::kVarMinusStore)
        {
            SPUSH((cell)pVar);
            longOps[(ucell)varOp](pCore);
        }
        else
        {
            // report GET_VAR_OPERATION out of range
            pEngine->SetError( kForthErrorBadVarOperation );
        }
        CLEAR_VAR_OPERATION;
    }
    else
    {
        // just a fetch
#ifdef FORTH64
        SPUSH(*pVar);
#else
        stackInt64 val64;
        val64.s64 = *pVar;
        LPUSH(val64);
#endif
    }
}

#ifndef ASM_INNER_INTERPRETER
GFORTHOP( doLongBop )
{
    // IP points to data field
    int64_t* pVar = (int64_t *)(GET_IP);

	longVarAction( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}

GFORTHOP( longVarActionBop )
{
    int64_t* pVar = (int64_t *)(SPOP);
	longVarAction( pCore, pVar );
}
#endif

OPTYPE_ACTION( LocalLongAction )
{
	SET_OPVAL;
    int64_t* pVar = (int64_t *)(GET_FP - opVal);

	longVarAction( pCore, pVar );
}


OPTYPE_ACTION( FieldLongAction )
{
	SET_OPVAL;
    int64_t* pVar = (int64_t *)(SPOP + opVal);

	longVarAction( pCore, pVar );
}


OPTYPE_ACTION( MemberLongAction )
{
	SET_OPVAL;
    int64_t* pVar = (int64_t *) (((cell)(GET_TP)) + opVal);

	longVarAction( pCore, pVar );
}

#ifndef ASM_INNER_INTERPRETER
// this is an internal op that is compiled before the data field of each array
GFORTHOP( doLongArrayBop )
{
    // IP points to data field
    int64_t* pVar = ((int64_t *) (GET_IP)) + SPOP;

	longVarAction( pCore, pVar );
    SET_IP( (forthop *) (RPOP) );
}
#endif

OPTYPE_ACTION( LocalLongArrayAction )
{
	SET_OPVAL;
    int64_t* pVar = ((int64_t *) (GET_FP - opVal)) + SPOP;

	longVarAction( pCore, pVar );
}

OPTYPE_ACTION( FieldLongArrayAction )
{
	SET_OPVAL;
    // TOS is struct base, NOS is index
    // opVal is byte offset of double[0]
    int64_t* pVar = (int64_t *)(SPOP + opVal);
    pVar += SPOP;

	longVarAction( pCore, pVar );
}

OPTYPE_ACTION( MemberLongArrayAction )
{
	SET_OPVAL;
    // TOS is index
    // opVal is byte offset of byte[0]
    int64_t* pVar = ((int64_t *) (((cell)(GET_TP)) + opVal)) + SPOP;

	longVarAction( pCore, pVar );
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

OPTYPE_ACTION( CCodeAction )
{
    ForthCOp opRoutine;
    // op is builtin
    if ( opVal < pCore->numOps )
    {
        opRoutine = (ForthCOp)(pCore->ops[opVal]);
        opRoutine( pCore );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( UserDefAction )
{
    // op is normal user-defined, push IP on rstack, lookup new IP
    //  in table of user-defined ops
    if ( opVal < GET_NUM_OPS )
    {
        RPUSH( (cell) GET_IP );
        SET_IP( OP_TABLE[opVal] );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION( RelativeDefAction )
{
    // op is normal user-defined, push IP on rstack,
    //  newIP is opVal + dictionary base
    forthop* newIP = pCore->pDictionary->pBase + opVal;
    if ( newIP < pCore->pDictionary->pCurrent )
    {
        RPUSH( (cell) GET_IP );
        SET_IP( newIP );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}

OPTYPE_ACTION(RelativeDataAction)
{
    // op is normal user-defined, push IP on rstack,
    //  newIP is opVal + dictionary base
    forthop* pData = pCore->pDictionary->pBase + opVal;
    if (pData < pCore->pDictionary->pCurrent)
    {
        SPUSH((cell) pData);
    }
    else
    {
        SET_ERROR(kForthErrorBadOpcode);
    }
}

OPTYPE_ACTION(BranchAction)
{
    int32_t offset = opVal;
    if ( (opVal & 0x00800000) != 0 )
    {
        // TODO: trap a hard loop (opVal == -1)?
        offset |= 0xFF000000;
    }
    SET_IP( GET_IP + offset);
}

OPTYPE_ACTION( BranchNZAction )
{
    if ( SPOP != 0 )
    {
        int32_t offset = opVal;
        if ( (opVal & 0x00800000) != 0 )
        {
            // TODO: trap a hard loop (opVal == -1)?
            offset |= 0xFF000000;
        }
        SET_IP( GET_IP + offset);
    }
}

OPTYPE_ACTION( BranchZAction )
{
    if ( SPOP == 0 )
    {
        int32_t offset = opVal;
        if ( (opVal & 0x00800000) != 0 )
        {
            // TODO: trap a hard loop (opVal == -1)?
            offset |= 0xFF000000;
        }
        SET_IP( GET_IP + offset);
    }
}

OPTYPE_ACTION(CaseBranchTAction)
{
    // TOS: this_case_value case_selector
    cell *pSP = GET_SP;
    if ( *pSP != pSP[1] )
    {
        // case didn't match
        pSP++;
    }
    else
    {
        // case matched - drop this_case_value & skip to case body
        pSP += 2;
        // case branch is always forward
        SET_IP( GET_IP + opVal );
    }
    SET_SP( pSP );
}

OPTYPE_ACTION(CaseBranchFAction)
{
    // TOS: this_case_value case_selector
    cell *pSP = GET_SP;
    if (*pSP == pSP[1])
    {
        // case matched
        pSP += 2;
    }
    else
    {
        // no match - drop this_case_value & skip to next case
        pSP++;
        // case branch is always forward
        SET_IP(GET_IP + opVal);
    }
    SET_SP(pSP);
}

OPTYPE_ACTION(PushBranchAction)
{
	SPUSH((cell)(GET_IP));
	SET_IP(GET_IP + opVal);
}

OPTYPE_ACTION(RelativeDefBranchAction)
{
	// push the opcode for the immediately following anonymous def
	int32_t opcode = (GET_IP - pCore->pDictionary->pBase) | (kOpRelativeDef << 24);
	SPUSH(opcode);
	// and branch around the anonymous def
	SET_IP(GET_IP + opVal);
}

OPTYPE_ACTION(ConstantAction)
{
    // push constant in opVal
    int32_t val = opVal;
    if ( (opVal & 0x00800000) != 0 )
    {
        val |= 0xFF000000;
    }
    SPUSH( val );
}

OPTYPE_ACTION( OffsetAction )
{
    // push constant in opVal
    int32_t offset = opVal;
    if ( (opVal & 0x00800000) != 0 )
    {
        offset |= 0xFF000000;
    }
    cell v = SPOP + offset;
    SPUSH( v );
}

OPTYPE_ACTION( OffsetFetchAction )
{
    // push constant in opVal
    int32_t offset = opVal;
    if ( (opVal & 0x00800000) != 0 )
    {
        offset |= 0xFF000000;
    }
    cell v = *(((cell *)(SPOP)) + offset);
    SPUSH( v );
}

OPTYPE_ACTION( ArrayOffsetAction )
{
    // opVal is array element size
    // TOS is array base, index
    char* pArray = (char *) (SPOP);
    pArray += ((SPOP) * opVal);
    SPUSH( (cell) pArray );
}

OPTYPE_ACTION( LocalStructArrayAction )
{
    // bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
    // init the current & max length fields of a local string
    cell* pStruct = GET_FP - (opVal >> 12);
    cell offset = ((opVal & 0xFFF) * SPOP) + ((cell) pStruct);
    SPUSH( offset );
}

OPTYPE_ACTION( ConstantStringAction )
{
    // push address of immediate string & skip over
    // opVal is number of longwords in string
    forthop *pIP = GET_IP;
    SPUSH( (cell) pIP );
    SET_IP( pIP + opVal );
}

OPTYPE_ACTION( AllocLocalsAction )
{
    // allocate a local var stack frame
    RPUSH( (cell) GET_FP );      // rpush old FP
    SET_FP( GET_RP );                // set FP = RP, points at oldFP
    SET_RP( GET_RP - opVal );                // allocate storage for local vars
	memset( GET_RP, 0, (opVal << CELL_SHIFT) );
}

OPTYPE_ACTION( InitLocalStringAction )
{
    // bits 0..11 are string length in bytes, bits 12..23 are frame offset in cells
    // init the current & max length fields of a local string
    cell* pFP = GET_FP;
    cell* pStr = pFP - (opVal >> 12);
    *pStr++ = (opVal & 0xFFF);          // max length
    *pStr++ = 0;                        // current length
    *((char *) pStr) = 0;               // terminating null
}

OPTYPE_ACTION( LocalRefAction )
{
    // opVal is offset in longs
    SPUSH( (cell)(GET_FP - opVal) );
}

OPTYPE_ACTION( MemberRefAction )
{
    // opVal is offset in bytes
    SPUSH( ((cell)GET_TP) + opVal );
}

// bits 0..15 are index into ForthCoreState userOps table, 16..18 are flags, 19..23 are arg count
OPTYPE_ACTION( DLLEntryPointAction )
{
#ifdef WIN32
    uint32_t entryIndex = CODE_TO_DLL_ENTRY_INDEX( opVal );
    uint32_t argCount = CODE_TO_DLL_ENTRY_NUM_ARGS( opVal );
	uint32_t flags = CODE_TO_DLL_ENTRY_FLAGS( opVal );
    if ( entryIndex < GET_NUM_OPS )
    {
        CallDLLRoutine( (DLLRoutine)(OP_TABLE[entryIndex]), argCount, flags, pCore );
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
#endif
}

void SpewMethodName(ForthObject obj, forthop opVal)
{
    if (obj == nullptr)
    {
        SPEW_INNER_INTERPRETER(" NULL_OBJECT:method_%d  ", opVal);
        return;
    }

    char buffer[256];
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
	if (pClassObject != nullptr)
	{
        ForthClassVocabulary* pVocab = pClassObject->pVocab;
        const char* pVocabName = pVocab->GetName();
        strcpy(buffer, "UNKNOWN_METHOD");
		while (pVocab != nullptr)
		{
            forthop *pEntry = pVocab->FindNextSymbolByValue(opVal, nullptr);
			while (true)
			{
				if (pEntry != nullptr)
				{
					int32_t typeCode = pEntry[1];
					bool isMethod = CODE_IS_METHOD(typeCode);
					if (isMethod)
					{
						int len = pVocab->GetEntryNameLength(pEntry);
						char* pBuffer = &(buffer[0]);
						const char* pName = pVocab->GetEntryName(pEntry);
						for (int i = 0; i < len; i++)
						{
							*pBuffer++ = *pName++;
						}
						*pBuffer = '\0';
						pVocab = nullptr;
						break;
					}
					else
					{
						pEntry = pVocab->NextEntrySafe(pEntry);
                        if (pEntry != nullptr)
                        {
                            pEntry = pVocab->FindNextSymbolByValue(opVal, pEntry);
                        }
					}
				}
				else
				{
					pVocab = pVocab->ParentClass();
					break;
				}
			}  // end while true
		}  // end while pVocab not null
		SPEW_INNER_INTERPRETER(" %s:%s  ", pVocabName, buffer);
	}
}

OPTYPE_ACTION(MethodWithThisAction)
{
    // this is called when an object method invokes another method on itself
    // opVal is the method number
    ForthEngine *pEngine = GET_ENGINE;
    ForthObject thisObject = (ForthObject)(GET_TP);
    forthop* pMethods = thisObject->pMethods;
    RPUSH( ((cell) GET_TP) );
	if (pEngine->GetTraceFlags() & kLogInnerInterpreter)
	{
		SpewMethodName(thisObject, opVal);
	}
	pEngine->ExecuteOp(pCore, pMethods[opVal]);
}

OPTYPE_ACTION(MethodWithSuperAction)
{
    // this is called when an object method invokes a method off its superclass
    // opVal is the method number
    if (opVal == kMethodDelete)
    {
        // super.delete is illegal - system does this for you
        SET_ERROR(kForthErrorIllegalMethod);
        return;
    }

    ForthEngine *pEngine = GET_ENGINE;
    ForthObject thisObject = (ForthObject)(GET_TP);
    forthop* pMethods = thisObject->pMethods;
    // save old methods on rstack to be restored by unsuper, which is compiled in next opcode
    //  after the methodWithSuper opcode
    RPUSH((cell)pMethods);
    RPUSH(((cell)GET_TP));
    if (pEngine->GetTraceFlags() & kLogInnerInterpreter)
    {
        SpewMethodName(thisObject, opVal);
    }
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(thisObject);
    forthop* pSuperMethods = pClassObject->pVocab->ParentClass()->GetMethods();
    thisObject->pMethods = pSuperMethods;
    pEngine->ExecuteOp(pCore, pSuperMethods[opVal]);
}

OPTYPE_ACTION( MethodWithTOSAction )
{
    // TOS is object (top is vtable, next is data)
    // this is called when a method is invoked from inside another
    // method in the same class - the difference is that in this case
    // there is no explicit source for the "this" pointer, we just keep
    // on using the current "this" pointer
    ForthEngine *pEngine = GET_ENGINE;
	//pEngine->TraceOut(">>MethodWithTOSAction IP %p  RP %p\n", GET_IP, GET_RP);
    RPUSH( ((cell) GET_TP) );

    ForthObject obj;
    POP_OBJECT(obj);
    if (obj == nullptr || obj->pMethods == nullptr)
    {
        SET_ERROR(kForthErrorBadObject);
        return;
    }

    SET_TP(obj);
	if (pEngine->GetTraceFlags() & kLogInnerInterpreter)
	{
        SpewMethodName(obj, opVal);
	}
    pEngine->ExecuteOp(pCore,  obj->pMethods[ opVal ] );
	//pEngine->TraceOut("<<MethodWithTOSAction IP %p  RP %p\n", GET_IP, GET_RP);
}

OPTYPE_ACTION( MemberStringInitAction )
{
    // bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
    // init the current & max length fields of a local string
    ForthObject pThis = GET_TP;
    int32_t* pStr = ((int32_t *)pThis) + (opVal >> 12);
    *pStr++ = (opVal & 0xFFF);          // max length
    *pStr++ = 0;                        // current length
    *((char *) pStr) = 0;               // terminating null
}

OPTYPE_ACTION( NumVaropOpComboAction )
{
	// NUM VAROP OP combo - bits 0:10 are signed integer, bits 11:12 are varop-2, bits 13-23 are opcode

	// push signed int in bits 0:10
	int32_t num = opVal;
    if ( (opVal & 0x400) != 0 )
    {
      num |= 0xFFFFF800;
    }
	else
	{
		num &= 0x3FF;
	}
    SPUSH( num );

	// set varop to bits 11:12 + 2
	SET_VAR_OPERATION( ((opVal >> 11) & 3) + 2 );

	// execute op in bits 13:23
	forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal >> 13));
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
}

OPTYPE_ACTION( NumVaropComboAction )
{
	// NUM VAROP combo - bits 0:21 are signed integer, bits 22:23 are varop-2

	// push signed int in bits 0:21
	int32_t num = opVal;
    if ( (opVal & 0x200000) != 0 )
    {
      num |= 0xFFE00000;
    }
	else
	{
		num &= 0x1FFFFF;
	}
    SPUSH( num );

	// set varop to bits 22:23 + 2
    SET_VAR_OPERATION(static_cast<VarOperation>(((opVal >> 22) & 3) + 2));
}

OPTYPE_ACTION( NumOpComboAction )
{
	// NUM OP combo - bits 0:12 are signed integer, bit 13 is builtin/userdef, bits 14:23 are opcode

	// push signed int in bits 0:12
	int32_t num = opVal;
    if ( (opVal & 0x1000) != 0 )
    {
      num |= 0xFFFFE000;
    }
	else
	{
		num &= 0xFFF;
	}
    SPUSH( num );

	// execute op in bits 13:23
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal >> 13) );
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
}

OPTYPE_ACTION( VaropOpComboAction )
{
	// VAROP OP combo - bits 0:1 are varop-2, bits 2:23 are opcode

	// set varop to bits 0:1 + 2
	SET_VAR_OPERATION( (opVal & 3) + 2 );

	// execute op in bits 2:23
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal >> 2) );
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
}

OPTYPE_ACTION( OpZBranchComboAction )
{
	// bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal & 0xFFF));
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
    if ( SPOP == 0 )
    {
		int32_t branchOffset = opVal >> 12;
        if ( (branchOffset & 0x800) != 0 )
        {
            // TODO: trap a hard loop (opVal == -1)?
            branchOffset |= 0xFFFFF000;
        }
        SET_IP( GET_IP + branchOffset );
    }
}

OPTYPE_ACTION(OpNZBranchComboAction)
{
    // bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal & 0xFFF));
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore, op);
    if (SPOP != 0)
    {
        int32_t branchOffset = opVal >> 12;
        if ((branchOffset & 0x800) != 0)
        {
            // TODO: trap a hard loop (opVal == -1)?
            branchOffset |= 0xFFFFF000;
        }
        SET_IP(GET_IP + branchOffset);
    }
}

OPTYPE_ACTION( SquishedFloatAction )
{
	float fval = ((ForthEngine *)pCore->pEngine)->UnsquishFloat( opVal );
	FPUSH( fval );
}

OPTYPE_ACTION( SquishedDoubleAction )
{
	double dval = ((ForthEngine *)pCore->pEngine)->UnsquishDouble( opVal );
	DPUSH( dval );
}

OPTYPE_ACTION( SquishedLongAction )
{
#if defined(FORTH64)
    int64_t lval = ((ForthEngine *)pCore->pEngine)->UnsquishLong(opVal);
    SPUSH(lval);
#else
    stackInt64 lval;
    lval.s64 = ((ForthEngine *)pCore->pEngine)->UnsquishLong(opVal);
    LPUSH(lval);
#endif
}

OPTYPE_ACTION( LocalRefOpComboAction )
{
	// REF_OFFSET OP combo - bits 0:11 are local var offset in longs, bits 12:23 are opcode
    SPUSH( (cell)(GET_FP - (opVal & 0xFFF)) );

	// execute op in bits 12:23
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal >> 12));
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
}

OPTYPE_ACTION( MemberRefOpComboAction )
{
	// REF_OFFSET OP combo - bits 0:11 are member offset in bytes, bits 12:23 are opcode
    // opVal is offset in bytes
    SPUSH( ((cell)GET_TP) + (opVal & 0xFFF) );

	// execute op in bits 12:23
    forthop op = COMPILED_OP(NATIVE_OPTYPE, (opVal >> 12));
    ((ForthEngine *)pCore->pEngine)->ExecuteOp(pCore,  op );
}

OPTYPE_ACTION( IllegalOptypeAction )
{
    SET_ERROR( kForthErrorBadOpcodeType );
}

OPTYPE_ACTION( ReservedOptypeAction )
{
    SET_ERROR( kForthErrorBadOpcodeType );
}

#if 0
OPTYPE_ACTION( MethodAction )
{
    // token is object method invocation
    forthop op = GET_CURRENT_OP;
    forthOpType opType = FORTH_OP_TYPE( op );
    int methodNum = ((int) opType) & 0x7F;
    int32_t* pObj = NULL;
    if ( opVal & 0x00800000 )
    {
        // object ptr is in local variable
        pObj = GET_FP - (opVal & 0x007F0000);
    }
    else
    {
        // object ptr is in global op
        if ( opVal < pCore->numUserOps )
        {
            pObj = (int32_t *) (pCore->userOps[opVal]);
        }
        else
        {
            SET_ERROR( kForthErrorBadOpcode );
        }
    }
    if ( pObj != NULL )
    {
        // pObj is a pair of pointers, first pointer is to
        //   class descriptor for this type of object,
        //   second pointer is to storage for object (this ptr)
        int32_t *pClass = (int32_t *) (*pObj);
        if ( (pClass[1] == CLASS_MAGIC_NUMBER)
            && (pClass[2] > methodNum) )
        {
            RPUSH( (cell) GET_IP );
            RPUSH( (cell) GET_TP );
            SET_TP( pObj );
            SET_IP( (forthop *) (pClass[methodNum + 3]) );
        }
        else
        {
            // bad class magic number, or bad method number
            SET_ERROR( kForthErrorBadOpcode );
        }
    }
    else
    {
        SET_ERROR( kForthErrorBadOpcode );
    }
}
#endif

// NOTE: there is no opcode assigned to this op
FORTHOP( BadOpcodeOp )
{
    SET_ERROR( kForthErrorBadOpcode );
}

optypeActionRoutine builtinOptypeAction[] =
{
    // 00 - 09
    NativeActionOuter,
    NativeActionOuter,           // immediate
    UserDefAction,
    UserDefAction,          // immediate
    CCodeAction,
    CCodeAction,            // immediate
    RelativeDefAction,
    RelativeDefAction,      // immediate
    DLLEntryPointAction,
    ReservedOptypeAction,

    // 10 - 19
    BranchAction,				// 0x0A
    BranchNZAction,
    BranchZAction,
    CaseBranchTAction,
    CaseBranchFAction,
    PushBranchAction,
    RelativeDefBranchAction,    // 0x10
    RelativeDataAction,		
    RelativeDataAction,
    ReservedOptypeAction,

    // 20 - 29
    ConstantAction,				// 0x14
    ConstantStringAction,
    OffsetAction,
    ArrayOffsetAction,
    AllocLocalsAction,			// 0x18
    LocalRefAction,
    InitLocalStringAction,
    LocalStructArrayAction,
    OffsetFetchAction,			// 0x1C
    MemberRefAction,

    // 30 -39
    LocalByteAction,
    LocalUByteAction,
    LocalShortAction,			// 0x20
    LocalUShortAction,
    LocalIntAction,
    LocalUIntAction,
    LocalLongAction,			// 0x24
    LocalLongAction,
    LocalFloatAction,
    LocalDoubleAction,

    // 40 - 49
    LocalStringAction,			// 0x28
    LocalOpAction,
    LocalObjectAction,
    LocalByteArrayAction,
    LocalUByteArrayAction,		// 0x2C
    LocalShortArrayAction,
    LocalUShortArrayAction,
    LocalIntArrayAction,
    LocalUIntArrayAction,		// 0x30
    LocalLongArrayAction,

    // 50 - 59
    LocalLongArrayAction,
    LocalFloatArrayAction,
    LocalDoubleArrayAction,		// 0x34
    LocalStringArrayAction,
    LocalOpArrayAction,
    LocalObjectArrayAction,
    FieldByteAction,			// 0x38
    FieldUByteAction,
    FieldShortAction,
    FieldUShortAction,

    // 60 - 69
    FieldIntAction,				// 0x3C
    FieldUIntAction,
    FieldLongAction,
    FieldLongAction,
    FieldFloatAction,			// 0x40
    FieldDoubleAction,
    FieldStringAction,
    FieldOpAction,
    FieldObjectAction,			// 0x44
    FieldByteArrayAction,

    // 70 - 79
    FieldUByteArrayAction,
    FieldShortArrayAction,
    FieldUShortArrayAction,		// 0x48
    FieldIntArrayAction,
    FieldUIntArrayAction,
    FieldLongArrayAction,
    FieldLongArrayAction,		// 0x4C
    FieldFloatArrayAction,
    FieldDoubleArrayAction,
    FieldStringArrayAction,

    // 80 - 89
    FieldOpArrayAction,			// 0x50
    FieldObjectArrayAction,
    MemberByteAction,
    MemberUByteAction,
    MemberShortAction,			// 0x54
    MemberUShortAction,
    MemberIntAction,
    MemberUIntAction,
    MemberLongAction,			// 0x58
    MemberLongAction,

    // 90 - 99
    MemberFloatAction,
    MemberDoubleAction,
    MemberStringAction,			// 0x5C
    MemberOpAction,
    MemberObjectAction,
    MemberByteArrayAction,
    MemberUByteArrayAction,		// 0x60
    MemberShortArrayAction,
    MemberUShortArrayAction,
    MemberIntArrayAction,

	// 100 - 109	64 - 6D
    MemberUIntArrayAction,		// 0x64
    MemberLongArrayAction,
    MemberLongArrayAction,
    MemberFloatArrayAction,
    MemberDoubleArrayAction,	// 0x68
    MemberStringArrayAction,
    MemberOpArrayAction,
    MemberObjectArrayAction,
    MethodWithThisAction,		// 0x6C
    MethodWithTOSAction,

	// 110 - 119	6E - 77
    MemberStringInitAction,
	NumVaropOpComboAction,
	NumVaropComboAction,		// 0x70
	NumOpComboAction,
	VaropOpComboAction,
	OpZBranchComboAction,
	OpNZBranchComboAction,		// 0x74
	SquishedFloatAction,
	SquishedDoubleAction,
	SquishedLongAction,

	// 120 - 122
	LocalRefOpComboAction,		// 0x78
	MemberRefOpComboAction,
    MethodWithSuperAction,

    NULL            // this must be last to end the list
};

void InitDispatchTables( ForthCoreState* pCore )
{
    int i;

    for ( i = 0; i < 256; i++ )
    {
        pCore->optypeAction[i] = IllegalOptypeAction;
    }

    for ( i = 0; i < MAX_BUILTIN_OPS; i++ )
    {
        pCore->ops[i] = (forthop *)(BadOpcodeOp);
    }
    for ( i = 0; builtinOptypeAction[i] != NULL; i++ )
    {
        pCore->optypeAction[i] = builtinOptypeAction[i];
    }
}

void CoreSetError( ForthCoreState *pCore, eForthError error, bool isFatal )
{
    pCore->error =  error;
    pCore->state = (ucell)((isFatal) ? OpResult::kResultFatalError : OpResult::kResultError);
}

//############################################################################
//
//          I N N E R    I N T E R P R E T E R
//
//############################################################################




OpResult InnerInterpreter( ForthCoreState *pCore )
{
    SET_STATE( OpResult::kResultOk );

	bool bContinueLooping = true;
	while (bContinueLooping)
	{
#ifdef TRACE_INNER_INTERPRETER
		ForthEngine* pEngine = GET_ENGINE;
		int traceFlags = pEngine->GetTraceFlags();
		if (traceFlags & kLogInnerInterpreter)
		{
			OpResult result = GET_STATE;
			while (result == OpResult::kResultOk)
			{
				// fetch op at IP, advance IP
                forthop* pIP = GET_IP;
                forthop op = *pIP;
                pEngine->TraceOp(pIP, op);
                pIP++;
				SET_IP(pIP);
				DISPATCH_FORTH_OP(pCore, op);
				result = GET_STATE;
				if (result != OpResult::kResultDone)
				{
					if (traceFlags & kLogStack)
					{
						pEngine->TraceStack(pCore);
					}
					pEngine->TraceOut("\n");
				}
			}
			break;
		}
		else
#endif
		{
			while (GET_STATE == OpResult::kResultOk)
			{
                forthop* pIP = GET_IP;
				forthop op = *pIP++;
				SET_IP(pIP);
				//DISPATCH_FORTH_OP( pCore, op );
				//#define DISPATCH_FORTH_OP( _pCore, _op ) 	_pCore->optypeAction[ (int) FORTH_OP_TYPE( _op ) ]( _pCore, FORTH_OP_VALUE( _op ) )
				optypeActionRoutine typeAction = pCore->optypeAction[(int)FORTH_OP_TYPE(op)];
				typeAction(pCore, FORTH_OP_VALUE(op));
			}
			break;
		}
	}
    return GET_STATE;
}

OpResult InterpretOneOp( ForthCoreState *pCore, forthop op )
{
    SET_STATE( OpResult::kResultOk );

#ifdef TRACE_INNER_INTERPRETER
	ForthEngine* pEngine = GET_ENGINE;
	int traceFlags = pEngine->GetTraceFlags();
	if ( traceFlags & kLogInnerInterpreter )
	{
		// fetch op at IP, advance IP
        forthop* pIP = GET_IP;
		pEngine->TraceOp(pIP, *pIP);
		//DISPATCH_FORTH_OP( pCore, op );
		optypeActionRoutine typeAction = pCore->optypeAction[(int)FORTH_OP_TYPE(op)];
		typeAction(pCore, FORTH_OP_VALUE(op));
		if (traceFlags & kLogStack)
		{
			pEngine->TraceStack( pCore );
		}
		pEngine->TraceOut( "\n" );
	}
	else
#endif
	{
		//DISPATCH_FORTH_OP( pCore, op );
		optypeActionRoutine typeAction = pCore->optypeAction[(int)FORTH_OP_TYPE(op)];
		typeAction(pCore, FORTH_OP_VALUE(op));
	}
    return GET_STATE;
}

#if 0
OpResult InnerInterpreter( ForthCoreState *pCore )
{
    forthop opVal;
    ucell numBuiltinOps;
    forthOpType opType;
    forthop *pIP;
    forthop op;
    numBuiltinOps = pCore->numBuiltinOps;

    SET_STATE( OpResult::kResultOk );

#ifdef TRACE_INNER_INTERPRETER
	ForthEngine* pEngine = GET_ENGINE;
	int traceFlags = pEngine->GetTraceFlags();
	if ( traceFlags & kLogInnerInterpreter )
	{
		while ( GET_STATE == OpResult::kResultOk )
		{
			// fetch op at IP, advance IP
			pIP = GET_IP;
			pEngine->TraceOp( pIP );
			op = *pIP++;
			SET_IP( pIP );
			opType = FORTH_OP_TYPE( op );
			opVal = FORTH_OP_VALUE( op );
			pCore->optypeAction[ (int) opType ]( pCore, opVal );
			if ( traceFlags & kLogStack )
			{
				pEngine->TraceStack( pCore );
			}
			pEngine->TraceOut( "\n" );
		}
	}
	else
#endif
	{
		while ( GET_STATE == OpResult::kResultOk )
		{
			// fetch op at IP, advance IP
			pIP = GET_IP;
			op = *pIP++;
			SET_IP( pIP );
			opType = FORTH_OP_TYPE( op );
			opVal = FORTH_OP_VALUE( op );
			pCore->optypeAction[ (int) opType ]( pCore, opVal );
		}
	}
    return GET_STATE;
}
#endif

};      // end extern "C"
