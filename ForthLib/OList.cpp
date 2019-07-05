//////////////////////////////////////////////////////////////////////
//
// OList.cpp: builtin list related classes
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
#include "ForthObjectReader.h"

#include "OList.h"
#include "OArray.h"

namespace OList
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 List
	//

	FORTHOP(oListNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oListStruct, pList, pClassVocab);
        pList->pMethods = pClassVocab->GetMethods();
		pList->refCount = 0;
		pList->head = NULL;
		pList->tail = NULL;
		PUSH_OBJECT(pList);
	}

	FORTHOP(oListDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		while (pCur != NULL)
		{
			oListElement* pNext = pCur->next;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
			pCur = pNext;
		}
		FREE_OBJECT(pList);
		METHOD_RETURN;
	}

	FORTHOP(oListShowInnerMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
		while (pCur != NULL)
		{
            pShowContext->BeginArrayElement(1);
            oListElement* pNext = pCur->next;
			ForthShowObject(pCur->obj, pCore);
            pCur = pNext;
		}
        pShowContext->EndArray();
		METHOD_RETURN;
	}

    FORTHOP(oListHeadIterMethod)
    {
        GET_THIS(oListStruct, pList);
        pList->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
        MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pList);
        pIter->cursor = pList->head;
        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oListTailIterMethod)
    {
        GET_THIS(oListStruct, pList);
        pList->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
        MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pList);
        pIter->cursor = NULL;
        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oListFindMethod)
    {
        GET_THIS(oListStruct, pList);
        oListElement* pCur = pList->head;
        ForthObject soughtObj;
        POP_OBJECT(soughtObj);
        while (pCur != NULL)
        {
            ForthObject& o = pCur->obj;
            if (OBJECTS_SAME(o, soughtObj))
            {
                break;
            }
            pCur = pCur->next;
        }
        if (pCur == NULL)
        {
            SPUSH(0);
        }
        else
        {
            pList->refCount++;
            TRACK_KEEP;
            ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIListIter);
            MALLOCATE_ITER(oListIterStruct, pIter, pIterVocab);
            pIter->pMethods = pIterVocab->GetMethods();
            pIter->refCount = 0;
            pIter->parent = reinterpret_cast<ForthObject>(pList);
            pIter->cursor = pCur;
            PUSH_OBJECT(pIter);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oListCloneMethod)
    {
        // create an empty list
        ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIList);
        MALLOCATE_OBJECT(oListStruct, pCloneList, pClassVocab);
        pCloneList->pMethods = pClassVocab->GetMethods();
        pCloneList->refCount = 0;
        pCloneList->head = NULL;
        pCloneList->tail = NULL;
        // go through all elements and add a reference to any which are not null and add to clone list
        GET_THIS(oListStruct, pList);
        oListElement* pCur = pList->head;
        oListElement* oldTail = NULL;
        while (pCur != NULL)
        {
            oListElement* pNext = pCur->next;
            MALLOCATE_LINK(oListElement, newElem);
            newElem->obj = pCur->obj;
            SAFE_KEEP(newElem->obj);
            if (oldTail == NULL)
            {
                pCloneList->head = newElem;
            }
            else
            {
                oldTail->next = newElem;
            }
            newElem->prev = oldTail;
            oldTail = newElem;
            pCur = pNext;
        }
        if (oldTail != NULL)
        {
            oldTail->next = NULL;
        }
        PUSH_OBJECT(pCloneList);
        METHOD_RETURN;
    }

    FORTHOP(oListCountMethod)
    {
        GET_THIS(oListStruct, pList);
        long count = 0;
        oListElement* pCur = pList->head;
        while (pCur != NULL)
        {
            count++;
            pCur = pCur->next;
        }
        SPUSH(count);
        METHOD_RETURN;
    }

    FORTHOP(oListClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oListStruct, pList);
        oListElement* pCur = pList->head;
        while (pCur != NULL)
        {
            oListElement* pNext = pCur->next;
            SAFE_RELEASE(pCore, pCur->obj);
            FREE_LINK(pCur);
            pCur = pNext;
        }
        pList->head = NULL;
        pList->tail = NULL;
        METHOD_RETURN;
    }

    FORTHOP(oListLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oListStruct, pList);
        oListElement* pCur = pList->head;
        while (pCur != NULL)
        {
            oListElement* pNext = pCur->next;
            SAFE_RELEASE(pCore, pCur->obj);
            FREE_LINK(pCur);
            pCur = pNext;
        }
        pList->head = NULL;
        pList->tail = NULL;

        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            MALLOCATE_LINK(oListElement, newElem);
            POP_OBJECT(newElem->obj);
            SAFE_KEEP(newElem->obj);
            newElem->next = NULL;
            oListElement* oldTail = pList->tail;
            if (oldTail == NULL)
            {
                ASSERT(pList->head == NULL);
                pList->head = newElem;
            }
            else
            {
                ASSERT(oldTail->next == NULL);
                oldTail->next = newElem;
            }
            newElem->prev = oldTail;
            pList->tail = newElem;
        }
        METHOD_RETURN;
    }

    FORTHOP(oListToArrayMethod)
    {
        GET_THIS(oListStruct, pList);
        oListElement* pCur = pList->head;
        ForthClassVocabulary *pArrayVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIArray);
        MALLOCATE_OBJECT(oArrayStruct, pArray, pArrayVocab);
        pArray->pMethods = pArrayVocab->GetMethods();
        pArray->refCount = 0;
        pArray->elements = new oArray;

        while (pCur != NULL)
        {
            ForthObject& o = pCur->obj;
            SAFE_KEEP(o);
            pArray->elements->push_back(o);
            pCur = pCur->next;
        }

        // push array on TOS
        PUSH_OBJECT(pArray);
        METHOD_RETURN;
    }

    FORTHOP(oListIsEmptyMethod)
	{
		GET_THIS(oListStruct, pList);
		int result = 0;
		if (pList->head == NULL)
		{
			ASSERT(pList->tail == NULL);
			result = ~0;
		}
		SPUSH(result);
		METHOD_RETURN;
	}

	FORTHOP(oListHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		if (pList->head == NULL)
		{
			ASSERT(pList->tail == NULL);
			PUSH_OBJECT(nullptr);
		}
		else
		{
			PUSH_OBJECT(pList->head->obj);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListTailMethod)
	{
		GET_THIS(oListStruct, pList);
		if (pList->tail == NULL)
		{
			ASSERT(pList->head == NULL);
			PUSH_OBJECT(nullptr);
		}
		else
		{
			PUSH_OBJECT(pList->tail->obj);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListAddHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		MALLOCATE_LINK(oListElement, newElem);
		POP_OBJECT(newElem->obj);
		SAFE_KEEP(newElem->obj);
		newElem->prev = NULL;
		oListElement* oldHead = pList->head;
		if (oldHead == NULL)
		{
			ASSERT(pList->tail == NULL);
			pList->tail = newElem;
		}
		else
		{
			ASSERT(oldHead->prev == NULL);
			oldHead->prev = newElem;
		}
		newElem->next = oldHead;
		pList->head = newElem;
		METHOD_RETURN;
	}

    void listAddTail(oListStruct* pList, ForthObject& obj, ForthCoreState* pCore)
    {
        MALLOCATE_LINK(oListElement, newElem);
        newElem->obj = obj;
        SAFE_KEEP(obj);
        newElem->next = NULL;
        oListElement* oldTail = pList->tail;
        if (oldTail == NULL)
        {
            ASSERT(pList->head == NULL);
            pList->head = newElem;
        }
        else
        {
            ASSERT(oldTail->next == NULL);
            oldTail->next = newElem;
        }
        newElem->prev = oldTail;
        pList->tail = newElem;
    }

	FORTHOP(oListAddTailMethod)
	{
		GET_THIS(oListStruct, pList);
        ForthObject obj;
        POP_OBJECT(obj);
        listAddTail(pList, obj, pCore);
		METHOD_RETURN;
	}

	FORTHOP(oListRemoveHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldHead = pList->head;
		if (oldHead != NULL)
		{
			ForthObject& obj = oldHead->obj;
			if (oldHead == pList->tail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldHead->prev == NULL);
				oListElement* newHead = oldHead->next;
				ASSERT(newHead != NULL);
				newHead->prev = NULL;
				pList->head = newHead;
			}
			SAFE_RELEASE(pCore, obj);
			FREE_LINK(oldHead);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListRemoveTailMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldTail = pList->tail;
		if (oldTail != NULL)
		{
			ForthObject& obj = oldTail->obj;
			if (pList->head == oldTail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oListElement* newTail = oldTail->prev;
				ASSERT(newTail != NULL);
				newTail->next = NULL;
				pList->tail = newTail;
			}
			SAFE_RELEASE(pCore, obj);
			FREE_LINK(oldTail);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListUnrefHeadMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldHead = pList->head;
		if (oldHead == NULL)
		{
			ASSERT(pList->tail == NULL);
			PUSH_OBJECT(nullptr);
		}
		else
		{
			ForthObject& obj = oldHead->obj;
			if (oldHead == pList->tail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldHead->prev == NULL);
				oListElement* newHead = oldHead->next;
				ASSERT(newHead != NULL);
				newHead->prev = NULL;
				pList->head = newHead;
			}
			unrefObject(obj);
			PUSH_OBJECT(obj);
			FREE_LINK(oldHead);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListUnrefTailMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* oldTail = pList->tail;
		if (oldTail == NULL)
		{
			ASSERT(pList->head == NULL);
			PUSH_OBJECT(nullptr);
		}
		else
		{
			ForthObject& obj = oldTail->obj;
			if (pList->head == oldTail)
			{
				// removed only element in list
				pList->head = NULL;
				pList->tail = NULL;
			}
			else
			{
				ASSERT(oldTail->next == NULL);
				oListElement* newTail = oldTail->prev;
				ASSERT(newTail != NULL);
				newTail->next = NULL;
				pList->tail = newTail;
			}
			unrefObject(obj);
			PUSH_OBJECT(obj);
			FREE_LINK(oldTail);
		}
		METHOD_RETURN;
	}

	/*


	Cut( oListIter start, oListIter end )
	remove a sublist from this list
	end is one past last element to delete, end==NULL deletes to end of list
	Splice( oList soList, listElement* insertBefore
	insert soList into this list before position specified by insertBefore
	if insertBefore is null, insert at tail of list
	after splice, soList is empty

	? replace an elements object
	? isEmpty
	*/

	FORTHOP(oListRemoveMethod)
	{
		GET_THIS(oListStruct, pList);
		oListElement* pCur = pList->head;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
				break;
			}
			pCur = pCur->next;
		}
		if (pCur != NULL)
		{
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	baseMethodEntry oListMembers[] =
	{
		METHOD("__newOp", oListNew),
		METHOD("delete", oListDeleteMethod),
		METHOD("showInner", oListShowInnerMethod),

		METHOD_RET("headIter", oListHeadIterMethod, RETURNS_OBJECT(kBCIListIter)),
		METHOD_RET("tailIter", oListTailIterMethod, RETURNS_OBJECT(kBCIListIter)),
		METHOD_RET("find", oListFindMethod, RETURNS_OBJECT(kBCIListIter)),
		METHOD_RET("clone", oListCloneMethod, RETURNS_OBJECT(kBCIList)),
		METHOD_RET("count", oListCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oListClearMethod),

		METHOD("load", oListLoadMethod),
		METHOD_RET("toArray", oListToArrayMethod, RETURNS_OBJECT(kBCIArray)),
		METHOD_RET("isEmpty", oListIsEmptyMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("head", oListHeadMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD_RET("tail", oListTailMethod, RETURNS_OBJECT(kBCIContainedType)),
		METHOD("addHead", oListAddHeadMethod),
		METHOD("addTail", oListAddTailMethod),
		METHOD("removeHead", oListRemoveHeadMethod),
		METHOD("removeTail", oListRemoveTailMethod),
		METHOD("unrefHead", oListUnrefHeadMethod),
		METHOD("unrefTail", oListUnrefTailMethod),
		METHOD("remove", oListRemoveMethod),

		MEMBER_VAR("__head", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		MEMBER_VAR("__tail", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 ListIter
	//

	FORTHOP(oListIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a ListIter object");
	}

	FORTHOP(oListIterDeleteMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oListIterShowInnerMethod)
	{
		GET_THIS(oListIterStruct, pIter);
        char buffer[32];
		ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oListElement* pCur = reinterpret_cast<oListStruct *>(pIter->parent)->head;
        int cursor = 0;
        while (pCur != NULL)
        {
            if (pCur == pIter->cursor)
            {
                break;
            }
            cursor++;
            pCur = pCur->next;
        }

        pShowContext->BeginElement("cursor");
        sprintf(buffer, "%d", cursor);
        pShowContext->ShowText(buffer);
        pShowContext->EndElement();
        pShowContext->BeginElement("parent");
        ForthShowObject(pIter->parent, pCore);
		pShowContext->EndElement();
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor != NULL)
		{
			pIter->cursor = pIter->cursor->next;
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekPrevMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor != NULL)
		{
			pIter->cursor = pIter->cursor->prev;
		}
		else
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent)->tail;
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekHeadMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent)->head;
		METHOD_RETURN;
	}

	FORTHOP(oListIterSeekTailMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		pIter->cursor = NULL;
		METHOD_RETURN;
	}

    FORTHOP(oListIterAtHeadMethod)
    {
        GET_THIS(oListIterStruct, pIter);
        long retVal = (pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent)->head) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oListIterAtTailMethod)
    {
        GET_THIS(oListIterStruct, pIter);
        long retVal = (pIter->cursor == nullptr) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

	FORTHOP(oListIterNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			pIter->cursor = pIter->cursor->next;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterPrevMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		// special case: NULL cursor means tail of list
		if (pIter->cursor == NULL)
		{
			pIter->cursor = reinterpret_cast<oListStruct *>(pIter->parent)->tail;
		}
		else
		{
			pIter->cursor = pIter->cursor->prev;
		}
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterCurrentMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		if (pIter->cursor == NULL)
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(pIter->cursor->obj);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterRemoveMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCur = pIter->cursor;
		if (pCur != NULL)
		{
			oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent);
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			pIter->cursor = pNext;
			SAFE_RELEASE(pCore, pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterUnrefMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCur = pIter->cursor;
		if (pCur != NULL)
		{
			oListStruct* pList = reinterpret_cast<oListStruct *>(pIter->parent);
			oListElement* pPrev = pCur->prev;
			oListElement* pNext = pCur->next;
			if (pCur == pList->head)
			{
				pList->head = pNext;
			}
			if (pCur == pList->tail)
			{
				pList->tail = pPrev;
			}
			if (pNext != NULL)
			{
				pNext->prev = pPrev;
			}
			if (pPrev != NULL)
			{
				pPrev->next = pNext;
			}
			pIter->cursor = pNext;
			PUSH_OBJECT(pCur->obj);
			unrefObject(pCur->obj);
			FREE_LINK(pCur);
		}
		METHOD_RETURN;
	}

	FORTHOP(oListIterFindNextMethod)
	{
		GET_THIS(oListIterStruct, pIter);
		oListElement* pCur = pIter->cursor;
        long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		if (pCur != NULL)
		{
			pCur = pCur->next;
		}
		while (pCur != NULL)
		{
			ForthObject& o = pCur->obj;
			if (OBJECTS_SAME(o, soughtObj))
			{
                pIter->cursor = pCur;
                found = ~0;
                break;
			}
			pCur = pCur->next;
		}
		SPUSH(found);
		METHOD_RETURN;
	}

    FORTHOP(oListIterSwapNextMethod)
    {
        GET_THIS(oListIterStruct, pIter);
        oListElement* pCursor = pIter->cursor;
        if (pCursor != NULL)
        {
            oListElement* pNext = pCursor->next;
            if (pNext != NULL)
            {
                ForthObject nextObj = pNext->obj;
                ForthObject obj = pCursor->obj;
                pCursor->obj = nextObj;
                pNext->obj = obj;
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oListIterSwapPrevMethod)
    {
        GET_THIS(oListIterStruct, pIter);
        oListElement* pCursor = pIter->cursor;
        if (pCursor != NULL)
        {
            oListElement* pPrev = pCursor->prev;
            if (pPrev != NULL)
            {
                ForthObject prevObj = pPrev->obj;
                ForthObject obj = pCursor->obj;
                pCursor->obj = prevObj;
                pPrev->obj = obj;
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oListIterSplitMethod)
	{
		GET_THIS(oListIterStruct, pIter);

		// create an empty list
		ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIList);
		MALLOCATE_OBJECT(oListStruct, pNewList, pClassVocab);
        pNewList->pMethods = pClassVocab->GetMethods();
        pNewList->refCount = 0;
        pNewList->head = NULL;
		pNewList->tail = NULL;

		oListElement* pCursor = pIter->cursor;
		oListStruct* pOldList = reinterpret_cast<oListStruct *>(pIter->parent);
		// if pCursor is NULL, iter cursor is past tail, new list is just empty list, leave old list alone
		if (pCursor != NULL)
		{
			if (pCursor == pOldList->head)
			{
				// iter cursor is start of list, make old list empty, new list is entire old list
				pNewList->head = pOldList->head;
				pNewList->tail = pOldList->tail;
				pOldList->head = NULL;
				pOldList->tail = NULL;
			}
			else
			{
				// head of new list is iter cursor
				pNewList->head = pCursor;
				pNewList->tail = pOldList->tail;
				// fix old list tail
				pOldList->tail = pNewList->head->prev;
				pOldList->tail->next = NULL;
			}
		}

		// split leaves iter cursor past tail
		pIter->cursor = NULL;

		PUSH_OBJECT(pNewList);
		METHOD_RETURN;
	}

	baseMethodEntry oListIterMembers[] =
	{
		METHOD("__newOp", oListIterNew),
        METHOD("delete", oListIterDeleteMethod),
        METHOD("showInner", oListIterShowInnerMethod),

		METHOD("seekNext", oListIterSeekNextMethod),
		METHOD("seekPrev", oListIterSeekPrevMethod),
		METHOD("seekHead", oListIterSeekHeadMethod),
		METHOD("seekTail", oListIterSeekTailMethod),
        METHOD_RET("atHead", oListIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oListIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oListIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("prev", oListIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("current", oListIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oListIterRemoveMethod),
		METHOD("unref", oListIterUnrefMethod),
        //METHOD_RET( "clone",                oListIterCloneMethod, RETURNS_OBJECT(kBCIListIter) ),

        METHOD_RET("findNext", oListIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("swapNext", oListIterSwapNextMethod),
		METHOD("swapPrev", oListIterSwapPrevMethod),
		METHOD("split", oListIterSplitMethod),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIList)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

    bool customListReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oListStruct *dstList = (oListStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            ForthObject obj;
            while (true)
            {
                char ch = reader->getChar();
                if (ch == ']')
                {
                    break;
                }
                if (ch != ',')
                {
                    reader->ungetChar(ch);
                }
                reader->getObjectOrLink(&obj);
                listAddTail(dstList, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }


	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pListVoc = pEngine->AddBuiltinClass("List", kBCIList, kBCIIterable, oListMembers);
        pListVoc->SetCustomObjectReader(customListReader);

		pEngine->AddBuiltinClass("ListIter", kBCIListIter, kBCIIter, oListIterMembers);
	}

} // namespace OList
