//////////////////////////////////////////////////////////////////////
//
// OSystem.cpp: builtin system class
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
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
#include "Thread.h"
#include "ForthPortability.h"
#include "InputStack.h"
#include "ControlStack.h"

#include "TypesManager.h"
#include "ClassVocabulary.h"

#include "OSystem.h"
#include "OStream.h"
#include "OString.h"

namespace OSystem
{
    static ClassVocabulary* gpControlStackVocab = nullptr;
    static ClassVocabulary* gpSearchStackVocab = nullptr;

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

        ALLOCATE_OBJECT(oObjectStruct, pControlStack, gpControlStackVocab);
        gSystemSingleton.controlStack = pControlStack;
        pControlStack->pMethods = gpControlStackVocab->GetMethods();
        pControlStack->refCount = 2000000000;

        ALLOCATE_OBJECT(oObjectStruct, pSearchStack, gpSearchStackVocab);
        gSystemSingleton.searchStack = pSearchStack;
        pSearchStack->pMethods = gpSearchStackVocab->GetMethods();
        pSearchStack->refCount = 2000000000;

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
        Vocabulary* pVocab = GET_ENGINE->GetOuterInterpreter()->GetDefinitionVocabulary();
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

		Vocabulary* pVocab = pVocabStruct->vocabulary;
		if (pVocab != NULL)
		{
            GET_ENGINE->GetOuterInterpreter()->SetDefinitionVocabulary(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oSystemGetVocabByNameMethod)
	{
		const char* pVocabName = reinterpret_cast<const char*>(SPOP);
		Vocabulary* pVocab = Vocabulary::FindVocabulary(pVocabName);
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
	
    FORTHOP(oSystemGetVocabByWidMethod)
    {
        ucell wid = SPOP;
        Engine* pEngine = GET_ENGINE;
        Vocabulary* pVocab = pEngine->GetVocabulary(wid);
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
        Vocabulary* pVocab = Vocabulary::GetVocabularyChainHead();
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
        ClassVocabulary* pClassVocab = TypesManager::GetInstance()->GetClassVocabulary(typeIndex);
        if (pClassVocab != nullptr)
        {
            classObject = (ForthObject) pClassVocab->GetClassObject();
        }

        PUSH_OBJECT(classObject);
        METHOD_RETURN;
    }


    FORTHOP(oSystemGetNumClassesMethod)
    {
        int numClasses = TypesManager::GetInstance()->GetNewestClass()->GetTypeIndex() + 1;
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
        Engine* pEngine = GET_ENGINE;
        InputStack* inputStack = pEngine->GetShell()->GetInput();

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
        Engine* pEngine = GET_ENGINE;
        ForthObject newWorkDirObj;

        POP_OBJECT(newWorkDirObj);
        oStringStruct* newWorkDirStr = reinterpret_cast<oStringStruct*>(newWorkDirObj);
        pEngine->GetShell()->GetFileInterface()->setWorkDir(newWorkDirStr->str->data);
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetWorkDirMethod)
    {
        Engine* pEngine = GET_ENGINE;
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

    FORTHOP(oSystemAddUsingVocabularyMethod)
    {
        Engine* pEngine = GET_ENGINE;

        const char* pVocabName = (const char*)(SPOP);
        Vocabulary* pVocab = Vocabulary::FindVocabulary(pVocabName);
        if (pVocab)
        {
            OuterInterpreter* outer = pEngine->GetOuterInterpreter();
            outer->AddUsingVocabulary(pVocab);
        }
        METHOD_RETURN;
    }
    
    FORTHOP(oSystemClearUsingVocabulariesMethod)
    {
        Engine* pEngine = GET_ENGINE;
        OuterInterpreter* outer = pEngine->GetOuterInterpreter();
        outer->ClearUsingVocabularies();
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetControlStackMethod)
    {
        GET_THIS(oSystemStruct, pSystem);
        PUSH_OBJECT(pSystem->controlStack);
        METHOD_RETURN;
    }

    FORTHOP(oSystemGetSearchStackMethod)
    {
        GET_THIS(oSystemStruct, pSystem);
        PUSH_OBJECT(pSystem->searchStack);
        METHOD_RETURN;
    }

    baseMethodEntry oSystemMembers[] =
    {
        METHOD("__newOp", oSystemNew),
        METHOD("delete", oSystemDeleteMethod),
        METHOD("stats", oSystemStatsMethod),
        METHOD_RET("getDefinitionsVocab", oSystemGetDefinitionsVocabMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD("setDefinitionsVocab", oSystemSetDefinitionsVocabMethod),
        METHOD_RET("getVocabByName", oSystemGetVocabByNameMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD_RET("getVocabByWid", oSystemGetVocabByWidMethod, RETURNS_OBJECT(kBCIVocabulary)),
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
        METHOD("addUsingVocabulary", oSystemAddUsingVocabularyMethod),
        METHOD("clearUsingVocabularies", oSystemClearUsingVocabulariesMethod),
        METHOD_RET("getControlStack", oSystemGetControlStackMethod, RETURNS_OBJECT(kBCIControlStack)),
        METHOD_RET("getSearchStack", oSystemGetSearchStackMethod, RETURNS_OBJECT(kBCISearchStack)),

        MEMBER_VAR("namedObjects", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR("args", OBJECT_TYPE_TO_CODE(0, kBCIArray)),
        MEMBER_VAR("env", OBJECT_TYPE_TO_CODE(0, kBCIStringMap)),
        MEMBER_VAR("controlStack", OBJECT_TYPE_TO_CODE(0, kBCIControlStack)),
        MEMBER_VAR("searchStack", OBJECT_TYPE_TO_CODE(0, kBCISearchStack)),
        
		// following must be last in table
		END_MEMBERS
	};


    //////////////////////////////////////////////////////////////////////
    ///
    //                 OControlStack
    //

    FORTHOP(oControlStackDeleteMethod)
    {
        GET_THIS(oObjectStruct, obj);
        obj->refCount = 2000000000;
        // TODO: warn that something tried to delete system
        METHOD_RETURN;
    }

    FORTHOP(oControlStackSizeMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        SPUSH(stack->GetSize());
        METHOD_RETURN;
    }

    FORTHOP(oControlStackDepthMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        SPUSH(stack->GetDepth());
        METHOD_RETURN;
    }

    FORTHOP(oControlStackPushMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        forthop op = (forthop)(SPOP);
        const char* pName = (const char*)(SPOP);
        void* pAddress = (void*)(SPOP);
        ControlStackTag tag = (ControlStackTag)(SPOP);
        stack->Push(tag, pAddress, pName, op);
        METHOD_RETURN;
    }

    FORTHOP(oControlStackPeekMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        ControlStackEntry* pEntry = stack->Peek();
        SPUSH((cell)pEntry);
        METHOD_RETURN;
    }

    FORTHOP(oControlStackPeekAtMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        ucell index = SPOP;
        ControlStackEntry* pEntry = stack->Peek(index);
        SPUSH((cell)pEntry);
        METHOD_RETURN;
    }

    FORTHOP(oControlStackDropMethod)
    {
        ControlStack* stack = GET_ENGINE->GetShell()->GetControlStack();
        stack->Drop();
        METHOD_RETURN;
    }

    FORTHOP(oControlStackLastUsedTagMethod)
    {
        ucell v = (cell)kCSTagLastTag;
        SPUSH(v);
        METHOD_RETURN;
    }
    
    baseMethodEntry oControlStackMembers[] =
    {
        METHOD("delete", oControlStackDeleteMethod),
        METHOD_RET("size", oControlStackSizeMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("depth", oControlStackDepthMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD("push", oControlStackPushMethod),
        METHOD_RET("peek", oControlStackPeekMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("peekAt", oControlStackPeekAtMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD("drop", oControlStackDropMethod),
        METHOD_RET("lastUsedTag", oControlStackLastUsedTagMethod, RETURNS_NATIVE(BaseType::kCell)),

        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 OSearchStack
    //

    FORTHOP(oSearchStackDeleteMethod)
    {
        GET_THIS(oObjectStruct, obj);
        obj->refCount = 2000000000;
        // TODO: warn that something tried to delete system
        METHOD_RETURN;
    }

    FORTHOP(oSearchStackClearMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        pVocabStack->Clear();

        METHOD_RETURN;
    }

    FORTHOP(oSearchStackDepthMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        SPUSH(pVocabStack->GetDepth());

        METHOD_RETURN;
    }

    FORTHOP(oSearchStackGetAtMethod)
    {
        int vocabStackIndex = (int)SPOP;

        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        Vocabulary* pVocab = pVocabStack->GetElement(vocabStackIndex);
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

    FORTHOP(oSearchStackGetTopMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        Vocabulary* pVocab = pVocabStack->GetTop();
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

    FORTHOP(oSearchStackSetTopMethod)
    {
        ForthObject vocabObj;
        POP_OBJECT(vocabObj);
        oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct*>(vocabObj);

        Vocabulary* pVocab = pVocabStruct->vocabulary;
        if (pVocab != NULL)
        {
            VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
            pVocabStack->SetTop(pVocab);
        }
        METHOD_RETURN;
    }

    FORTHOP(oSearchStackDupTopMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        pVocabStack->DupTop();

        METHOD_RETURN;
    }

    FORTHOP(oSearchStackPushMethod)
    {
        ForthObject vocabObj;
        POP_OBJECT(vocabObj);
        oVocabularyStruct* pVocabStruct = reinterpret_cast<oVocabularyStruct*>(vocabObj);

        Vocabulary* pVocab = pVocabStruct->vocabulary;
        if (pVocab != NULL)
        {
            VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
            pVocabStack->DupTop();
            pVocabStack->SetTop(pVocab);
        }
        METHOD_RETURN;
    }

    FORTHOP(oSearchStackFindMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        const char* pSymbol = (const char*)(SPOP);
        Vocabulary* pFoundVocabulary = nullptr;

        forthop* pEntry = pVocabStack->FindSymbol(pSymbol, &pFoundVocabulary);
        SPUSH((cell)pEntry);
        if (pFoundVocabulary == nullptr)
        {
            SPUSH(0);
        }
        else
        {
            PUSH_OBJECT(pFoundVocabulary->GetVocabularyObject());
        }

        METHOD_RETURN;
    }

    FORTHOP(oSearchStackFindByValueMethod)
    {
        VocabularyStack* pVocabStack = GET_ENGINE->GetOuterInterpreter()->GetVocabularyStack();
        forthop soughtValue = (forthop)(SPOP);
        Vocabulary* pFoundVocabulary = nullptr;

        forthop* pEntry = pVocabStack->FindSymbolByValue(soughtValue, &pFoundVocabulary);
        SPUSH((cell)pEntry);
        if (pFoundVocabulary == nullptr)
        {
            SPUSH(0);
        }
        else
        {
            PUSH_OBJECT(pFoundVocabulary->GetVocabularyObject());
        }

        METHOD_RETURN;
    }

    baseMethodEntry oSearchStackMembers[] =
    {
        METHOD("delete", oSearchStackDeleteMethod),
        METHOD("clear", oSearchStackClearMethod),
        METHOD("depth", oSearchStackDepthMethod),
        METHOD_RET("getAt", oSearchStackGetAtMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD_RET("getTop", oSearchStackGetTopMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD("setTop", oSearchStackSetTopMethod),
        METHOD("dupTop", oSearchStackDupTopMethod),
        METHOD("push", oSearchStackPushMethod),
        METHOD_RET("find", oSearchStackFindMethod, RETURNS_NATIVE(BaseType::kCell)),
        METHOD_RET("findByValue", oSearchStackFindByValueMethod, RETURNS_NATIVE(BaseType::kCell)),

        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////
    void AddClasses(OuterInterpreter* pOuter)
	{
        gpControlStackVocab = pOuter->AddBuiltinClass("ControlStack", kBCIControlStack, kBCIObject, oControlStackMembers);
        gpSearchStackVocab = pOuter->AddBuiltinClass("SearchStack", kBCISearchStack, kBCIObject, oSearchStackMembers);
        pOuter->AddBuiltinClass("System", kBCISystem, kBCIObject, oSystemMembers);
	}

    void Shutdown(Engine* pEngine)
    {
        CoreState* pCore = pEngine->GetCoreState();
        SAFE_RELEASE(pCore, gSystemSingleton.args);
        SAFE_RELEASE(pCore, gSystemSingleton.env);
        SAFE_RELEASE(pCore, gSystemSingleton.namedObjects);
        // TODO: cleanup system, search stack and control stack objects
    }

} // namespace OSystem

