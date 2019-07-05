//////////////////////////////////////////////////////////////////////
//
// ONumber.cpp: builtin number related classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"

#include "ONumber.h"

namespace ONumber
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 oInt
	//

	struct oIntStruct
	{
        long*       pMethods;
        ulong       refCount;
		int			val;
	};


	FORTHOP(oIntNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oIntStruct, pInt, pClassVocab);
        pInt->pMethods = pClassVocab->GetMethods();
		pInt->refCount = 0;
		pInt->val = 0;
		PUSH_OBJECT(pInt);
	}

    FORTHOP(oIntShowInnerMethod)
    {
        char buff[32];
        GET_THIS(oIntStruct, pInt);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
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
        pInt->val = SPOP;
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
        METHOD_RET("compare", oIntCompareMethod, RETURNS_NATIVE(kBaseTypeInt)),
       
        METHOD_RET("get", oIntGetMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("set", oIntSetMethod),
        METHOD_RET("getByte", oIntGetSignedByteMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD_RET("getUByte", oIntGetUnsignedByteMethod, RETURNS_NATIVE(kBaseTypeUByte)),
        METHOD_RET("getShort", oIntGetSignedShortMethod, RETURNS_NATIVE(kBaseTypeShort)),
        METHOD_RET("getUShort", oIntGetUnsignedShortMethod, RETURNS_NATIVE(kBaseTypeUShort)),

		MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oLong
	//

	struct oLongStruct
	{
        long*       pMethods;
        ulong       refCount;
		long long	val;
	};


	FORTHOP(oLongNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oLongStruct, pLong, pClassVocab);
        pLong->pMethods = pClassVocab->GetMethods();
        pLong->refCount = 0;
		pLong->val = 0;
		PUSH_OBJECT(pLong);
	}

	FORTHOP(oLongShowInnerMethod)
	{
		char buff[32];
		GET_THIS(oLongStruct, pLong);
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
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
		METHOD_RET("compare", oLongCompareMethod, RETURNS_NATIVE(kBaseTypeInt)),

        METHOD_RET("get", oLongGetMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD("set", oLongSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeLong)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oFloat
	//

	struct oFloatStruct
	{
        long*       pMethods;
        ulong       refCount;
		float		val;
	};


	FORTHOP(oFloatNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oFloatStruct, pFloat, pClassVocab);
        pFloat->pMethods = pClassVocab->GetMethods();
        pFloat->refCount = 0;
		pFloat->val = 0.0f;
		PUSH_OBJECT(pFloat);
	}

    FORTHOP(oFloatShowInnerMethod)
    {
        char buff[32];
        GET_THIS(oFloatStruct, pFloat);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
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

	baseMethodEntry oFloatMembers[] =
	{
		METHOD("__newOp", oFloatNew),

        METHOD("showInner", oFloatShowInnerMethod),
		METHOD_RET("compare", oFloatCompareMethod, RETURNS_NATIVE(kBaseTypeInt)),

        METHOD_RET("get", oFloatGetMethod, RETURNS_NATIVE(kBaseTypeFloat)),
        METHOD("set", oFloatSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeFloat)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oDouble
	//

	struct oDoubleStruct
	{
        long*       pMethods;
        ulong       refCount;
		double		val;
	};


	FORTHOP(oDoubleNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oDoubleStruct, pDouble, pClassVocab);
        pDouble->pMethods = pClassVocab->GetMethods();
        pDouble->refCount = 0;
		pDouble->val = 0.0;
		PUSH_OBJECT(pDouble);
	}

	FORTHOP(oDoubleShowInnerMethod)
	{
		char buff[128];
		GET_THIS(oDoubleStruct, pDouble);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
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

    baseMethodEntry oDoubleMembers[] =
	{
		METHOD("__newOp", oDoubleNew),

        METHOD("showInner", oDoubleShowInnerMethod),
		METHOD_RET("compare", oDoubleCompareMethod, RETURNS_NATIVE(kBaseTypeInt)),

        METHOD_RET("get", oDoubleGetMethod, RETURNS_NATIVE(kBaseTypeDouble)),
        METHOD("set", oDoubleSetMethod),

        MEMBER_VAR("value", NATIVE_TYPE_TO_CODE(0, kBaseTypeDouble)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("Int", kBCIInt, kBCIObject, oIntMembers);
		pEngine->AddBuiltinClass("Long", kBCILong, kBCIObject, oLongMembers);
		pEngine->AddBuiltinClass("Float", kBCIFloat, kBCIObject, oFloatMembers);
		pEngine->AddBuiltinClass("Double", kBCIDouble, kBCIObject, oDoubleMembers);
	}

} // namespace ONumber
