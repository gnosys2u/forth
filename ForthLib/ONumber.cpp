//////////////////////////////////////////////////////////////////////
//
// ONumber.cpp: builtin number related classes
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "Engine.h"
#include "OuterInterpreter.h"
#include "Vocabulary.h"
#include "Object.h"
#include "BuiltinClasses.h"
#include "ShowContext.h"

#include "TypesManager.h"
#include "ClassVocabulary.h"

#include "ONumber.h"

namespace ONumber
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oInt
	//

	struct oIntStruct
	{
        forthop*    pMethods;
        REFCOUNTER  refCount;
		int			val;
	};


	FORTHOP(oIntNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
		ALLOCATE_OBJECT(oIntStruct, pInt, pClassVocab);
        pInt->pMethods = pClassVocab->GetMethods();
		pInt->refCount = 0;
		pInt->val = 0;
		PUSH_OBJECT(pInt);
	}

    FORTHOP(oIntShowInnerMethod)
    {
        char buff[32];
        GET_THIS(oIntStruct, pInt);
        GET_SHOW_CONTEXT;
        pShowContext->BeginElement("value");
        sprintf(buff, "%d", pInt->val);
        pShowContext->EndElement(buff);
        METHOD_RETURN;
    }

    FORTHOP(oIntCompareMethod)
    {
        GET_THIS(oIntStruct, pInt);
        ForthObject compObj;
        POP_OBJECT(compObj);
        oIntStruct* pComp = (oIntStruct *)compObj;
        int retVal = 0;
        if (pInt->val != pComp->val)
        {
            retVal = (pInt->val > pComp->val) ? 1 : -1;
        }
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetMethod)
    {
        GET_THIS(oIntStruct, pInt);
        SPUSH(pInt->val);
        METHOD_RETURN;
    }

    FORTHOP(oIntSetMethod)
    {
        GET_THIS(oIntStruct, pInt);
        pInt->val = (int)SPOP;
        METHOD_RETURN;
    }

    FORTHOP(oIntGetSignedByteMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFF;
        if (val > 0x7F)
        {
            val |= 0xFFFFFF00;
        }
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetUnsignedByteMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFF;
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetSignedShortMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFFFF;
        if (val > 0x7FFF)
        {
            val |= 0xFFFF0000;
        }
        SPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntGetUnsignedShortMethod)
    {
        GET_THIS(oIntStruct, pInt);
        int val = pInt->val & 0xFFFF;
        SPUSH(val);
        METHOD_RETURN;
    }

	baseMethodEntry oIntMembers[] =
	{
		METHOD("__newOp", oIntNew),

        METHOD("showInner", oIntShowInnerMethod),
        METHOD_RET("compare", oIntCompareMethod, RETURNS_NATIVE(BaseType::kInt)),
       
        METHOD_RET("get", oIntGetMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD("set", oIntSetMethod),
        METHOD_RET("getByte", oIntGetSignedByteMethod, RETURNS_NATIVE(BaseType::kByte)),
        METHOD_RET("getUByte", oIntGetUnsignedByteMethod, RETURNS_NATIVE(BaseType::kUByte)),
        METHOD_RET("getShort", oIntGetSignedShortMethod, RETURNS_NATIVE(BaseType::kShort)),
        METHOD_RET("getUShort", oIntGetUnsignedShortMethod, RETURNS_NATIVE(BaseType::kUShort)),

		MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, BaseType::kInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLong
	//

	struct oLongStruct
	{
        forthop*    pMethods;
        REFCOUNTER  refCount;
		int64_t	val;
	};


	FORTHOP(oLongNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
		ALLOCATE_OBJECT(oLongStruct, pLong, pClassVocab);
        pLong->pMethods = pClassVocab->GetMethods();
        pLong->refCount = 0;
		pLong->val = 0;
		PUSH_OBJECT(pLong);
	}

	FORTHOP(oLongShowInnerMethod)
	{
		char buff[32];
		GET_THIS(oLongStruct, pLong);
        GET_SHOW_CONTEXT;
        pShowContext->BeginElement("value");
		sprintf(buff, "%lld", pLong->val);
		pShowContext->EndElement(buff);
		METHOD_RETURN;
	}

	FORTHOP(oLongCompareMethod)
	{
		GET_THIS(oLongStruct, pLong);
		ForthObject compObj;
		POP_OBJECT(compObj);
		oLongStruct* pComp = (oLongStruct *)compObj;
		int retVal = 0;
		if (pLong->val != pComp->val)
		{
			retVal = (pLong->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

    FORTHOP(oLongGetMethod)
    {
        GET_THIS(oLongStruct, pLong);
        stackInt64 val;
        val.s64 = pLong->val;
        LPUSH(val);
        METHOD_RETURN;
    }

    FORTHOP(oLongSetMethod)
    {
        GET_THIS(oLongStruct, pLong);
        stackInt64 a64;
        LPOP(a64);
        pLong->val = a64.s64;
        METHOD_RETURN;
    }

    baseMethodEntry oLongMembers[] =
	{
		METHOD("__newOp", oLongNew),

		METHOD("showInner", oLongShowInnerMethod),
		METHOD_RET("compare", oLongCompareMethod, RETURNS_NATIVE(BaseType::kInt)),

        METHOD_RET("get", oLongGetMethod, RETURNS_NATIVE(BaseType::kLong)),
        METHOD("set", oLongSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, BaseType::kLong)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloat
	//

	struct oFloatStruct
	{
        forthop*    pMethods;
        REFCOUNTER  refCount;
		float		val;
	};


	FORTHOP(oFloatNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
		ALLOCATE_OBJECT(oFloatStruct, pFloat, pClassVocab);
        pFloat->pMethods = pClassVocab->GetMethods();
        pFloat->refCount = 0;
		pFloat->val = 0.0f;
		PUSH_OBJECT(pFloat);
	}

    FORTHOP(oFloatShowInnerMethod)
    {
        char buff[32];
        GET_THIS(oFloatStruct, pFloat);
        Engine *pEngine = Engine::GetInstance();
        GET_SHOW_CONTEXT;
        pShowContext->BeginElement("value");
        sprintf(buff, "%f", pFloat->val);
        pShowContext->EndElement(buff);
        METHOD_RETURN;
    }

    FORTHOP(oFloatCompareMethod)
    {
        GET_THIS(oFloatStruct, pFloat);
        ForthObject compObj;
        POP_OBJECT(compObj);
        oFloatStruct* pComp = (oFloatStruct *)compObj;
        int retVal = 0;
        if (pFloat->val != pComp->val)
        {
            retVal = (pFloat->val > pComp->val) ? 1 : -1;
        }
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oFloatGetMethod)
	{
		GET_THIS(oFloatStruct, pFloat);
		FPUSH(pFloat->val);
		METHOD_RETURN;
	}

	FORTHOP(oFloatSetMethod)
	{
		GET_THIS(oFloatStruct, pFloat);
		pFloat->val = FPOP;
		METHOD_RETURN;
	}

    static float floatNaN;
    static float floatPlusInfinity;
    static float floatMinusInfinity;

    FORTHOP(oFloatNan)
    {
        FPUSH(floatNaN);
    }

    FORTHOP(oFloatPlusInfinity)
    {
        FPUSH(floatPlusInfinity);
    }

    FORTHOP(oFloatMinusInfinity)
    {
        FPUSH(floatMinusInfinity);
    }

    baseMethodEntry oFloatMembers[] =
    {
        METHOD("__newOp", oFloatNew),

        METHOD("showInner", oFloatShowInnerMethod),
        METHOD_RET("compare", oFloatCompareMethod, RETURNS_NATIVE(BaseType::kInt)),

        METHOD_RET("get", oFloatGetMethod, RETURNS_NATIVE(BaseType::kFloat)),
        METHOD("set", oFloatSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, BaseType::kFloat)),

        CLASS_OP("NaN", oFloatNan),
        CLASS_OP("+Inf", oFloatPlusInfinity),
        CLASS_OP("-Inf", oFloatMinusInfinity),

        // following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oDouble
	//

	struct oDoubleStruct
	{
        forthop*    pMethods;
        REFCOUNTER  refCount;
		double		val;
	};


	FORTHOP(oDoubleNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
		ALLOCATE_OBJECT(oDoubleStruct, pDouble, pClassVocab);
        pDouble->pMethods = pClassVocab->GetMethods();
        pDouble->refCount = 0;
		pDouble->val = 0.0;
		PUSH_OBJECT(pDouble);
	}

	FORTHOP(oDoubleShowInnerMethod)
	{
		char buff[128];
		GET_THIS(oDoubleStruct, pDouble);
		Engine *pEngine = Engine::GetInstance();
        GET_SHOW_CONTEXT;
        pShowContext->BeginElement("value");
        sprintf(buff, "%f", pDouble->val);
        pShowContext->EndElement(buff);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleCompareMethod)
	{
		GET_THIS(oDoubleStruct, pDouble);
		int retVal = 0;
		ForthObject compObj;
		POP_OBJECT(compObj);
		oDoubleStruct* pComp = (oDoubleStruct *)compObj;
		if (pDouble->val != pComp->val)
		{
			retVal = (pDouble->val > pComp->val) ? 1 : -1;
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

    FORTHOP(oDoubleGetMethod)
    {
        GET_THIS(oDoubleStruct, pDouble);
        DPUSH(pDouble->val);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleSetMethod)
    {
        GET_THIS(oDoubleStruct, pDouble);
        pDouble->val = DPOP;
        METHOD_RETURN;
    }

    static double doubleNaN;
    static double doublePlusInfinity;
    static double doubleMinusInfinity;

    FORTHOP(oDoubleNan)
    {
        DPUSH(doubleNaN);
    }

    FORTHOP(oDoublePlusInfinity)
    {
        DPUSH(doublePlusInfinity);
    }

    FORTHOP(oDoubleMinusInfinity)
    {
        DPUSH(doubleMinusInfinity);
    }


    baseMethodEntry oDoubleMembers[] =
	{
		METHOD("__newOp", oDoubleNew),

        METHOD("showInner", oDoubleShowInnerMethod),
		METHOD_RET("compare", oDoubleCompareMethod, RETURNS_NATIVE(BaseType::kInt)),

        METHOD_RET("get", oDoubleGetMethod, RETURNS_NATIVE(BaseType::kDouble)),
        METHOD("set", oDoubleSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, BaseType::kDouble)),

        CLASS_OP("NaN", oDoubleNan),
        CLASS_OP("+Inf", oDoublePlusInfinity),
        CLASS_OP("-Inf", oDoubleMinusInfinity),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(OuterInterpreter* pOuter)
	{
        float fzero = 0.0f;
        floatNaN = 0.0f / fzero;
        floatPlusInfinity = 1.0f / fzero;
        floatMinusInfinity = -1.0f / fzero;

        double doubleZero = 0.0;
        doubleNaN = 0.0 / doubleZero;
        doublePlusInfinity = 1.0 / doubleZero;
        doubleMinusInfinity = -1.0 / doubleZero;

        pOuter->AddBuiltinClass("Int", kBCIInt, kBCIObject, oIntMembers);
		pOuter->AddBuiltinClass("Long", kBCILong, kBCIObject, oLongMembers);
		pOuter->AddBuiltinClass("SFloat", kBCIFloat, kBCIObject, oFloatMembers);
		pOuter->AddBuiltinClass("Float", kBCIDouble, kBCIObject, oDoubleMembers);
    }

} // namespace ONumber
