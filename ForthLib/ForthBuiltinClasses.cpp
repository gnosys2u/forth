//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.cpp: builtin classes
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
#include "ForthBlockFileManager.h"

#include "OArray.h"
#include "OList.h"
#include "OString.h"
#include "OMap.h"
#include "OStream.h"
#include "ONumber.h"
#include "OSystem.h"
#include "OSocket.h"

#ifdef TRACK_OBJECT_ALLOCATIONS
long gStatNews = 0;
long gStatDeletes = 0;
long gStatLinkNews = 0;
long gStatLinkDeletes = 0;
long gStatIterNews = 0;
long gStatIterDeletes = 0;
long gStatKeeps = 0;
long gStatReleases = 0;
#endif

// first time OString:printf fails due to overflow, it buffer is increased to this size
#define OSTRING_PRINTF_FIRST_OVERFLOW_SIZE 256
// this is size limit of buffer expansion upon OString:printf overflow
#define OSTRING_PRINTF_LAST_OVERFLOW_SIZE 0x2000000

extern "C" {
	unsigned long SuperFastHash (const char * data, int len, unsigned long hash);
	extern void unimplementedMethodOp( ForthCoreState *pCore );
	extern void illegalMethodOp( ForthCoreState *pCore );
	extern int oStringFormatSub( ForthCoreState* pCore, char* pBuffer, int bufferSize );
};

#ifdef WIN32
float __cdecl cdecl_boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}

float __stdcall stdcall_boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}

float boohoo(int aa, int bb, int cc)
{
	return (float)((aa + bb) * cc);
}
#endif

#if defined(LINUX) || defined(MACOSX)
#define SNPRINTF snprintf
#else
#define SNPRINTF _snprintf
#endif

void unrefObject(ForthObject& fobj)
{
	ForthEngine *pEngine = ForthEngine::GetInstance();
	if (fobj != nullptr)
	{
		if (fobj->refCount == 0)
		{
			pEngine->SetError(kForthErrorBadReferenceCount, " unref with refcount already zero");
		}
		else
		{
			fobj->refCount -= 1;
		}
	}
}

namespace
{

    //////////////////////////////////////////////////////////////////////
    ///
    //                 object
    //
    FORTHOP(objectNew)
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        long nBytes = pClassVocab->GetSize();
        ForthObject pThis = (ForthObject)__MALLOC(nBytes);
        // clear the entire object area - this handles both its refcount and any object pointers it might contain
        memset(pThis, 0, nBytes);
        pThis->pMethods = pClassVocab->GetMethods();
        TRACK_NEW;
        PUSH_OBJECT(pThis);
    }

    FORTHOP(objectDeleteMethod)
    {
        // TODO: warn if refcount isn't zero
        FREE_OBJECT(GET_TP);
        METHOD_RETURN;
    }

    FORTHOP(objectShowMethod)
    {
        ForthObject obj = GET_TP;
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        ForthClassObject* pClassObject = (ForthClassObject *)(*((obj->pMethods)-1));
        ForthEngine *pEngine = ForthEngine::GetInstance();

        if (pShowContext->ObjectAlreadyShown(obj))
        {
            pShowContext->ShowObjectLink(obj);
        }
        else
        {
            pShowContext->AddObject(obj);
            ForthClassVocabulary* pClassVocab = pClassObject->pVocab;

            pShowContext->BeginObject(pClassVocab->GetName(), obj, true);

            long* originalMethods = obj->pMethods;
            while (pClassVocab != nullptr)
            {
                pEngine->FullyExecuteMethod(pCore, obj, kMethodShowInner);
                pClassVocab = (ForthClassVocabulary *)pClassVocab->BaseVocabulary();
                if (pClassVocab != nullptr)
                {
                    // slightly horrible, but efficient
                    obj->pMethods = pClassVocab->GetMethods();
                }
            }
            obj->pMethods = originalMethods;

            pShowContext->EndObject();

            if (pShowContext->GetDepth() == 0)
            {
                pEngine->ConsoleOut("\n");
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(objectShowInnerMethod)
    {
        ForthObject obj = GET_TP;
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        ForthClassObject* pClassObject = (ForthClassObject *)(*((obj->pMethods)-1));
        ForthEngine *pEngine = ForthEngine::GetInstance();

        pClassObject->pVocab->ShowDataInner(GET_TP, pCore);
        METHOD_RETURN;
    }

    FORTHOP(objectClassMethod)
	{
		// this is the big gaping hole - where should the pointer to the class vocabulary be stored?
		// we could store it in the slot for method 0, but that would be kind of clunky - also,
		// would slot 0 of non-primary interfaces also have to hold it?
		// the class object is stored in the long before method 0
        ForthObject obj = GET_TP;
        ForthClassObject* pClassObject = (ForthClassObject *)(*((obj->pMethods) - 1));
        PUSH_OBJECT(pClassObject);
		METHOD_RETURN;
	}

	FORTHOP(objectCompareMethod)
	{
		ForthObject thatVal;
		POP_OBJECT(thatVal);
        ForthObject thisVal = (ForthObject)(GET_TP);
        long result = 0;
		if (thisVal != thatVal)
		{
			result = (thisVal > thatVal) ? 1 : -1;
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(objectKeepMethod)
	{
        ForthObject obj = GET_TP;
		obj->refCount += 1;
		TRACK_KEEP;
		METHOD_RETURN;
	}

	FORTHOP(objectReleaseMethod)
	{
        ForthObject obj = GET_TP;
		obj->refCount -= 1;
		TRACK_RELEASE;
		if (obj->refCount != 0)
		{
			METHOD_RETURN;
		}
		else
		{
			ForthEngine *pEngine = ForthEngine::GetInstance();
			ulong deleteOp = obj->pMethods[kMethodDelete];
			pEngine->ExecuteOp(pCore, deleteOp);
			// we are effectively chaining to the delete op, its method return will pop TPM & TPD for us
		}
	}

	baseMethodEntry objectMembers[] =
	{
		METHOD("__newOp", objectNew),
		METHOD("delete", objectDeleteMethod),
		METHOD("show", objectShowMethod),
        METHOD("showInner", objectShowInnerMethod),
        METHOD_RET("getClass", objectClassMethod, RETURNS_OBJECT(kBCIClass)),
		METHOD_RET("compare", objectCompareMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("keep", objectKeepMethod),
		METHOD("release", objectReleaseMethod),
        MEMBER_VAR("__methods", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("__refCount", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        // following must be last in table
		END_MEMBERS
	};


    //////////////////////////////////////////////////////////////////////
    ///
    //                 ContainedType
    //
    FORTHOP(oContainedTypeNew)
    {
        GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a ContainedType object");
    }

    baseMethodEntry containedTypeMembers[] =
    {
        METHOD("__newOp", oContainedTypeNew),
        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
	///
	//                 Class
	//
	FORTHOP(classCreateMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
		SPUSH((long)pClassObject->pVocab);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		METHOD_RETURN;
		pEngine->ExecuteOp(pCore, pClassObject->newOp);
	}

	FORTHOP(classSuperMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
		ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
        ForthObject superObj = nullptr;
        if (pClassVocab->IsClass())
        {
            superObj = ((ForthClassVocabulary *)(pClassVocab->BaseVocabulary()))->GetVocabularyObject();
        }
		// what should happen if a class is derived from a struct?
		PUSH_OBJECT(superObj);
		METHOD_RETURN;
	}

	FORTHOP(classNameMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
		ForthClassVocabulary* pClassVocab = pClassObject->pVocab;
		const char* pName = pClassVocab->GetName();
		SPUSH((long)pName);
		METHOD_RETURN;
	}

	FORTHOP(classVocabularyMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
		SPUSH((long)(pClassObject->pVocab->GetClassObject()));
		METHOD_RETURN;
	}

    FORTHOP(classGetTypeIndexMethod)
    {
        ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
        SPUSH(pClassObject->pVocab->GetTypeIndex());
        METHOD_RETURN;
    }
    
	FORTHOP(classDeleteMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot delete a class object");
		METHOD_RETURN;
	}

	FORTHOP(classSetNewMethod)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(GET_TP);
		pClassObject->newOp = SPOP;
		METHOD_RETURN;
	}

	baseMethodEntry classMembers[] =
	{
		METHOD("delete", classDeleteMethod),
		METHOD_RET("create", classCreateMethod, RETURNS_OBJECT(kBCIObject)),
		METHOD_RET("getParent", classSuperMethod, RETURNS_OBJECT(kBCIClass)),
		METHOD_RET("getName", classNameMethod, RETURNS_NATIVE(kDTIsPtr|kBaseTypeByte)),
        METHOD_RET("getVocabulary", classVocabularyMethod, RETURNS_OBJECT(kBCIVocabulary)),
        METHOD_RET("getTypeIndex", classGetTypeIndexMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("setNew", classSetNewMethod),

		MEMBER_VAR("vocab", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("newOp", NATIVE_TYPE_TO_CODE(0, kBaseTypeOp)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIter
	//

	// oIter is an abstract iterator class

	baseMethodEntry oIterMembers[] =
	{
		METHOD("seekNext", unimplementedMethodOp),
		METHOD("seekPrev", unimplementedMethodOp),
		METHOD("seekHead", unimplementedMethodOp),
		METHOD("seekTail", unimplementedMethodOp),
        METHOD_RET("atHead", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", unimplementedMethodOp),
		METHOD_RET("unref", unimplementedMethodOp, RETURNS_OBJECT(kBCIObject)),
        METHOD_RET("clone", unimplementedMethodOp, RETURNS_OBJECT(kBCIIter)),
        // following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oIterable
	//

	// oIterable is an abstract iterable class, containers should be derived from oIterable

	baseMethodEntry oIterableMembers[] =
	{
		METHOD_RET("headIter", unimplementedMethodOp, RETURNS_OBJECT(kBCIIter)),
		METHOD_RET("tailIter", unimplementedMethodOp, RETURNS_OBJECT(kBCIIter)),
		METHOD_RET("find", unimplementedMethodOp, RETURNS_OBJECT(kBCIIter)),
		METHOD_RET("clone", unimplementedMethodOp, RETURNS_OBJECT(kBCIIterable)),
		METHOD_RET("count", unimplementedMethodOp, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", unimplementedMethodOp),
		// following must be last in table
		END_MEMBERS
	};


} // namespace

// return true IFF object was already shown
bool ForthShowAlreadyShownObject(ForthObject obj, ForthCoreState* pCore, bool addIfUnshown)
{
	ForthEngine* pEngine = ForthEngine::GetInstance();
	ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
	if (obj != nullptr)
	{
		ForthClassObject* pClassObject = (ForthClassObject *)(obj->pMethods[-1]);
		if (pShowContext->ObjectAlreadyShown(obj))
		{
			pShowContext->ShowObjectLink(obj);
		}
		else
		{
			if (addIfUnshown)
			{
				pShowContext->AddObject(obj);
			}
			return false;
		}
	}
	else
	{
        ForthObject nullObj = nullptr;
		pShowContext->ShowObjectLink(nullObj);
	}
	return true;
}

void ForthShowObject(ForthObject& obj, ForthCoreState* pCore)
{
	if (!ForthShowAlreadyShownObject(obj, pCore, false))
	{
		ForthEngine* pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		pEngine->FullyExecuteMethod(pCore, obj, kMethodShow);
		pShowContext->AddObject(obj);
	}
}


//////////////////////////////////////////////////////////////////////
////
///     ForthForgettableGlobalObject - handles forgetting of global forth objects
//
// 
ForthForgettableGlobalObject::ForthForgettableGlobalObject( const char* pName, void* pOpAddress, long op, int numElements )
: ForthForgettable( pOpAddress, op )
,	mNumElements( numElements )
{
    int nameLen = strlen( pName );
    mpName = (char *) __MALLOC(nameLen + 1);
    strcpy( mpName, pName );
}

ForthForgettableGlobalObject::~ForthForgettableGlobalObject()
{
    __FREE( mpName );
}

const char *
ForthForgettableGlobalObject::GetName( void )
{
    return mpName;
}

const char *
ForthForgettableGlobalObject::GetTypeName( void )
{
    return "globalObject";
}

void ForthForgettableGlobalObject::ForgetCleanup( void* pForgetLimit, long op )
{
	// first longword is OP_DO_OBJECT or OP_DO_OBJECT_ARRAY, after that are object elements
	if ((ulong)mpOpAddress > (ulong)pForgetLimit)
	{
		ForthObject* pObject = (ForthObject *)((long *)mpOpAddress + 1);
		ForthCoreState* pCore = ForthEngine::GetInstance()->GetCoreState();
		for (int i = 0; i < mNumElements; i++)
		{
			// TODO: release each 
			SAFE_RELEASE(pCore, pObject[i]);
            pObject[i] = nullptr;
		}
	}
}

const char *
ForthTypesManager::GetName( void )
{
    return GetTypeName();
}

const char *
ForthTypesManager::GetTypeName( void )
{
    return "typesManager";
}

// TODO: find a better way to do this
long gObjectShowInnerOpcode = 0;

void
ForthTypesManager::AddBuiltinClasses(ForthEngine* pEngine)
{

    ForthClassVocabulary* pObjectClassVocab = pEngine->AddBuiltinClass("Object", kBCIObject, kBCIInvalid, objectMembers);
    gObjectShowInnerOpcode = pObjectClassVocab->GetInterface(0)->GetMethod(kMethodShowInner);

    ForthClassVocabulary* pClassClassVocab = pEngine->AddBuiltinClass("Class", kBCIClass, kBCIObject, classMembers);
    mpClassMethods = pClassClassVocab->GetMethods();

    // set the methods for class objects "Class" and "Object", they were created
    //   before "Class" had set the Class object methods pointer in type manager
    // no classes defined after this will need to call FixClassObjectMethods
    pObjectClassVocab->FixClassObjectMethods();
    pClassClassVocab->FixClassObjectMethods();

    pEngine->AddBuiltinClass("ContainedType", kBCIContainedType, kBCIObject, containedTypeMembers);

	pEngine->AddBuiltinClass("Iter", kBCIIter, kBCIObject, oIterMembers);
    pEngine->AddBuiltinClass("Iterable", kBCIIterable, kBCIObject, oIterableMembers);

	OArray::AddClasses(pEngine);
	OList::AddClasses(pEngine);
	OMap::AddClasses(pEngine);
	OString::AddClasses(pEngine);
	OStream::AddClasses(pEngine);
    OBlockFile::AddClasses(pEngine);
    ONumber::AddClasses(pEngine);
	OVocabulary::AddClasses(pEngine);
	OThread::AddClasses(pEngine);
	OLock::AddClasses(pEngine);
    OSystem::AddClasses(pEngine);
    OSocket::AddClasses(pEngine);
}

void
ForthTypesManager::ShutdownBuiltinClasses(ForthEngine* pEngine)
{
    OSystem::Shutdown(pEngine);
}