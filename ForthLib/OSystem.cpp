//////////////////////////////////////////////////////////////////////
//
// OSystem.cpp: builtin system class
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
#include "ForthThread.h"
#include "ForthPortability.h"

#include "OSystem.h"

namespace OSystem
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 OSystem
	//

    static oSystemStruct gSystemSingleton;

    FORTHOP(oSystemNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject obj;
        gSystemSingleton.refCount = 2000000000;
        CLEAR_OBJECT(gSystemSingleton.namedObjects);
        CLEAR_OBJECT(gSystemSingleton.args);
        CLEAR_OBJECT(gSystemSingleton.env);
        obj = reinterpret_cast<ForthObject>(&gSystemSingleton);
        obj->pMethods = pClassVocab->GetMethods();
        PUSH_OBJECT(obj);
	}

	FORTHOP(oSystemDeleteMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
        pSystem->refCount = 2000000000;
        // TODO: warn that something tried to delete system
		METHOD_RETURN;
	}

	FORTHOP(oSystemStatsMethod)
	{
		char buff[512];

		SNPRINTF(buff, sizeof(buff), "pCore %p pEngine %p     DP %p DBase %p    IP %p\n",
			pCore, pCore->pEngine, pCore->pDictionary, pCore->pDictionary->pBase, pCore->IP);
		CONSOLE_STRING_OUT(buff);

        SNPRINTF(buff, sizeof(buff), "SP %p ST %p SLen %d    RP %p RT %p RLen %d\n",
			pCore->SP, pCore->ST, pCore->SLen,
			pCore->RP, pCore->RT, pCore->RLen);
		CONSOLE_STRING_OUT(buff);

		SNPRINTF(buff, sizeof(buff), "%d builtins    %d userops @ %p\n", pCore->numBuiltinOps, pCore->numOps, pCore->ops);
		CONSOLE_STRING_OUT(buff);

        ForthEngine *pEngine = GET_ENGINE;
        pEngine->ShowSearchInfo();

		METHOD_RETURN;
	}


	FORTHOP(oSystemGetDefinitionsVocabMethod)
	{
		GET_THIS(oSystemStruct, pSystem);

		ForthVocabulary* pVocab = GET_ENGINE->GetDefinitionVocabulary();
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
			PUSH_OBJECT(nullptr);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemSetDefinitionsVocabMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
            GET_ENGINE->SetDefinitionVocabulary(pVocab);
		}
		METHOD_RETURN;
	}

    FORTHOP(oSystemClearSearchVocabMethod)
    {
        GET_THIS(oSystemStruct, pSystem);
        ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
        pVocabStack->Clear();

        METHOD_RETURN;
    }

    FORTHOP(oSystemGetSearchVocabDepthMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
		SPUSH(pVocabStack->GetDepth());

		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabAtMethod)
	{
		GET_THIS(oSystemStruct, pSystem);
		int vocabStackIndex = SPOP;

		ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
		ForthVocabulary* pVocab = pVocabStack->GetElement(vocabStackIndex);
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
            PUSH_OBJECT(nullptr);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabTopMethod)
	{
		GET_THIS(oSystemStruct, pSystem);

		ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
		ForthVocabulary* pVocab = pVocabStack->GetTop();
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
        else
        {
            PUSH_OBJECT(nullptr);
        }

		METHOD_RETURN;
	}

	FORTHOP(oSystemSetSearchVocabTopMethod)
	{
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
			ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemPushSearchVocabMethod)
	{
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
			ForthVocabularyStack* pVocabStack = GET_ENGINE->GetVocabularyStack();
			pVocabStack->DupTop();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetVocabByNameMethod)
	{
		GET_THIS(oSystemStruct, pSystem);

		const char* pVocabName = reinterpret_cast<const char*>(SPOP);
		ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary(pVocabName);
		if (pVocab != NULL)
		{
			PUSH_OBJECT(pVocab->GetVocabularyObject());
		}
		else
		{
            PUSH_OBJECT(nullptr);
        }
		METHOD_RETURN;
	}
	
    FORTHOP(oSystemGetVocabChainHeadMethod)
    {
        GET_THIS(oSystemStruct, pSystem);

        const char* pVocabName = reinterpret_cast<const char*>(SPOP);
        ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
        PUSH_OBJECT(pVocab->GetVocabularyObject());
        METHOD_RETURN;
    }

    
	FORTHOP(oSystemGetOpsTableMethod)
	{
		SPUSH((long)(pCore->ops));
		METHOD_RETURN;
	}


    FORTHOP(oSystemGetClassByIndexMethod)
    {
        int typeIndex = SPOP;
        ForthObject classObject = nullptr;
        ForthClassVocabulary* pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(typeIndex);
        if (pClassVocab != nullptr)
        {
            classObject = (ForthObject) pClassVocab->GetClassObject();
        }

        PUSH_OBJECT(classObject);
        METHOD_RETURN;
    }


    FORTHOP(oSystemGetNumClassesMethod)
    {
        int numClasses = ForthTypesManager::GetInstance()->GetNewestClass()->GetTypeIndex() + 1;
        SPUSH(numClasses);
        METHOD_RETURN;
    }

	FORTHOP(oSystemCreateAsyncThreadMethod)
	{
		ForthObject asyncThread;
		int returnStackLongs = (int)(SPOP);
		int paramStackLongs = (int)(SPOP);
		long threadOp = SPOP;
		OThread::CreateAsyncThreadObject(asyncThread, GET_ENGINE, threadOp, paramStackLongs, returnStackLongs);

		PUSH_OBJECT(asyncThread);
		METHOD_RETURN;
	}

	FORTHOP(oSystemCreateAsyncLockMethod)
	{
		ForthObject asyncLock;
		OLock::CreateAsyncLockObject(asyncLock, GET_ENGINE);

		PUSH_OBJECT(asyncLock);
		METHOD_RETURN;
	}

    FORTHOP(oSystemCreateAsyncSemaphoreMethod)
    {
        ForthObject sem;
        OLock::CreateAsyncSemaphoreObject(sem, GET_ENGINE);

        PUSH_OBJECT(sem);
        METHOD_RETURN;
    }

    baseMethodEntry oSystemMembers[] =
	{
		METHOD("__newOp", oSystemNew),
		METHOD("delete", oSystemDeleteMethod),
		METHOD("stats", oSystemStatsMethod),
		METHOD_RET("getDefinitionsVocab", oSystemGetDefinitionsVocabMethod, RETURNS_OBJECT(kBCIVocabulary)),
		METHOD("setDefinitionsVocab", oSystemSetDefinitionsVocabMethod),
		METHOD("clearSearchVocab", oSystemClearSearchVocabMethod),
		METHOD("getSearchVocabDepth", oSystemGetSearchVocabDepthMethod),
		METHOD_RET("getSearchVocabAt", oSystemGetSearchVocabAtMethod, RETURNS_OBJECT(kBCIVocabulary)),
		METHOD_RET("getSearchVocabTop", oSystemGetSearchVocabTopMethod, RETURNS_OBJECT(kBCIVocabulary)),
		METHOD("setSearchVocabTop", oSystemSetSearchVocabTopMethod),
		METHOD("pushSearchVocab", oSystemPushSearchVocabMethod),
		METHOD_RET("getVocabByName", oSystemGetVocabByNameMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD_RET("getVocabChainHead", oSystemGetVocabChainHeadMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD_RET("getOpsTable", oSystemGetOpsTableMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("getClassByIndex", oSystemGetClassByIndexMethod, RETURNS_OBJECT(kBCIObject)),
        METHOD_RET("getNumClasses", oSystemGetNumClassesMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("createAsyncThread", oSystemCreateAsyncThreadMethod, RETURNS_OBJECT(kBCIAsyncThread)),
        METHOD_RET("createAsyncLock", oSystemCreateAsyncLockMethod, RETURNS_OBJECT(kBCIAsyncLock)),
        METHOD_RET("createAsyncSemaphore", oSystemCreateAsyncSemaphoreMethod, RETURNS_OBJECT(kBCIAsyncSemaphore)),

        MEMBER_VAR("namedObjects", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR("args", OBJECT_TYPE_TO_CODE(0, kBCIArray)),
        MEMBER_VAR("env", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),

		// following must be last in table
		END_MEMBERS
	};


	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("System", kBCISystem, kBCIObject, oSystemMembers);
	}

    void Shutdown(ForthEngine* pEngine)
    {
        ForthCoreState* pCore = pEngine->GetCoreState();
        SAFE_RELEASE(pCore, gSystemSingleton.args);
        SAFE_RELEASE(pCore, gSystemSingleton.env);
        SAFE_RELEASE(pCore, gSystemSingleton.namedObjects);
    }

} // namespace OSystem

