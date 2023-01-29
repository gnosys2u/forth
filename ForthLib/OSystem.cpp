//////////////////////////////////////////////////////////////////////
//
// OSystem.cpp: builtin system class
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdio.h>
#include <string.h>
#include <map>

#include "ForthEngine.h"
#include "OuterInterpreter.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"
#include "ForthThread.h"
#include "ForthPortability.h"

#include "OSystem.h"
#include "OStream.h"
#include "OString.h"

namespace OSystem
{
    static ClassVocabulary *gpShellStackVocab = nullptr;

	//////////////////////////////////////////////////////////////////////
	///
	//                 OSystem
	//

    static oSystemStruct gSystemSingleton;

    FORTHOP(oSystemNew)
	{
		ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
        ForthObject obj;
        gSystemSingleton.refCount = 2000000000;
        CLEAR_OBJECT(gSystemSingleton.namedObjects);
        CLEAR_OBJECT(gSystemSingleton.args);
        CLEAR_OBJECT(gSystemSingleton.env);
        obj = reinterpret_cast<ForthObject>(&gSystemSingleton);
        obj->pMethods = pClassVocab->GetMethods();

        ALLOCATE_OBJECT(oObjectStruct, pShellStack, gpShellStackVocab);
        gSystemSingleton.shellStack = pShellStack;
        pShellStack->pMethods = gpShellStackVocab->GetMethods();
        pShellStack->refCount = 2000000000;

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

        OuterInterpreter* pOuter = GET_ENGINE->GetOuterInterpreter();
        pOuter->ShowSearchInfo();

		METHOD_RETURN;
	}


	FORTHOP(oSystemGetDefinitionsVocabMethod)
	{
        ForthVocabulary* pVocab = GET_ENGINE->GetOuterInterpreter()->GetDefinitionVocabulary();
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
		ForthObject vocabObj;
		POP_OBJECT(vocabObj);
		oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct *>(vocabObj);

		ForthVocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
            GET_ENGINE->GetOuterInterpreter()->SetDefinitionVocabulary(pVocab);
		}
		METHOD_RETURN;
	}

    FORTHOP(oSystemClearSearchVocabMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        pVocabStack->Clear();

        METHOD_RETURN;
    }

    FORTHOP(oSystemGetSearchVocabDepthMethod)
	{
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
		SPUSH(pVocabStack->GetDepth());

		METHOD_RETURN;
	}

	FORTHOP(oSystemGetSearchVocabAtMethod)
	{
		int vocabStackIndex = (int) SPOP;

        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
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
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
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
			VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
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
			VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
			pVocabStack->DupTop();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetVocabByNameMethod)
	{
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
        const char* pVocabName = reinterpret_cast<const char*>(SPOP);
        ForthVocabulary* pVocab = ForthVocabulary::GetVocabularyChainHead();
        PUSH_OBJECT(pVocab->GetVocabularyObject());
        METHOD_RETURN;
    }

    
	FORTHOP(oSystemGetOpsTableMethod)
	{
		SPUSH((cell)(pCore->ops));
		METHOD_RETURN;
	}


    FORTHOP(oSystemGetClassByIndexMethod)
    {
        int typeIndex = (int)SPOP;
        ForthObject classObject = nullptr;
        ClassVocabulary* pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(typeIndex);
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

	FORTHOP(oSystemCreateThreadMethod)
	{
		ForthObject asyncThread;
		int returnStackLongs = (int)SPOP;
		int paramStackLongs = (int)SPOP;
        forthop threadOp = (forthop)SPOP;
		OThread::CreateThreadObject(asyncThread, GET_ENGINE, threadOp, paramStackLongs, returnStackLongs);

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

    FORTHOP(oSystemGetConsoleOutMethod)
    {
        PUSH_OBJECT(OStream::getStdoutObject());
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetErrorOutMethod)
    {
        PUSH_OBJECT(OStream::getStderrObject());
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetInputInfoMethod)
    {
        ForthEngine* pEngine = GET_ENGINE;
        ForthInputStack* inputStack = pEngine->GetShell()->GetInput();

        int lineNumber;
        const char* filename = inputStack->GetFilenameAndLineNumber(lineNumber);
        int lineOffset = inputStack->GetReadOffset();
        const char* line = inputStack->GetBufferBasePointer();

        SPUSH((cell)line);
        SPUSH((cell)filename);
        SPUSH((cell)lineNumber);
        SPUSH((cell)lineOffset);

        METHOD_RETURN;
    }

    FORTHOP(oSystemSetWorkDirMethod)
    {
        ForthEngine* pEngine = GET_ENGINE;
        ForthObject newWorkDirObj;

        POP_OBJECT(newWorkDirObj);
        oStringStruct* newWorkDirStr = reinterpret_cast<oStringStruct*>(newWorkDirObj);
        pEngine->GetShell()->GetFileInterface()->setWorkDir(newWorkDirStr->str->data);
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetWorkDirMethod)
    {
        ForthEngine* pEngine = GET_ENGINE;
        ForthObject workDirObj;

        POP_OBJECT(workDirObj);
        oStringStruct* workDirStr = reinterpret_cast<oStringStruct*>(workDirObj);
        int maxLen = workDirStr->str->maxLen;
        int len = pEngine->GetShell()->GetFileInterface()->getWorkDir(workDirStr->str->data, maxLen);
        if (len >= maxLen)
        {
            len += 4;
            oString* newString = OString::resizeOString(workDirStr, len);
            pEngine->GetShell()->GetFileInterface()->getWorkDir(newString->data, len);
        }
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
        METHOD_RET("getOpsTable", oSystemGetOpsTableMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("getClassByIndex", oSystemGetClassByIndexMethod, RETURNS_OBJECT(kBCIObject)),
        METHOD_RET("getNumClasses", oSystemGetNumClassesMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("createThread", oSystemCreateThreadMethod, RETURNS_OBJECT(kBCIThread)),
        METHOD_RET("createAsyncLock", oSystemCreateAsyncLockMethod, RETURNS_OBJECT(kBCIAsyncLock)),
        METHOD_RET("createAsyncSemaphore", oSystemCreateAsyncSemaphoreMethod, RETURNS_OBJECT(kBCIAsyncSemaphore)),
        METHOD_RET("getInputInfo", oSystemGetInputInfoMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("getStdOut", oSystemGetConsoleOutMethod, RETURNS_OBJECT(kBCIConsoleOutStream)),
        METHOD_RET("getErrOut", oSystemGetErrorOutMethod, RETURNS_OBJECT(kBCIErrorOutStream)),
        METHOD("setWorkDir", oSystemSetWorkDirMethod),
        METHOD("getWorkDir", oSystemGetWorkDirMethod),

        MEMBER_VAR("namedObjects", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR("args", OBJECT_TYPE_TO_CODE(0, kBCIArray)),
        MEMBER_VAR("env", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR("shellStack", OBJECT_TYPE_TO_CODE(0, kBCIShellStack)),

		// following must be last in table
		END_MEMBERS
	};


    //////////////////////////////////////////////////////////////////////
    ///
    //                 OShellStack
    //

    FORTHOP(oShellStackDeleteMethod)
    {
        GET_THIS(oObjectStruct, obj);
        obj->refCount = 2000000000;
        // TODO: warn that something tried to delete system
        METHOD_RETURN;
    }

    FORTHOP(oShellStackSizeMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        SPUSH(stack->GetSize());
        METHOD_RETURN;
    }

    FORTHOP(oShellStackDepthMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        SPUSH(stack->GetDepth());
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPushMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = SPOP;
        stack->Push(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPushTagMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = SPOP;
        stack->PushTag((eShellTag) v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPushAddressMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = SPOP;
        stack->PushAddress((forthop *)v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPushStringMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = SPOP;
        stack->PushString((const char *)v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPopMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = stack->Pop();
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPopTagMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = (cell) stack->PopTag();
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPopAddressMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        cell v = (cell)stack->PopAddress();
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPopStringMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        int maxLen = (int)SPOP;
        char* pDstString = (char *)SPOP;
        bool isString = stack->PopString(pDstString, maxLen);
        SPUSH(isString ? -1 : 0);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPeekMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        int i = (int) SPOP;
        cell v = stack->Peek(i);
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPeekTagMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        int i = (int)SPOP;
        cell v = (cell)stack->PeekTag(i);
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackPeekAddressMethod)
    {
        ForthShellStack* stack = GET_ENGINE->GetShell()->GetShellStack();
        int i = (int)SPOP;
        cell v = (cell)stack->PeekAddress(i);
        SPUSH(v);
        METHOD_RETURN;
    }

    FORTHOP(oShellStackLastUsedTagMethod)
    {
        cell v = (cell)kShellLastTag;
        SPUSH(v);
        METHOD_RETURN;
    }
    
    baseMethodEntry oShellStackMembers[] =
    {
        METHOD("delete", oShellStackDeleteMethod),
        METHOD_RET("size", oShellStackSizeMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("depth", oShellStackDepthMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD("push", oShellStackPushMethod),
        METHOD("pushTag", oShellStackPushTagMethod),
        METHOD("pushAddress", oShellStackPushAddressMethod),
        METHOD("pushString", oShellStackPushStringMethod),
        METHOD_RET("pop", oShellStackPopMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("popTag", oShellStackPopTagMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("popAddress", oShellStackPopAddressMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("popString", oShellStackPopStringMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("peek", oShellStackPopMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("peekTag", oShellStackPopTagMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("peekAddress", oShellStackPopAddressMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("peekString", oShellStackPopStringMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("lastUsedTag", oShellStackLastUsedTagMethod, RETURNS_NATIVE(BaseType::kCell)),

        // following must be last in table
        END_MEMBERS
    };


    void AddClasses(OuterInterpreter* pOuter)
	{
        gpShellStackVocab = pOuter->AddBuiltinClass("ShellStack", kBCIShellStack, kBCIObject, oShellStackMembers);
        pOuter->AddBuiltinClass("System", kBCISystem, kBCIObject, oSystemMembers);
	}

    void Shutdown(ForthEngine* pEngine)
    {
        ForthCoreState* pCore = pEngine->GetCoreState();
        SAFE_RELEASE(pCore, gSystemSingleton.args);
        SAFE_RELEASE(pCore, gSystemSingleton.env);
        SAFE_RELEASE(pCore, gSystemSingleton.namedObjects);
    }

} // namespace OSystem

