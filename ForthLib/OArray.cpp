//////////////////////////////////////////////////////////////////////
//
// OArray.cpp: builtin array related classes
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>

#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthObject.h"
#include "ForthBuiltinClasses.h"
#include "ForthShowContext.h"
#include "ForthObjectReader.h"

#include "OArray.h"
#include "OList.h"
#include "OMap.h"

static void ReportBadArrayIndex(const char* pWhere, int ix, int arraySize)
{
	char buff[64];
#if defined(WIN32)
    sprintf_s(buff, sizeof(buff), " in %s index:%d size:%d", pWhere, ix, arraySize);
#elif defined(LINUX) || defined(MACOSX)
    snprintf(buff, sizeof(buff), " in %s index:%d size:%d", pWhere, ix, arraySize);
#endif
	ForthEngine::GetInstance()->SetError(kForthErrorBadArrayIndex, buff);
}

namespace OArray
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 Array
	//

    ForthClassVocabulary* gpArrayClassVocab = nullptr;

    oArrayStruct* createArrayObject(ForthClassVocabulary *pClassVocab)
    {

        MALLOCATE_OBJECT(oArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
        pArray->elements = new oArray;
        return pArray;
    }

    oArrayIterStruct* createArrayIterator(ForthCoreState* pCore, oArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIArrayIter);
        MALLOCATE_ITER(oArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oArrayStruct *dstArray = (oArrayStruct *)(reader->getCustomReaderContext().pData);
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
                SAFE_KEEP(obj);
                dstArray->elements->push_back(obj);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    FORTHOP(oArrayNew)
	{
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        oArrayStruct* newArray = createArrayObject(pClassVocab);
        PUSH_OBJECT(newArray);
    }

	FORTHOP(oArrayDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
			SAFE_RELEASE(pCore, o);
		}
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oArrayShowInnerMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = *iter;
            pShowContext->BeginArrayElement(1);
            ForthShowObject(o, pCore);
        }
        pShowContext->EndArray();
		METHOD_RETURN;
	}

    FORTHOP(oArrayHeadIterMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oArrayIterStruct* pIter = createArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oArrayTailIterMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oArrayIterStruct* pIter = createArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oArrayFindMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        long found = 0;
        ForthObject soughtObj;
        POP_OBJECT(soughtObj);
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            ForthObject& o = a[i];
            if (OBJECTS_SAME(o, soughtObj))
            {
                pArray->refCount++;
                TRACK_KEEP;

                oArrayIterStruct* pIter = createArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }

        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oArrayCloneMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        // bump reference counts of all valid elements in this array
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = *iter;
            SAFE_KEEP(o);
        }
        // create clone array and set is size to match this array
        ForthClassVocabulary *pArrayVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIArray);
        MALLOCATE_OBJECT(oArrayStruct, pCloneArray, pArrayVocab);
        pCloneArray->pMethods = pArray->pMethods;
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oArray;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneArray->elements->resize(numElements);
            // copy this array contents to clone array
            oArray& cloneElements = *(pCloneArray->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements << 3);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneArray);
        METHOD_RETURN;
    }

    FORTHOP(oArrayCountMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        SPUSH((long)(pArray->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oArrayClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oArrayStruct, pArray);
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = *iter;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oArrayGetMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            PUSH_OBJECT(a[ix]);
        }
        else
        {
            ReportBadArrayIndex("Array:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArraySetMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            ForthObject& oldObj = a[ix];
            ForthObject newObj;
            POP_OBJECT(newObj);
            OBJECT_ASSIGN(pCore, oldObj, newObj);
            a[ix] = newObj;
        }
        else
        {
            ReportBadArrayIndex("Array:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayRefMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("Array:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArraySwapMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            ForthObject t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("Array:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayResizeMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		if (a.size() != newSize)
		{
			if (a.size() > newSize)
			{
				// shrinking - release elements which are beyond new end of array
				for (ulong i = newSize; i < a.size(); i++)
				{
					SAFE_RELEASE(pCore, a[i]);
				}
				a.resize(newSize);
			}
			else
			{
				// growing - add null objects to end of array
				a.resize(newSize);
				for (ulong i = a.size(); i < newSize; i++)
				{
					a[i] = nullptr;
				}
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oArrayInsertMethod)
    {
        GET_THIS(oArrayStruct, pArray);

        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            ForthObject newObj;
            POP_OBJECT(newObj);
            // add dummy object to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
            }
            a[ix] = newObj;
            SAFE_KEEP(newObj);
        }
        else
        {
            ReportBadArrayIndex("Array:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayRemoveMethod)
    {
        GET_THIS(oArrayStruct, pArray);

        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (ix < a.size())
        {
            ForthObject fobj = a[ix];
            unrefObject(fobj);
            PUSH_OBJECT(fobj);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("Array:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayPushMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ForthObject fobj;
        POP_OBJECT(fobj);
        SAFE_KEEP(fobj);
        a.push_back(fobj);
        METHOD_RETURN;
    }

    FORTHOP(oArrayPopMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        if (a.size() > 0)
        {
            ForthObject fobj = a.back();
            a.pop_back();
            unrefObject(fobj);
            PUSH_OBJECT(fobj);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty OArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayBaseMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        SPUSH(a.size() > 0 ? (long)&(a[0]) : NULL);
        METHOD_RETURN;
    }

    FORTHOP(oArrayLoadMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		if (a.size() != newSize)
		{
			if (a.size() > newSize)
			{
				// shrinking - release elements which are beyond new end of array
				for (ulong i = newSize; i < a.size(); i++)
				{
					SAFE_RELEASE(pCore, a[i]);
				}
				a.resize(newSize);
			}
			else
			{
				// growing - add null objects to end of array
				a.resize(newSize);
				for (ulong i = a.size(); i < newSize; i++)
				{
					a[i] = nullptr;
				}
			}
		}
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				ForthObject& oldObj = a[i];
				ForthObject newObj;
				POP_OBJECT(newObj);
				if (OBJECTS_DIFFERENT(oldObj, newObj))
				{
					SAFE_KEEP(newObj);
					SAFE_RELEASE(pCore, oldObj);
				}
				a[i] = newObj;
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oArrayFromMemoryMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong offset = SPOP;
        ulong numObjects = SPOP;
        ForthObject* pSrc = (ForthObject*)(SPOP);
        ulong newSize = (ulong)(numObjects + offset);
        ulong oldSize = a.size();

        // NOTE: overlapping source and dest will result in undefined behavior

        // increase refcount on all source objects
        for (ulong i = 0; i < numObjects; i++)
        {
            ForthObject& srcObj = pSrc[i];
            SAFE_KEEP(srcObj);
        }

        // decrease refcount on all dest objects
        for (ulong i = offset; i < oldSize; i++)
        {
            ForthObject& dstObj = a[i];
            SAFE_RELEASE(pCore, dstObj);
        }
        
        // resize dest array
        if (newSize != oldSize)
        {
            a.resize(newSize);
        }

        // copy objects
        ForthObject* pDst = &(a[offset]);
        memcpy(pDst, pSrc, numObjects << 3);
        METHOD_RETURN;
    }

    FORTHOP(oArrayFindValueMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        long found = 0;
        ForthObject soughtObj;
        POP_OBJECT(soughtObj);
        oArray::iterator iter;
        oArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            ForthObject& o = a[i];
            if (OBJECTS_SAME(o, soughtObj))
            {
                found = ~0;
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

	FORTHOP(oArrayReverseMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray& a = *(pArray->elements);
        std::reverse(a.begin(), a.end());
		METHOD_RETURN;
	}

    int objectArrayQuicksortPartition(ForthCoreState* pCore, ForthObject* pData, int left, int right)
	{
		ForthObject* pLeft = pData + left;
		ForthObject* pRight = pData + right;
		ForthObject pivot = *pLeft;
		ForthObject tmp;
		int compareResult;
		ForthEngine* pEngine = ForthEngine::GetInstance();

		left--;
		pLeft--;
		right++;
		pRight++;
		while (true)
		{
			do
			{
				left++;
				pLeft++;
				PUSH_OBJECT(pivot);
				pEngine->FullyExecuteMethod(pCore, *pLeft, kMethodCompare);
				compareResult = SPOP;
			} while (compareResult < 0);

			do
			{
				right--;
				pRight--;
				PUSH_OBJECT(pivot);
				pEngine->FullyExecuteMethod(pCore, *pRight, kMethodCompare);
				compareResult = SPOP;
			} while (compareResult > 0);

			if (left >= right)
			{
				return right;
			}

			// swap(a[left], a[right]);
			tmp = *pLeft;
			*pLeft = *pRight;
			*pRight = tmp;
		}
	}

	void objectArrayQuicksortStep(ForthCoreState* pCore, ForthObject* pData, int left, int right)
	{
		if (left < right)
		{
			int pivot = objectArrayQuicksortPartition(pCore, pData, left, right);
			objectArrayQuicksortStep(pCore, pData, left, pivot);
			objectArrayQuicksortStep(pCore, pData, pivot + 1, right);
		}
	}

	FORTHOP(oArraySortMethod)
	{
		GET_THIS(oArrayStruct, pArray);
		oArray::iterator iter;
		oArray& a = *(pArray->elements);
		if (a.size() > 1)
		{
			objectArrayQuicksortStep(pCore, &(a[0]), 0, a.size() - 1);
		}
		METHOD_RETURN;
	}

    FORTHOP(oArrayToListMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray::iterator iter;
        oArray& a = *(pArray->elements);

        ForthClassVocabulary *pListVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIList);
        MALLOCATE_OBJECT(oListStruct, pList, pListVocab);
        pList->pMethods = pListVocab->GetMethods();
        pList->refCount = 0;
        pList->head = NULL;
        pList->tail = NULL;
        oListElement* oldTail = NULL;

        // bump reference counts of all valid elements in this array
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = *iter;
            SAFE_KEEP(o);
            MALLOCATE_LINK(oListElement, newElem);
            newElem->obj = o;
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
            newElem->next = NULL;
            pList->tail = newElem;
            oldTail = newElem;
        }

        // push list on TOS
        PUSH_OBJECT(pList);
        METHOD_RETURN;
    }

    FORTHOP(oArrayUnrefMethod)
    {
        GET_THIS(oArrayStruct, pArray);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            ForthObject fobj = a[ix];
            unrefObject(fobj);
            PUSH_OBJECT(fobj);
            a[ix] = nullptr;
        }
        else
        {
            ReportBadArrayIndex("Array:unref", ix, a.size());
        }
        METHOD_RETURN;
    }

    baseMethodEntry oArrayMembers[] =
	{
		METHOD("__newOp", oArrayNew),
		METHOD("delete", oArrayDeleteMethod),
		METHOD("showInner", oArrayShowInnerMethod),

        METHOD_RET("headIter", oArrayHeadIterMethod, RETURNS_OBJECT(kBCIArrayIter)),
        METHOD_RET("tailIter", oArrayTailIterMethod, RETURNS_OBJECT(kBCIArrayIter)),
        METHOD_RET("find", oArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oArrayCloneMethod, RETURNS_OBJECT(kBCIArray)),
        METHOD_RET("count", oArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oArrayClearMethod),

        METHOD_RET("get", oArrayGetMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("set", oArraySetMethod),
        METHOD_RET("ref", oArrayRefMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod | kDTIsPtr, kBCIObject)),
        METHOD("swap", oArraySwapMethod),
        METHOD("resize", oArrayResizeMethod),
        METHOD("insert", oArrayInsertMethod),
        METHOD("remove", oArrayRemoveMethod),
        METHOD("push", oArrayPushMethod),
        METHOD("pop", oArrayPopMethod),
        METHOD_RET("base", oArrayBaseMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod | kDTIsPtr, kBCIObject)),
        METHOD("load", oArrayLoadMethod),
        METHOD("fromMemory", oArrayFromMemoryMethod),
        METHOD_RET("findValue", oArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oArrayReverseMethod),
        METHOD("sort", oArraySortMethod),
        METHOD_RET("toList", oArrayToListMethod, RETURNS_OBJECT(kBCIList)),
        METHOD("unref", oArrayUnrefMethod),
       
        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 ArrayIter
	//

	FORTHOP(oArrayIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create an ArrayIter object");
	}

	FORTHOP(oArrayIterDeleteMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekNextMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekPrevMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekHeadMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterSeekTailMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

    FORTHOP(oArrayIterAtHeadMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oArrayIterAtTailMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oArrayIterNextMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(a[pIter->cursor]);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterPrevMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			PUSH_OBJECT(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterCurrentMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			PUSH_OBJECT(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oArrayIterRemoveMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			// TODO!
			ForthObject o = a[pIter->cursor];
			SAFE_RELEASE(pCore, o);
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		else
		{
			ReportBadArrayIndex("ArrayIter:remove", pIter->cursor, a.size());
		}
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterUnrefMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		ForthObject fobj = nullptr;
		if (pIter->cursor < a.size())
		{
			fobj = a[pIter->cursor];
			unrefObject(fobj);
            a[pIter->cursor] = nullptr;
		}
		PUSH_OBJECT(fobj);
		METHOD_RETURN;
	}

	FORTHOP(oArrayIterCloneMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);
		ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIArrayIter);
		MALLOCATE_ITER(oArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIterVocab->GetMethods();
		pNewIter->refCount = 0;
        pNewIter->parent = pIter->parent;
		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		pArray->refCount++;
		TRACK_KEEP;
		pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
		METHOD_RETURN;
	}

    FORTHOP(oArrayIterSeekMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);

        oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
        oArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("ArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oArrayIterTellMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    FORTHOP(oArrayIterFindNextMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        long retVal = 0;
        ForthObject soughtObj;
        POP_OBJECT(soughtObj);
        oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
        oArray& a = *(pArray->elements);
        unsigned int i = pIter->cursor;
        while (i < a.size())
        {
            ForthObject& o = a[i];
            if (OBJECTS_SAME(o, soughtObj))
            {
                retVal = ~0;
                pIter->cursor = i;
                break;
            }
            ++i;
        }
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oArrayIterInsertMethod)
	{
		GET_THIS(oArrayIterStruct, pIter);

		oArrayStruct* pArray = reinterpret_cast<oArrayStruct *>(pIter->parent);
		oArray& a = *(pArray->elements);
		ulong ix = pIter->cursor;
		ulong oldSize = a.size();
		if (oldSize >= ix)
		{
			ForthObject newObj;
			POP_OBJECT(newObj);
			// add dummy object to end of array
			a.resize(oldSize + 1);
			if ((oldSize > 0) && (ix < oldSize))
			{
				// move old entries up by size of ForthObject
				memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
			}
			a[ix] = newObj;
			SAFE_KEEP(newObj);
		}
		else
		{
			ReportBadArrayIndex("ArrayIter:insert", ix, a.size());
		}
		METHOD_RETURN;
	}

	baseMethodEntry oArrayIterMembers[] =
	{
		METHOD("__newOp", oArrayIterNew),
		METHOD("delete", oArrayIterDeleteMethod),

		METHOD("seekNext", oArrayIterSeekNextMethod),
		METHOD("seekPrev", oArrayIterSeekPrevMethod),
		METHOD("seekHead", oArrayIterSeekHeadMethod),
        METHOD("seekTail", oArrayIterSeekTailMethod),
        METHOD_RET("atHead", oArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("current", oArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oArrayIterRemoveMethod),
		METHOD("unref", oArrayIterUnrefMethod),
        METHOD_RET("clone", oArrayIterCloneMethod, RETURNS_OBJECT(kBCIArrayIter)),

        METHOD("seek", oArrayIterSeekMethod),
        METHOD_RET("tell", oArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oArrayIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("insert", oArrayIterInsertMethod),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIArray)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


    struct bagElement
    {
        stackInt64   tag;
        ForthObject obj;
    };

    typedef std::vector<bagElement> oBag;
    struct oBagStruct
    {
        long*    pMethods;
        ulong    refCount;
        oBag*    elements;
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 Bag
    //

    ForthClassVocabulary* gpBagClassVocab = nullptr;

    oBagStruct* createBagObject(ForthObject& destObj, ForthClassVocabulary *pClassVocab)
    {

        MALLOCATE_OBJECT(oBagStruct, pBag, pClassVocab);
        pBag->pMethods = pClassVocab->GetMethods();
        pBag->refCount = 0;
        pBag->elements = new oBag;
        return pBag;
    }

    oArrayIterStruct* createBagIterator(ForthCoreState* pCore, oBagStruct* pBag)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIBagIter);
        MALLOCATE_ITER(oArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pBag);
        return pIter;
    }

    bool customBagReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oBagStruct *dstBag = (oBagStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string tag;
            bagElement newElement;
            int tagParts[3];

            while (true)
            {
                char ch = reader->getChar();
                if (ch == '}')
                {
                    break;
                }
                if (ch != ',')
                {
                    reader->ungetChar(ch);
                }
                reader->getName(tag);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&newElement.obj);
                tagParts[0] = 0;
                tagParts[1] = 0;
                tagParts[2] = 0;
                strcpy((char *)(&tagParts[0]), tag.c_str());
                newElement.tag.s64 = *((long long *)(&tagParts[0]));
                SAFE_KEEP(newElement.obj);
                dstBag->elements->push_back(newElement);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    FORTHOP(oBagNew)
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject newBag;
        createBagObject(newBag, pClassVocab);
        PUSH_OBJECT(newBag);
    }

    FORTHOP(oBagDeleteMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oBagStruct, pBag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            bagElement& element = *iter;
            SAFE_RELEASE(pCore, element.obj);
        }
        delete pBag->elements;
        FREE_OBJECT(pBag);
        METHOD_RETURN;
    }

    FORTHOP(oBagShowInnerMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        char tag[12] = "12345678\0";
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("elements");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            // TODO: show tag
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                bagElement& element = *iter;
                ForthObject& o = element.obj;
                *((long long *)&tag[0]) = element.tag.s64;
                pShowContext->BeginElement(tag);
                ForthShowObject(o, pCore);
                pShowContext->EndElement();
            }
            pShowContext->ShowTextReturn();
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->EndNestedShow();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oBagHeadIterMethod)
    {
        GET_THIS(oBagStruct, pBag);
        pBag->refCount++;
        TRACK_KEEP;

        oArrayIterStruct* pIter = createBagIterator(pCore, pBag);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oBagTailIterMethod)
    {
        GET_THIS(oBagStruct, pBag);
        pBag->refCount++;
        TRACK_KEEP;

        oArrayIterStruct* pIter = createBagIterator(pCore, pBag);
        pIter->cursor = pBag->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oBagFindMethod)
    {
        GET_THIS(oBagStruct, pBag);
        long found = 0;
        stackInt64 soughtTag;
        LPOP(soughtTag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (a[i].tag.s64 == soughtTag.s64)
            {
                found = ~0;
                pBag->refCount++;
                TRACK_KEEP;
                ForthObject obj;

                oArrayIterStruct* pIter = createBagIterator(pCore, pBag);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                break;
            }
        }

        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oBagCloneMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        // bump reference counts of all valid elements in this array
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            bagElement& element = *iter;
            SAFE_KEEP(element.obj);
        }
        // create clone array and set is size to match this array
        ForthClassVocabulary *pBagVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIBag);
        MALLOCATE_OBJECT(oBagStruct, pCloneBag, pBagVocab);
        pCloneBag->pMethods = pBag->pMethods;
        pCloneBag->refCount = 0;
        pCloneBag->elements = new oBag;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneBag->elements->resize(numElements);
            // copy this array contents to clone array
            oBag& cloneElements = *(pCloneBag->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements << 4);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneBag);
        METHOD_RETURN;
    }

    FORTHOP(oBagCountMethod)
    {
        GET_THIS(oBagStruct, pBag);
        SPUSH((long)(pBag->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oBagClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oBagStruct, pBag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            bagElement& element = *iter;
            SAFE_RELEASE(pCore, element.obj);
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oBagGetMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            bagElement& element = a[ix];
            PUSH_OBJECT(element.obj);
            LPUSH(element.tag);
        }
        else
        {
            ReportBadArrayIndex("Bag:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagGetObjectMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            bagElement& element = a[ix];
            PUSH_OBJECT(element.obj);
        }
        else
        {
            ReportBadArrayIndex("Bag:geto", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagGetTagMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            bagElement& element = a[ix];
            LPUSH(element.tag);
        }
        else
        {
            ReportBadArrayIndex("Bag:gett", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagSetMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            bagElement& element = a[ix];
            ForthObject& oldObj = element.obj;
            ForthObject newObj;
            LPOP(element.tag);
            POP_OBJECT(newObj);
            OBJECT_ASSIGN(pCore, oldObj, newObj);
            element.obj = newObj;
        }
        else
        {
            ReportBadArrayIndex("Bag:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagRefMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("Bag:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagSwapMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            bagElement t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("Bag:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagResizeMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong newSize = SPOP;
        if (a.size() != newSize)
        {
            if (a.size() > newSize)
            {
                // shrinking - release elements which are beyond new end of array
                for (ulong i = newSize; i < a.size(); i++)
                {
                    SAFE_RELEASE(pCore, a[i].obj);
                }
                a.resize(newSize);
            }
            else
            {
                // growing - add null elements to end of array
                a.resize(newSize);
                bagElement emptyElement;
                emptyElement.obj = nullptr;
                emptyElement.tag.s64 = 0L;
                for (ulong i = a.size(); i < newSize; i++)
                {
                    a[i] = emptyElement;
                }
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagInsertMethod)
    {
        GET_THIS(oBagStruct, pBag);

        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            bagElement newElement;
            LPOP(newElement.tag);
            POP_OBJECT(newElement.obj);
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(bagElement) * (oldSize - ix));
            }
            a[ix] = newElement;
            SAFE_KEEP(newElement.obj);
        }
        else
        {
            ReportBadArrayIndex("Bag:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagRemoveMethod)
    {
        GET_THIS(oBagStruct, pBag);

        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (ix < a.size())
        {
            bagElement oldElement = a[ix];
            unrefObject(oldElement.obj);
            PUSH_OBJECT(oldElement.obj);
            LPUSH(oldElement.tag);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("Bag:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagPushMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        bagElement newElement;
        LPOP(newElement.tag);
        POP_OBJECT(newElement.obj);
        SAFE_KEEP(newElement.obj);
        a.push_back(newElement);
        METHOD_RETURN;
    }

    FORTHOP(oBagPopMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        if (a.size() > 0)
        {
            bagElement& oldElement = a.back();
            unrefObject(oldElement.obj);
            PUSH_OBJECT(oldElement.obj);
            LPUSH(oldElement.tag);
            a.resize(a.size() - 1);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty OBag");
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagBaseMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        SPUSH(a.size() > 0 ? (long)&(a[0]) : NULL);
        METHOD_RETURN;
    }

    FORTHOP(oBagLoadMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong newSize = SPOP;
        if (a.size() != newSize)
        {
            if (a.size() > newSize)
            {
                // shrinking - release elements which are beyond new end of array
                for (ulong i = newSize; i < a.size(); i++)
                {
                    SAFE_RELEASE(pCore, a[i].obj);
                }
                a.resize(newSize);
            }
            else
            {
                // growing - add null elements to end of array
                a.resize(newSize);
                bagElement emptyElement;
                emptyElement.obj = nullptr;
                emptyElement.tag.s64 = 0L;
                for (ulong i = a.size(); i < newSize; i++)
                {
                    a[i] = emptyElement;
                }
            }
        }
        if (newSize > 0)
        {
            for (int i = newSize - 1; i >= 0; i--)
            {
                bagElement& oldElement = a[i];
                bagElement newElement;
                LPOP(newElement.tag);
                POP_OBJECT(newElement.obj);
                if (OBJECTS_DIFFERENT(oldElement.obj, newElement.obj))
                {
                    SAFE_KEEP(newElement.obj);
                    SAFE_RELEASE(pCore, oldElement.obj);
                }
                a[i] = newElement;
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagFromMemoryMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong offset = SPOP;
        ulong numElements = SPOP;
        bagElement* pSrc = (bagElement*)(SPOP);
        ulong newSize = (ulong)(numElements + offset);
        ulong oldSize = a.size();

        // NOTE: overlapping source and dest will result in undefined behavior

        // increase refcount on all source objects
        for (ulong i = 0; i < numElements; i++)
        {
            bagElement& srcElement = pSrc[i];
            SAFE_KEEP(srcElement.obj);
        }

        // decrease refcount on all dest objects
        for (ulong i = offset; i < oldSize; i++)
        {
            bagElement& dstElement = a[i];
            SAFE_RELEASE(pCore, dstElement.obj);
        }

        // resize dest array
        if (newSize != oldSize)
        {
            a.resize(newSize);
        }

        // copy objects
        bagElement* pDst = &(a[offset]);
        memcpy(pDst, pSrc, numElements << 4);
        METHOD_RETURN;
    }

    FORTHOP(oBagFindValueMethod)
    {
        GET_THIS(oBagStruct, pBag);
        long found = 0;
        stackInt64 soughtTag;
        LPOP(soughtTag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (a[i].tag.s64 == soughtTag.s64)
            {
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oBagGrabMethod)
    {
        GET_THIS(oBagStruct, pBag);
        long retVal = 0;
        stackInt64 soughtTag;
        LPOP(soughtTag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (a[i].tag.s64 == soughtTag.s64)
            {
                retVal = -1;
                PUSH_OBJECT(a[retVal].obj);
                break;
            }
        }
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oBagReverseMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        std::reverse(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oBagToLongMapMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag::iterator iter;
        oBag& a = *(pBag->elements);

        ForthObject newMapObject = (ForthObject)OMap::createLongMapObject(OMap::gpLongMapClassVocab);
        oLongMapStruct* pMap = (oLongMapStruct *)newMapObject;
        oLongMap& destMap = *(pMap->elements);

        // bump reference counts of all valid elements in this array
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            bagElement& element = *iter;
            SAFE_KEEP(element.obj);
            destMap[element.tag.s64] = element.obj;
        }

        // push new LongMap on TOS
        PUSH_OBJECT(newMapObject);
        METHOD_RETURN;
    }

    FORTHOP(oBagUnrefMethod)
    {
        GET_THIS(oBagStruct, pBag);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            bagElement element = a[ix];
            unrefObject(element.obj);
            PUSH_OBJECT(element.obj);
            LPUSH(element.tag);
            a[ix].obj = nullptr;
            a[ix].tag.s64 = 0L;
        }
        else
        {
            ReportBadArrayIndex("Bag:unref", ix, a.size());
        }
        METHOD_RETURN;
    }

    baseMethodEntry oBagMembers[] =
    {
        METHOD("__newOp", oBagNew),
        METHOD("delete", oBagDeleteMethod),
        METHOD("showInner", oBagShowInnerMethod),

        METHOD_RET("headIter", oBagHeadIterMethod, RETURNS_OBJECT(kBCIBagIter)),
        METHOD_RET("tailIter", oBagTailIterMethod, RETURNS_OBJECT(kBCIBagIter)),
        METHOD_RET("find", oBagFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oBagCloneMethod, RETURNS_OBJECT(kBCIBag)),
        METHOD_RET("count", oBagCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oBagClearMethod),

        METHOD_RET("get", oBagGetMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD_RET("geto", oBagGetObjectMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD_RET("gett", oBagGetTagMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD("set", oBagSetMethod),
        METHOD_RET("ref", oBagRefMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod | kDTIsPtr, kBCIObject)),
        METHOD("swap", oBagSwapMethod),
        METHOD("resize", oBagResizeMethod),
        METHOD("insert", oBagInsertMethod),
        // TODO: METHOD("remove", oBagRemoveMethod),
        METHOD("push", oBagPushMethod),
        METHOD("pop", oBagPopMethod),
        METHOD_RET("base", oBagBaseMethod, OBJECT_TYPE_TO_CODE(kDTIsMethod | kDTIsPtr, kBCIObject)),
        METHOD("load", oBagLoadMethod),
        METHOD("fromMemory", oBagFromMemoryMethod),
        METHOD_RET("findValue", oBagFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("grab", oBagGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oBagReverseMethod),
        METHOD_RET("toLongMap", oBagToLongMapMethod, RETURNS_OBJECT(kBCILongMap)),
        METHOD("unref", oBagUnrefMethod),

        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        // following must be last in table
        END_MEMBERS
    };

    //////////////////////////////////////////////////////////////////////
    ///
    //                 BagIter
    //

    FORTHOP(oBagIterNew)
    {
        GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a BagIter object");
    }

    FORTHOP(oBagIterDeleteMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        SAFE_RELEASE(pCore, pIter->parent);
        FREE_ITER(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterSeekNextMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        if (pIter->cursor < pBag->elements->size())
        {
            pIter->cursor++;
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterSeekPrevMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        if (pIter->cursor > 0)
        {
            pIter->cursor--;
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterSeekHeadMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP(oBagIterSeekTailMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        pIter->cursor = pBag->elements->size();
        METHOD_RETURN;
    }

    FORTHOP(oBagIterAtHeadMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterAtTailMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pBag->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterNextMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        if (pIter->cursor >= a.size())
        {
            SPUSH(0);
        }
        else
        {
            bagElement& element = a[pIter->cursor];
            LPUSH(element.tag);
            PUSH_OBJECT(element.obj);
            pIter->cursor++;
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterPrevMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        if (pIter->cursor == 0)
        {
            SPUSH(0);
        }
        else
        {
            pIter->cursor--;
            bagElement& element = a[pIter->cursor];
            LPUSH(element.tag);
            PUSH_OBJECT(element.obj);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterCurrentMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        if (pIter->cursor >= a.size())
        {
            SPUSH(0);
        }
        else
        {
            bagElement& element = a[pIter->cursor];
            LPUSH(element.tag);
            PUSH_OBJECT(element.obj);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterRemoveMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        if (pIter->cursor < a.size())
        {
            // TODO!
            bagElement& element = a[pIter->cursor];
            LPUSH(element.tag);
            PUSH_OBJECT(element.obj);
            SAFE_RELEASE(pCore, element.obj);
            pBag->elements->erase(pBag->elements->begin() + pIter->cursor);
        }
        else
        {
            ReportBadArrayIndex("BagIter:remove", pIter->cursor, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterUnrefMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        ForthObject fobj = nullptr;
        if (pIter->cursor < a.size())
        {
            ForthObject& obj = a[pIter->cursor].obj;
            fobj = obj;
            unrefObject(fobj);
            obj = nullptr;
        }
        PUSH_OBJECT(fobj);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterCloneMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIBagIter);
        MALLOCATE_ITER(oArrayIterStruct, pNewIter, pClassVocab);
        pNewIter->pMethods = pClassVocab->GetMethods();
        pNewIter->refCount = 0;
        pNewIter->parent = pIter->parent;
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        pBag->refCount++;
        TRACK_KEEP;
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterSeekMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);

        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("BagIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oBagIterFindNextMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        long retVal = 0;
        stackInt64 soughtTag;
        LPOP(soughtTag);
        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        unsigned int i = pIter->cursor;
        while (i < a.size())
        {
            if (a[i].tag.s64 == soughtTag.s64)
            {
                retVal = ~0;
                pIter->cursor = i;
                break;
            }
            ++i;
        }
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oBagIterInsertMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);

        oBagStruct* pBag = reinterpret_cast<oBagStruct *>(pIter->parent);
        oBag& a = *(pBag->elements);
        ulong ix = pIter->cursor;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            bagElement newElement;
            POP_OBJECT(newElement.obj);
            LPOP(newElement.tag);
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(ForthObject) * (oldSize - ix));
            }
            a[ix] = newElement;
            SAFE_KEEP(newElement.obj);
        }
        else
        {
            ReportBadArrayIndex("BagIter:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    baseMethodEntry oBagIterMembers[] =
    {
        METHOD("__newOp", oBagIterNew),
        METHOD("delete", oBagIterDeleteMethod),

        METHOD("seekNext", oBagIterSeekNextMethod),
        METHOD("seekPrev", oBagIterSeekPrevMethod),
        METHOD("seekHead", oBagIterSeekHeadMethod),
        METHOD("seekTail", oBagIterSeekTailMethod),
        METHOD_RET("atHead", oBagIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oBagIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oBagIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("prev", oBagIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("current", oBagIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oBagIterRemoveMethod),
        METHOD("unref", oBagIterUnrefMethod),
        METHOD_RET("clone", oBagIterCloneMethod, RETURNS_OBJECT(kBCIBagIter)),

        METHOD("seek", oBagIterSeekMethod),
        METHOD_RET("tell", oArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oBagIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("insert", oBagIterInsertMethod),

        MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIBag)),
        MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
	///
	//                 ByteArray
	//

	typedef std::vector<char> oByteArray;
	struct oByteArrayStruct
	{
        long*           pMethods;
        ulong           refCount;
		oByteArray*    elements;
	};

	struct oByteArrayIterStruct
	{
        long*           pMethods;
		ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};

    oByteArrayIterStruct* createByteArrayIterator(ForthCoreState* pCore, oByteArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIByteArrayIter);
        MALLOCATE_ITER(oByteArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customByteArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oByteArrayStruct *dstArray = (oByteArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                int value;
                if (sscanf(number.c_str(), "%d", &value) == 1)
                {
                    dstArray->elements->push_back((char)value);
                }
                else
                {
                    reader->throwError("error parsing byte value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oByteArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oByteArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
		pArray->refCount = 0;
		pArray->elements = new oByteArray;
		PUSH_OBJECT(pArray);
	}

	FORTHOP(oByteArrayDeleteMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayShowInnerMethod)
	{
		char buffer[8];
		GET_THIS(oByteArrayStruct, pArray);
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
		oByteArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
		for (unsigned int i = 0; i < a.size(); i++)
		{
            pShowContext->BeginArrayElement();
			sprintf(buffer, "%u", a[i] & 0xFF);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayHeadIterMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oByteArrayIterStruct* pIter = createByteArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayTailIterMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;
        oByteArrayIterStruct* pIter = createByteArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayFindMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        long found = 0;
        char soughtByte = (char)(SPOP);
        oByteArray::iterator iter;
        oByteArray& a = *(pArray->elements);
        for (unsigned int i = 0; i < a.size(); i++)
        {
            if (soughtByte == a[i])
            {
                pArray->refCount++;
                TRACK_KEEP;
                oByteArrayIterStruct* pIter = createByteArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayCloneMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        // create clone array and set is size to match this array
        ForthClassVocabulary *pArrayVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIByteArray);
        MALLOCATE_OBJECT(oByteArrayStruct, pCloneArray, pArrayVocab);
        pCloneArray->pMethods = pArray->pMethods;
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oByteArray;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneArray->elements->resize(numElements);
            // copy this array contents to clone array
            oByteArray& cloneElements = *(pCloneArray->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneArray);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayCountMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        SPUSH((long)(pArray->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayGetMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)a[ix]);
        }
        else
        {
            ReportBadArrayIndex("ByteArray:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArraySetMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            a[ix] = (char)SPOP;
        }
        else
        {
            ReportBadArrayIndex("ByteArray:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayRefMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("ByteArray:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArraySwapMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            char t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("ByteArray:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayResizeMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add null bytes to end of array
			char* pElement = &(a[oldSize]);
			memset(pElement, 0, newSize - oldSize);
		}
		METHOD_RETURN;
	}

    FORTHOP(oByteArrayInsertMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);

        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            char insertedVal = (char)SPOP;
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(char) * (oldSize - ix));
            }
            a[ix] = insertedVal;
        }
        else
        {
            ReportBadArrayIndex("ByteArray:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayRemoveMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oByteArray& a = *(pArray->elements);
        if (ix < a.size())
        {
            SPUSH((long)a[ix]);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("ByteArray:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayPushMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        char val = (char)SPOP;
        a.push_back(val);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayPopMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        if (a.size() > 0)
        {
            char val = a.back();
            a.pop_back();
            SPUSH((long)val);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty oByteArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayBaseMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        SPUSH(a.size() > 0 ? (long)&(a[0]) : NULL);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayLoadMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = (char)c;
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oByteArrayFromMemoryMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        int offset = SPOP;
        int numBytes = SPOP;
        const char* pSrc = (const char *)(SPOP);
        ulong copyEnd = (ulong)(numBytes + offset);
        if (copyEnd != a.size())
        {
            a.resize(copyEnd);
        }
        char* pDst = &(a[0]) + offset;
        memcpy(pDst, pSrc, numBytes);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayFindValueMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        long found = 0;
        char val = (char)SPOP;
        oByteArray& a = *(pArray->elements);
        char* pElement = &(a[0]);
        char* pFound = (char *)memchr(pElement, val, a.size());
        if (pFound)
        {
            found = ~0;
            SPUSH(pFound - pElement);
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayReverseMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        std::reverse(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oByteArraySortMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        oByteArray& a = *(pArray->elements);
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayUnsignedSortMethod)
    {
        GET_THIS(oByteArrayStruct, pArray);
        std::vector<unsigned char> & a = *(((std::vector<unsigned char> *)(pArray->elements)));
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

	FORTHOP(oByteArrayFromStringMethod)
	{
		GET_THIS(oByteArrayStruct, pArray);
		oByteArray& a = *(pArray->elements);
		const char* pSrc = (const char *)(SPOP);
		int numBytes = ((int)strlen(pSrc)) + 1;
		a.resize(numBytes);
		memcpy(&(a[0]), pSrc, numBytes);
		METHOD_RETURN;
	}


	baseMethodEntry oByteArrayMembers[] =
	{
		METHOD("__newOp", oByteArrayNew),
		METHOD("delete", oByteArrayDeleteMethod),
		METHOD("showInner", oByteArrayShowInnerMethod),

        METHOD_RET("headIter", oByteArrayHeadIterMethod, RETURNS_OBJECT(kBCIByteArrayIter)),
        METHOD_RET("tailIter", oByteArrayTailIterMethod, RETURNS_OBJECT(kBCIByteArrayIter)),
        METHOD_RET("find", oByteArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oByteArrayCloneMethod, RETURNS_OBJECT(kBCIByteArray)),
        METHOD_RET("count", oByteArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oByteArrayClearMethod),

        METHOD_RET("get", oByteArrayGetMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD("set", oByteArraySetMethod),
        METHOD_RET("ref", oByteArrayRefMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("swap", oByteArraySwapMethod),
        METHOD("resize", oByteArrayResizeMethod),
        METHOD("insert", oByteArrayInsertMethod),
        METHOD_RET("remove", oByteArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD("push", oByteArrayPushMethod),
        METHOD_RET("pop", oByteArrayPopMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD_RET("base", oByteArrayBaseMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("load", oByteArrayLoadMethod),
        METHOD("fromMemory", oByteArrayFromMemoryMethod),
        METHOD_RET("findValue", oByteArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oByteArrayReverseMethod),
        METHOD("sort", oByteArraySortMethod),
        METHOD("usort", oByteArrayUnsignedSortMethod),
		METHOD("setFromString", oByteArrayFromStringMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 ByteArrayIter
	//

	FORTHOP(oByteArrayIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a ByteArrayIter object");
	}

	FORTHOP(oByteArrayIterDeleteMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterSeekPrevMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

    FORTHOP(oByteArrayIterAtHeadMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);
        oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterAtTailMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);
        oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterSeekHeadMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);
        pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterSeekTailMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			char val = a[pIter->cursor];
			SPUSH((long)val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterPrevMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			char val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterCurrentMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			char ch = a[pIter->cursor];
			SPUSH((long)ch);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oByteArrayIterRemoveMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		oByteArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

    FORTHOP(oByteArrayIterCloneMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);
        oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
        pArray->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIByteArrayIter);
        MALLOCATE_ITER(oByteArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIter->pMethods;
        pNewIter->refCount = 0;
        pNewIter->parent = pIter->parent;
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterSeekMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);

        oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
        oByteArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("ByteArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterTellMethod)
    {
        GET_THIS(oByteArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    FORTHOP(oByteArrayIterFindNextMethod)
	{
		GET_THIS(oByteArrayIterStruct, pIter);
		long retVal = 0;
		char soughtByte = (char)(SPOP);
		oByteArrayStruct* pArray = reinterpret_cast<oByteArrayStruct *>(pIter->parent);
		oByteArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtByte)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oByteArrayIterMembers[] =
	{
		METHOD("__newOp", oByteArrayIterNew),
		METHOD("delete", oByteArrayIterDeleteMethod),

		METHOD("seekNext", oByteArrayIterSeekNextMethod),
		METHOD("seekPrev", oByteArrayIterSeekPrevMethod),
		METHOD("seekHead", oByteArrayIterSeekHeadMethod),
		METHOD("seekTail", oByteArrayIterSeekTailMethod),
        METHOD_RET("atHead", oByteArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oByteArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oByteArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oByteArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oByteArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oByteArrayIterRemoveMethod),
		METHOD_RET("clone", oByteArrayIterCloneMethod, RETURNS_OBJECT(kBCIByteArrayIter)),

        METHOD("seek", oByteArrayIterSeekMethod),
        METHOD_RET("tell", oByteArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oByteArrayIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIByteArray)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};



	//////////////////////////////////////////////////////////////////////
	///
	//                 ShortArray
	//

	typedef std::vector<short> oShortArray;
	struct oShortArrayStruct
	{
        long*           pMethods;
        ulong           refCount;
		oShortArray*    elements;
	};

	struct oShortArrayIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};

    oShortArrayIterStruct* createShortArrayIterator(ForthCoreState* pCore, oShortArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIShortArrayIter);
        MALLOCATE_ITER(oShortArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = (ForthObject)(pArray);
        return pIter;
    }

    bool customShortArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oShortArrayStruct *dstArray = (oShortArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                int value;
                if (sscanf(number.c_str(), "%d", &value) == 1)
                {
                    dstArray->elements->push_back((short)value);
                }
                else
                {
                    reader->throwError("error parsing short value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oShortArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oShortArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
		pArray->elements = new oShortArray;
		PUSH_OBJECT(pArray);
	}

	FORTHOP(oShortArrayDeleteMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

    FORTHOP(oShortArrayShowInnerMethod)
    {
        char buffer[32];
        GET_THIS(oShortArrayStruct, pArray);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oShortArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
        for (unsigned int i = 0; i < a.size(); i++)
        {
            pShowContext->BeginArrayElement();
            sprintf(buffer, "%d", a[i]);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayHeadIterMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;
        oShortArrayIterStruct* pIter = createShortArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayTailIterMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oShortArrayIterStruct* pIter = createShortArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayFindMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        long found = 0;
        short soughtShort = (short)(SPOP);
        oShortArray::iterator iter;
        oShortArray& a = *(pArray->elements);
        for (unsigned int i = 0; i < a.size(); i++)
        {
            if (soughtShort == a[i])
            {
                oShortArrayIterStruct* pIter = createShortArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayCloneMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        // create clone array and set is size to match this array
        ForthClassVocabulary *pArrayVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIShortArray);
        MALLOCATE_OBJECT(oShortArrayStruct, pCloneArray, pArrayVocab);
        pCloneArray->pMethods = pArray->pMethods;
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oShortArray;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneArray->elements->resize(numElements);
            // copy this array contents to clone array
            oShortArray& cloneElements = *(pCloneArray->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements << 1);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneArray);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayCountMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        SPUSH((long)(pArray->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayGetMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)a[ix]);
        }
        else
        {
            ReportBadArrayIndex("ShortArray:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArraySetMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            a[ix] = (short)SPOP;
        }
        else
        {
            ReportBadArrayIndex("ShortArray:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayRefMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("ShortArray:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArraySwapMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            short t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("ShortArray:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayResizeMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add zeros to end of array
			short* pElement = &(a[oldSize]);
			memset(pElement, 0, ((newSize - oldSize) << 1));
		}
		METHOD_RETURN;
	}

    FORTHOP(oShortArrayInsertMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);

        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            short insertedVal = (short)SPOP;
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(short) * (oldSize - ix));
            }
            a[ix] = insertedVal;
        }
        else
        {
            ReportBadArrayIndex("ShortArray:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayRemoveMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oShortArray& a = *(pArray->elements);
        if (ix < a.size())
        {
            SPUSH((long)a[ix]);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("ShortArray:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayPushMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        short val = (short)SPOP;
        a.push_back(val);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayPopMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        if (a.size() > 0)
        {
            short val = a.back();
            a.pop_back();
            SPUSH((long)val);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty OShortArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayBaseMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        SPUSH((long)&(a[0]));
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayLoadMethod)
	{
		GET_THIS(oShortArrayStruct, pArray);
		oShortArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = (short)c;
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oShortArrayFromMemoryMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        int offset = SPOP;
        int numShorts = SPOP;
        const short* pSrc = (const short *)(SPOP);
        ulong copyEnd = (ulong)(numShorts + offset);
        if (copyEnd != a.size())
        {
            a.resize(copyEnd);
        }
        short* pDst = &(a[0]) + offset;
        memcpy(pDst, pSrc, numShorts << 1);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayFindValueMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        long found = 0;
        short val = (short)SPOP;
        oShortArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (val == a[i])
            {
                found = ~0;
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayReverseMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        std::reverse(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oShortArraySortMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        oShortArray& a = *(pArray->elements);
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayUnsignedSortMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        std::vector<unsigned short> & a = *(((std::vector<unsigned short> *)(pArray->elements)));
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }


    baseMethodEntry oShortArrayMembers[] =
	{
		METHOD("__newOp", oShortArrayNew),
		METHOD("delete", oShortArrayDeleteMethod),
		METHOD("showInner", oShortArrayShowInnerMethod),

        METHOD_RET("headIter", oShortArrayHeadIterMethod, RETURNS_OBJECT(kBCIShortArrayIter)),
        METHOD_RET("tailIter", oShortArrayTailIterMethod, RETURNS_OBJECT(kBCIShortArrayIter)),
        METHOD_RET("find", oShortArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oShortArrayCloneMethod, RETURNS_OBJECT(kBCIShortArray)),
        METHOD_RET("count", oShortArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oShortArrayClearMethod),

        METHOD_RET("get", oShortArrayGetMethod, RETURNS_NATIVE(kBaseTypeShort)),
        METHOD("set", oShortArraySetMethod),
        METHOD_RET("ref", oShortArrayRefMethod, RETURNS_NATIVE(kBaseTypeShort | kDTIsPtr)),
        METHOD("swap", oShortArraySwapMethod),
        METHOD("resize", oShortArrayResizeMethod),
        METHOD("insert", oShortArrayInsertMethod),
        METHOD_RET("remove", oShortArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeShort)),
        METHOD("push", oShortArrayPushMethod),
        METHOD_RET("pop", oShortArrayPopMethod, RETURNS_NATIVE(kBaseTypeShort)),
        METHOD_RET("base", oShortArrayBaseMethod, RETURNS_NATIVE(kBaseTypeShort | kDTIsPtr)),
        METHOD("load", oShortArrayLoadMethod),
        METHOD("fromMemory", oShortArrayFromMemoryMethod),
        METHOD_RET("findValue", oShortArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oShortArrayReverseMethod),
        METHOD("sort", oShortArraySortMethod),
        METHOD("usort", oShortArrayUnsignedSortMethod),

        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 ShortArrayIter
	//

	FORTHOP(oShortArrayIterNew)
	{
		ForthEngine *pEngine = GET_ENGINE;
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a ShortArrayIter object");
	}

	FORTHOP(oShortArrayIterDeleteMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekPrevMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekHeadMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterSeekTailMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

    FORTHOP(oShortArrayIterAtHeadMethod)
    {
        GET_THIS(oShortArrayIterStruct, pIter);
        oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayIterAtTailMethod)
    {
        GET_THIS(oShortArrayIterStruct, pIter);
        oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayIterNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			short val = a[pIter->cursor];
			SPUSH((long)val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterPrevMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			short val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterCurrentMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			short val = a[pIter->cursor];
			SPUSH((long)val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oShortArrayIterRemoveMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		oShortArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

    FORTHOP(oShortArrayIterCloneMethod)
    {
        GET_THIS(oShortArrayIterStruct, pIter);
        oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
        pArray->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIShortArrayIter);
        MALLOCATE_ITER(oShortArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIter->pMethods;
        pNewIter->refCount = 0;
        pNewIter->parent = pIter->parent;
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayIterSeekMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);

        oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
        oShortArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("ShortArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayIterTellMethod)
    {
        GET_THIS(oShortArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    FORTHOP(oShortArrayIterFindNextMethod)
	{
		GET_THIS(oShortArrayIterStruct, pIter);
		long retVal = 0;
		char soughtShort = (char)(SPOP);
		oShortArrayStruct* pArray = reinterpret_cast<oShortArrayStruct *>(pIter->parent);
		oShortArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtShort)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oShortArrayIterMembers[] =
	{
		METHOD("__newOp", oShortArrayIterNew),
		METHOD("delete", oShortArrayIterDeleteMethod),
		METHOD("seekNext", oShortArrayIterSeekNextMethod),
		METHOD("seekPrev", oShortArrayIterSeekPrevMethod),
		METHOD("seekHead", oShortArrayIterSeekHeadMethod),
		METHOD("seekTail", oShortArrayIterSeekTailMethod),
        METHOD_RET("atHead", oShortArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oShortArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oShortArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oShortArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oShortArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oShortArrayIterRemoveMethod),
        METHOD_RET("clone", oShortArrayIterCloneMethod, RETURNS_OBJECT(kBCIShortArrayIter)),

        METHOD("seek", oShortArrayIterSeekMethod),
        METHOD_RET("tell", oShortArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oShortArrayIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIShortArray)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 IntArray
	//

	typedef std::vector<int> oIntArray;
	struct oIntArrayStruct
	{
        long*           pMethods;
        ulong           refCount;
		oIntArray*      elements;
	};

	struct oIntArrayIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};

    oIntArrayIterStruct* createIntArrayIterator(ForthCoreState* pCore, oIntArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIIntArrayIter);
        MALLOCATE_ITER(oIntArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customIntArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oIntArrayStruct *dstArray = (oIntArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                int value;
                if (sscanf(number.c_str(), "%d", &value) == 1)
                {
                    dstArray->elements->push_back(value);
                }
                else
                {
                    reader->throwError("error parsing float value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oIntArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oIntArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
		pArray->elements = new oIntArray;
		PUSH_OBJECT(pArray);
	}

	FORTHOP(oIntArrayDeleteMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

    FORTHOP(oIntArrayShowInnerMethod)
    {
        char buffer[32];
        GET_THIS(oIntArrayStruct, pArray);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oIntArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
        for (unsigned int i = 0; i < a.size(); i++)
        {
            pShowContext->BeginArrayElement();
            sprintf(buffer, "%d", a[i]);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayHeadIterMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oIntArrayIterStruct* pIter = createIntArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayTailIterMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;
        oIntArrayIterStruct* pIter = createIntArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayFindMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        long found = 0;
        int soughtInt = SPOP;
        oIntArray::iterator iter;
        oIntArray& a = *(pArray->elements);
        for (unsigned int i = 0; i < a.size(); i++)
        {
            if (soughtInt == a[i])
            {
                oIntArrayIterStruct* pIter = createIntArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayCloneMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        // create clone array and set is size to match this array
        ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIIntArray);
        MALLOCATE_OBJECT(oIntArrayStruct, pCloneArray, pClassVocab);
        pCloneArray->pMethods = pClassVocab->GetMethods();
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oIntArray;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneArray->elements->resize(numElements);
            // copy this array contents to clone array
            oIntArray& cloneElements = *(pCloneArray->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements << 2);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneArray);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayCountMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        SPUSH((long)(pArray->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayGetMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH(a[ix]);
        }
        else
        {
            ReportBadArrayIndex("IntArray:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArraySetMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            a[ix] = SPOP;
        }
        else
        {
            ReportBadArrayIndex("IntArray:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayRefMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("IntArray:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArraySwapMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            int t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("IntArray:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayResizeMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        ulong oldSize = a.size();
        a.resize(newSize);
        if (oldSize < newSize)
        {
            // growing - add zeros to end of array
            int* pElement = &(a[oldSize]);
            memset(pElement, 0, ((newSize - oldSize) << 2));
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayInsertMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);

        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            int insertedVal = SPOP;
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(int) * (oldSize - ix));
            }
            a[ix] = insertedVal;
        }
        else
        {
            ReportBadArrayIndex("IntArray:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayRemoveMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oIntArray& a = *(pArray->elements);
        if (ix < a.size())
        {
            SPUSH(a[ix]);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("IntArray:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayPushMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        int val = SPOP;
        a.push_back(val);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayPopMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        if (a.size() > 0)
        {
            long val = a.back();
            a.pop_back();
            SPUSH(val);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty oIntArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayBaseMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        SPUSH((long)&(a[0]));
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayLoadMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				int c = SPOP;
				a[i] = c;
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oIntArrayFindValueMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        long found = 0;
        int val = SPOP;
        oIntArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (val == a[i])
            {
                found = ~0;
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayReverseMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        std::reverse(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oIntArraySortMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        oIntArray& a = *(pArray->elements);
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayUnsignedSortMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        std::vector<unsigned int> & a = *(((std::vector<unsigned int> *)(pArray->elements)));
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

	FORTHOP(oIntArrayFromMemoryMethod)
	{
		GET_THIS(oIntArrayStruct, pArray);
		oIntArray& a = *(pArray->elements);
		int offset = SPOP;
		int numInts = SPOP;
		const int* pSrc = (const int *)(SPOP);
		ulong copyEnd = (ulong)(numInts + offset);
		if (copyEnd != a.size())
		{
            a.resize(copyEnd);
		}
		int* pDst = &(a[0]) + offset;
		memcpy(pDst, pSrc, numInts << 2);
		METHOD_RETURN;
	}


	baseMethodEntry oIntArrayMembers[] =
	{
		METHOD("__newOp", oIntArrayNew),
		METHOD("delete", oIntArrayDeleteMethod),
		METHOD("showInner", oIntArrayShowInnerMethod),

        METHOD_RET("headIter", oIntArrayHeadIterMethod, RETURNS_OBJECT(kBCIIntArrayIter)),
        METHOD_RET("tailIter", oIntArrayTailIterMethod, RETURNS_OBJECT(kBCIIntArrayIter)),
        METHOD_RET("find", oIntArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oIntArrayCloneMethod, RETURNS_OBJECT(kBCIIntArray)),
        METHOD_RET("count", oIntArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oIntArrayClearMethod),

        METHOD_RET("get", oIntArrayGetMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("set", oIntArraySetMethod),
        METHOD_RET("ref", oIntArrayRefMethod, RETURNS_NATIVE(kBaseTypeInt | kDTIsPtr)),
        METHOD("swap", oIntArraySwapMethod),
        METHOD("resize", oIntArrayResizeMethod),
        METHOD("insert", oIntArrayInsertMethod),
        METHOD_RET("remove", oIntArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("push", oIntArrayPushMethod),
        METHOD_RET("pop", oIntArrayPopMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("base", oIntArrayBaseMethod, RETURNS_NATIVE(kBaseTypeInt | kDTIsPtr)),
        METHOD("load", oIntArrayLoadMethod),
        METHOD("fromMemory", oIntArrayFromMemoryMethod),
        METHOD_RET("findValue", oIntArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oIntArrayReverseMethod),
        METHOD("sort", oIntArraySortMethod),
        METHOD("usort", oIntArrayUnsignedSortMethod),

        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 IntArrayIter
	//

	FORTHOP(oIntArrayIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create an IntArrayIter object");
	}

	FORTHOP(oIntArrayIterDeleteMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekPrevMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekHeadMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterSeekTailMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

    FORTHOP(oIntArrayIterAtHeadMethod)
    {
        GET_THIS(oIntArrayIterStruct, pIter);
        oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayIterAtTailMethod)
    {
        GET_THIS(oIntArrayIterStruct, pIter);
        oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayIterNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			SPUSH(a[pIter->cursor]);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterPrevMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			SPUSH(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterCurrentMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			SPUSH(a[pIter->cursor]);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntArrayIterRemoveMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		oIntArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

    FORTHOP(oIntArrayIterCloneMethod)
    {
        GET_THIS(oIntArrayIterStruct, pIter);
        oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
        pArray->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIIntArrayIter);
        MALLOCATE_ITER(oIntArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIter->pMethods;
        pNewIter->refCount = 0;
        pNewIter->cursor = pIter->cursor;
        pNewIter->parent = reinterpret_cast<ForthObject>(pArray);
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayIterSeekMethod)
    {
        GET_THIS(oIntArrayIterStruct, pIter);

        oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
        oIntArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("IntArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayIterTellMethod)
    {
        GET_THIS(oIntArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    FORTHOP(oIntArrayIterFindNextMethod)
	{
		GET_THIS(oIntArrayIterStruct, pIter);
		long retVal = 0;
		char soughtInt = (char)(SPOP);
		oIntArrayStruct* pArray = reinterpret_cast<oIntArrayStruct *>(pIter->parent);
		oIntArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtInt)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oIntArrayIterMembers[] =
	{
		METHOD("__newOp", oIntArrayIterNew),
		METHOD("delete", oIntArrayIterDeleteMethod),
		METHOD("seekNext", oIntArrayIterSeekNextMethod),
		METHOD("seekPrev", oIntArrayIterSeekPrevMethod),
		METHOD("seekHead", oIntArrayIterSeekHeadMethod),
		METHOD("seekTail", oIntArrayIterSeekTailMethod),
        METHOD_RET("atHead", oIntArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oIntArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oIntArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oIntArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oIntArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oIntArrayIterRemoveMethod),
        METHOD_RET("clone", oIntArrayIterCloneMethod, RETURNS_OBJECT(kBCIIntArrayIter)),

        METHOD("seek", oIntArrayIterSeekMethod),
        METHOD_RET("tell", oIntArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oIntArrayIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIIntArray)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 FloatArray
	//

    bool customFloatArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oIntArrayStruct *dstArray = (oIntArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                float value;
                if (sscanf(number.c_str(), "%f", &value) == 1)
                {
                    dstArray->elements->push_back(*((int *)&value));
                }
                else
                {
                    reader->throwError("error parsing float value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oFloatArrayShowInnerMethod)
    {
        char buffer[32];
        GET_THIS(oIntArrayStruct, pArray);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oIntArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
        for (unsigned int i = 0; i < a.size(); i++)
        {
            pShowContext->BeginArrayElement();
            float fval = *((float *)(&(a[i])));
            sprintf(buffer, "%f", fval);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oFloatArraySortMethod)
    {
        GET_THIS(oIntArrayStruct, pArray);
        std::vector<float> & a = *(((std::vector<float> *)(pArray->elements)));
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    baseMethodEntry oFloatArrayMembers[] =
	{
        // note that all these methods except showInner and sort are cloned from IntArray
        METHOD("__newOp", oIntArrayNew),
        METHOD("delete", oIntArrayDeleteMethod),
        METHOD("showInner", oFloatArrayShowInnerMethod),

        METHOD_RET("headIter", oIntArrayHeadIterMethod, RETURNS_OBJECT(kBCIIntArrayIter)),
        METHOD_RET("tailIter", oIntArrayTailIterMethod, RETURNS_OBJECT(kBCIIntArrayIter)),
        METHOD_RET("find", oIntArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oIntArrayCloneMethod, RETURNS_OBJECT(kBCIIntArray)),
        METHOD_RET("count", oIntArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oIntArrayClearMethod),

        METHOD_RET("get", oIntArrayGetMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("set", oIntArraySetMethod),
        METHOD_RET("ref", oIntArrayRefMethod, RETURNS_NATIVE(kBaseTypeInt | kDTIsPtr)),
        METHOD("swap", oIntArraySwapMethod),
        METHOD("resize", oIntArrayResizeMethod),
        METHOD("insert", oIntArrayInsertMethod),
        METHOD_RET("remove", oIntArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("push", oIntArrayPushMethod),
        METHOD_RET("pop", oIntArrayPopMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("base", oIntArrayBaseMethod, RETURNS_NATIVE(kBaseTypeInt | kDTIsPtr)),
        METHOD("load", oIntArrayLoadMethod),
        METHOD("fromMemory", oIntArrayFromMemoryMethod),
        METHOD_RET("findValue", oIntArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oIntArrayReverseMethod),
        METHOD("sort", oFloatArraySortMethod),

        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongArray
	//

	typedef std::vector<long long> oLongArray;
	struct oLongArrayStruct
	{
        long*           pMethods;
        ulong           refCount;
		oLongArray*    elements;
	};

	struct oLongArrayIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};

    oLongArrayIterStruct* createLongArrayIterator(ForthCoreState* pCore, oLongArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCILongArrayIter);
        MALLOCATE_ITER(oLongArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customLongArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oLongArrayStruct *dstArray = (oLongArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                long long value;
                if (sscanf(number.c_str(), "%lld", &value) == 1)
                {
                    dstArray->elements->push_back(value);
                }
                else
                {
                    reader->throwError("error parsing long value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oLongArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oLongArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
		pArray->elements = new oLongArray;
		PUSH_OBJECT(pArray);
	}

	FORTHOP(oLongArrayDeleteMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

    FORTHOP(oLongArrayShowInnerMethod)
    {
        char buffer[32];
        GET_THIS(oLongArrayStruct, pArray);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oLongArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
        for (unsigned int i = 0; i < a.size(); i++)
        {
            pShowContext->BeginArrayElement();
            sprintf(buffer, "%lld", a[i]);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayHeadIterMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oLongArrayIterStruct* pIter = createLongArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayTailIterMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oLongArrayIterStruct* pIter = createLongArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayFindMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        long found = 0;
        stackInt64 a64;
        LPOP(a64);
        long long soughtLong = a64.s64;
        oLongArray::iterator iter;
        oLongArray& a = *(pArray->elements);
        for (unsigned int i = 0; i < a.size(); i++)
        {
            if (soughtLong == a[i])
            {
                oLongArrayIterStruct* pIter = createLongArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayCloneMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        // create clone array and set is size to match this array
        ForthClassVocabulary *pClassVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCILongArray);
        MALLOCATE_OBJECT(oLongArrayStruct, pCloneArray, pClassVocab);
        pCloneArray->pMethods = pClassVocab->GetMethods();
        pCloneArray->refCount = 0;
        pCloneArray->elements = new oLongArray;
        size_t numElements = a.size();
        if (numElements != 0)
        {
            pCloneArray->elements->resize(numElements);
            // copy this array contents to clone array
            oLongArray& cloneElements = *(pCloneArray->elements);
            memcpy(&(cloneElements[0]), &(a[0]), numElements << 3);
        }
        // push cloned array on TOS
        PUSH_OBJECT(pCloneArray);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayCountMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        SPUSH((long)(pArray->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayGetMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            stackInt64 a64;
            a64.s64 = a[ix];
            LPUSH(a64);
        }
        else
        {
            ReportBadArrayIndex("LongArray:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArraySetMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        stackInt64 a64;
        if (a.size() > ix)
        {
            LPOP(a64);
            a[ix] = a64.s64;
        }
        else
        {
            ReportBadArrayIndex("LongArray:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayRefMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (a.size() > ix)
        {
            SPUSH((long)&(a[ix]));
        }
        else
        {
            ReportBadArrayIndex("LongArray:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArraySwapMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((a.size() > ix) && (a.size() > jx))
        {
            long long t = a[ix];
            a[ix] = a[jx];
            a[jx] = t;
        }
        else
        {
            ReportBadArrayIndex("LongArray:swap", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayResizeMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		ulong oldSize = a.size();
		a.resize(newSize);
		if (oldSize < newSize)
		{
			// growing - add zeros to end of array
			long long* pElement = &(a[oldSize]);
			memset(pElement, 0, ((newSize - oldSize) << 3));
		}
		METHOD_RETURN;
	}

    FORTHOP(oLongArrayInsertMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);

        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            stackInt64 a64;
            LPOP(a64);
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(long long) * (oldSize - ix));
            }
            a[ix] = a64.s64;
        }
        else
        {
            ReportBadArrayIndex("LongArray:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayRemoveMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oLongArray& a = *(pArray->elements);
        if (ix < a.size())
        {
            stackInt64 a64;
            a64.s64 = a[ix];
            LPUSH(a64);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("LongArray:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayPushMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        stackInt64 a64;
        LPOP(a64);
        a.push_back(a64.s64);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayPopMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        if (a.size() > 0)
        {
            stackInt64 a64;
            a64.s64 = a.back();
            a.pop_back();
            LPUSH(a64);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty oLongArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayBaseMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        SPUSH((long)&(a[0]));
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayLoadMethod)
	{
		GET_THIS(oLongArrayStruct, pArray);
		oLongArray& a = *(pArray->elements);
		ulong newSize = SPOP;
		a.resize(newSize);
		stackInt64 a64;
		if (newSize > 0)
		{
			for (int i = newSize - 1; i >= 0; i--)
			{
				LPOP(a64);
				a[i] = a64.s64;
			}
		}
		METHOD_RETURN;
	}

    FORTHOP(oLongArrayFromMemoryMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        int offset = SPOP;
        int numLongs = SPOP;
        const long long* pSrc = (const long long*)(SPOP);
        ulong copyEnd = (ulong)(numLongs + offset);
        if (copyEnd != a.size())
        {
            a.resize(copyEnd);
        }
        long long* pDst = &(a[0]) + offset;
        memcpy(pDst, pSrc, numLongs << 3);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayFindValueMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        long found = 0;
        stackInt64 a64;
        LPOP(a64);
        long long val = a64.s64;
        oLongArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (val == a[i])
            {
                found = ~0;
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayReverseMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        std::reverse(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oLongArraySortMethod)
    {
        GET_THIS(oLongArrayStruct, pArray);
        oLongArray& a = *(pArray->elements);
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayUnsignedSortMethod)
    {
        GET_THIS(oShortArrayStruct, pArray);
        std::vector<unsigned long long>& a = *(((std::vector<unsigned long long> *)(pArray->elements)));
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

	baseMethodEntry oLongArrayMembers[] =
	{
		METHOD("__newOp", oLongArrayNew),
		METHOD("delete", oLongArrayDeleteMethod),
		METHOD("showInner", oLongArrayShowInnerMethod),

        METHOD_RET("headIter", oLongArrayHeadIterMethod, RETURNS_OBJECT(kBCILongArrayIter)),
        METHOD_RET("tailIter", oLongArrayTailIterMethod, RETURNS_OBJECT(kBCILongArrayIter)),
        METHOD_RET("find", oLongArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oLongArrayCloneMethod, RETURNS_OBJECT(kBCILongArray)),
        METHOD_RET("count", oLongArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oLongArrayClearMethod),

        METHOD_RET("get", oLongArrayGetMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD("set", oLongArraySetMethod),
        METHOD_RET("ref", oLongArrayRefMethod, RETURNS_NATIVE(kBaseTypeLong | kDTIsPtr)),
        METHOD("swap", oLongArraySwapMethod),
        METHOD("resize", oLongArrayResizeMethod),
        METHOD("insert", oLongArrayInsertMethod),
        METHOD_RET("remove", oLongArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD("push", oLongArrayPushMethod),
        METHOD_RET("pop", oLongArrayPopMethod, RETURNS_NATIVE(kBaseTypeLong)),
        METHOD_RET("base", oLongArrayBaseMethod, RETURNS_NATIVE(kBaseTypeLong | kDTIsPtr)),
        METHOD("load", oLongArrayLoadMethod),
        METHOD("fromMemory", oLongArrayFromMemoryMethod),
        METHOD_RET("findValue", oLongArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oLongArrayReverseMethod),
        METHOD("sort", oLongArraySortMethod),
        METHOD("usort", oLongArrayUnsignedSortMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongArrayIter
	//

	FORTHOP(oLongArrayIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a LongArrayIter object");
	}

	FORTHOP(oLongArrayIterDeleteMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		FREE_ITER(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		if (pIter->cursor < pArray->elements->size())
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekPrevMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekHeadMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterSeekTailMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		pIter->cursor = pArray->elements->size();
		METHOD_RETURN;
	}

    FORTHOP(oLongArrayIterAtHeadMethod)
    {
        GET_THIS(oLongArrayIterStruct, pIter);
        oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayIterAtTailMethod)
    {
        GET_THIS(oArrayIterStruct, pIter);
        oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayIterNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			pIter->cursor++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterPrevMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor == 0)
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterCurrentMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor >= a.size())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = a[pIter->cursor];
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongArrayIterRemoveMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		oLongArray& a = *(pArray->elements);
		if (pIter->cursor < a.size())
		{
			pArray->elements->erase(pArray->elements->begin() + pIter->cursor);
		}
		METHOD_RETURN;
	}

    FORTHOP(oLongArrayIterCloneMethod)
    {
        GET_THIS(oLongArrayIterStruct, pIter);
        oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
        pArray->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCILongArrayIter);
        MALLOCATE_ITER(oLongArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIter->pMethods;
        pNewIter->refCount = 0;
        pNewIter->parent = reinterpret_cast<ForthObject>(pArray);
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayIterSeekMethod)
    {
        GET_THIS(oLongArrayIterStruct, pIter);

        oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
        oLongArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("LongArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayIterTellMethod)
    {
        GET_THIS(oLongArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    FORTHOP(oLongArrayIterFindNextMethod)
	{
		GET_THIS(oLongArrayIterStruct, pIter);
		long retVal = 0;
		stackInt64 a64;
		LPOP(a64);
		long long soughtLong = a64.s64;
		oLongArrayStruct* pArray = reinterpret_cast<oLongArrayStruct *>(pIter->parent);
		oLongArray& a = *(pArray->elements);
		unsigned int i = pIter->cursor;
		while (i < a.size())
		{
			if (a[i] == soughtLong)
			{
				retVal = ~0;
				pIter->cursor = i;
				break;
			}
		}
		SPUSH(retVal);
		METHOD_RETURN;
	}

	baseMethodEntry oLongArrayIterMembers[] =
	{
		METHOD("__newOp", oLongArrayIterNew),
		METHOD("delete", oLongArrayIterDeleteMethod),

		METHOD("seekNext", oLongArrayIterSeekNextMethod),
		METHOD("seekPrev", oLongArrayIterSeekPrevMethod),
		METHOD("seekHead", oLongArrayIterSeekHeadMethod),
		METHOD("seekTail", oLongArrayIterSeekTailMethod),
        METHOD_RET("atHead", oLongArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oLongArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oLongArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oLongArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oLongArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oLongArrayIterRemoveMethod),
        METHOD_RET("clone", oLongArrayIterCloneMethod, RETURNS_OBJECT(kBCILongArrayIter)),

        METHOD("seek", oLongArrayIterSeekMethod),
        METHOD_RET("tell", oLongArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("findNext", oLongArrayIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCILongArray)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 DoubleArray
	//

	typedef std::vector<double> oDoubleArray;
	struct oDoubleArrayStruct
	{
        long*           pMethods;
        ulong           refCount;
		oDoubleArray*   elements;
	};

	struct oDoubleArrayIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		ulong			cursor;
	};

    oDoubleArrayIterStruct* createDoubleArrayIterator(ForthCoreState* pCore, oDoubleArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIDoubleArrayIter);
        MALLOCATE_ITER(oDoubleArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customDoubleArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "elements")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oDoubleArrayStruct *dstArray = (oDoubleArrayStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('[');
            std::string number;
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
                reader->getNumber(number);
                double value;
                if (sscanf(number.c_str(), "%lg", &value) == 1)
                {
                    dstArray->elements->push_back(value);
                }
                else
                {
                    reader->throwError("error parsing double value");
                }
            }
            return true;
        }
        return false;
    }

    FORTHOP(oDoubleArrayNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oDoubleArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
		pArray->elements = new oDoubleArray;
		PUSH_OBJECT(pArray);
	}

	FORTHOP(oDoubleArrayDeleteMethod)
	{
		GET_THIS(oDoubleArrayStruct, pArray);
		delete pArray->elements;
		FREE_OBJECT(pArray);
		METHOD_RETURN;
	}

    FORTHOP(oDoubleArrayShowInnerMethod)
    {
        char buffer[32];
        GET_THIS(oDoubleArrayStruct, pArray);
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        oDoubleArray& a = *(pArray->elements);
        pShowContext->BeginElement("elements");
        pShowContext->BeginArray();
        for (unsigned int i = 0; i < a.size(); i++)
        {
            pShowContext->BeginArrayElement();
            sprintf(buffer, "%g", a[i]);
            pShowContext->ShowText(buffer);
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayHeadIterMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oDoubleArrayIterStruct* pIter = createDoubleArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayTailIterMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;
        oDoubleArrayIterStruct* pIter = createDoubleArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayFindMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        long found = 0;
        double soughtDouble = DPOP;
        oDoubleArray::iterator iter;
        oDoubleArray& a = *(pArray->elements);
        for (unsigned int i = 0; i < a.size(); i++)
        {
            if (soughtDouble == a[i])
            {
                oDoubleArrayIterStruct* pIter = createDoubleArrayIterator(pCore, pArray);
                pIter->cursor = i;

                PUSH_OBJECT(pIter);
                found = ~0;
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayGetMethod)
	{
		GET_THIS(oDoubleArrayStruct, pArray);
		oDoubleArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			DPUSH(a[ix]);
		}
		else
		{
			ReportBadArrayIndex("DoubleArray:get", ix, a.size());
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleArraySetMethod)
	{
		GET_THIS(oDoubleArrayStruct, pArray);
		oDoubleArray& a = *(pArray->elements);
		ulong ix = (ulong)SPOP;
		if (a.size() > ix)
		{
			double dval = DPOP;
			a[ix] = dval;
		}
		else
		{
			ReportBadArrayIndex("DoubleArray:set", ix, a.size());
		}
		METHOD_RETURN;
	}

    FORTHOP(oDoubleArrayInsertMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);

        oDoubleArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong oldSize = a.size();
        if (oldSize >= ix)
        {
            double dval = DPOP;
            // add dummy element to end of array
            a.resize(oldSize + 1);
            if ((oldSize > 0) && (ix < oldSize))
            {
                // move old entries up by size of ForthObject
                memmove(&(a[ix + 1]), &(a[ix]), sizeof(double) * (oldSize - ix));
            }
            a[ix] = dval;
        }
        else
        {
            ReportBadArrayIndex("DoubleArray:insert", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayRemoveMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oDoubleArray& a = *(pArray->elements);
        if (ix < a.size())
        {
            double dval = a[ix];
            DPUSH(dval);
            a.erase(a.begin() + ix);
        }
        else
        {
            ReportBadArrayIndex("DoubleArray:remove", ix, a.size());
        }
        METHOD_RETURN;
    }

	FORTHOP(oDoubleArrayPushMethod)
	{
		GET_THIS(oDoubleArrayStruct, pArray);
		oDoubleArray& a = *(pArray->elements);
		double dval = DPOP;
		a.push_back(dval);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleArrayPopMethod)
	{
		GET_THIS(oDoubleArrayStruct, pArray);
		oDoubleArray& a = *(pArray->elements);
		if (a.size() > 0)
		{
			double dval = a.back();
			a.pop_back();
			DPUSH(dval);
		}
		else
		{
			GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty oDoubleArray");
		}
		METHOD_RETURN;
	}

    FORTHOP(oDoubleArrayLoadMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        oDoubleArray& a = *(pArray->elements);
        ulong newSize = SPOP;
        a.resize(newSize);
        if (newSize > 0)
        {
            for (int i = newSize - 1; i >= 0; i--)
            {
                double dval = DPOP;
                a[i] = dval;
            }
        }
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArrayFindValueMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        long found = 0;
        double dval = DPOP;
        oDoubleArray& a = *(pArray->elements);
        for (ulong i = 0; i < a.size(); i++)
        {
            if (dval == a[i])
            {
                found = ~0;
                SPUSH(i);
                break;
            }
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleArraySortMethod)
    {
        GET_THIS(oDoubleArrayStruct, pArray);
        oDoubleArray& a = *(pArray->elements);
        std::sort(a.begin(), a.end());
        METHOD_RETURN;
    }

    baseMethodEntry oDoubleArrayMembers[] =
	{
        // note that many of these methods are cloned from LongArray
		METHOD("__newOp", oDoubleArrayNew),
		METHOD("delete", oDoubleArrayDeleteMethod),
		METHOD("showInner", oDoubleArrayShowInnerMethod),

        METHOD_RET("headIter", oDoubleArrayHeadIterMethod, RETURNS_OBJECT(kBCIDoubleArrayIter)),
        METHOD_RET("tailIter", oDoubleArrayTailIterMethod, RETURNS_OBJECT(kBCIDoubleArrayIter)),
        METHOD_RET("find", oDoubleArrayFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("clone", oLongArrayCloneMethod, RETURNS_OBJECT(kBCILongArray)),
        METHOD_RET("count", oLongArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oLongArrayClearMethod),

        METHOD_RET("get", oDoubleArrayGetMethod, RETURNS_NATIVE(kBaseTypeDouble)),
        METHOD("set", oDoubleArraySetMethod),
        METHOD_RET("ref", oLongArrayRefMethod, RETURNS_NATIVE(kBaseTypeLong | kDTIsPtr)),
        METHOD("swap", oLongArraySwapMethod),
        METHOD("resize", oLongArrayResizeMethod),
        METHOD("insert", oDoubleArrayInsertMethod),
        METHOD_RET("remove", oDoubleArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeDouble)),
        METHOD("push", oDoubleArrayPushMethod),
        METHOD_RET("pop", oDoubleArrayPopMethod, RETURNS_NATIVE(kBaseTypeDouble)),
        METHOD_RET("base", oLongArrayBaseMethod, RETURNS_NATIVE(kBaseTypeLong | kDTIsPtr)),
        METHOD("load", oDoubleArrayLoadMethod),
        METHOD("fromMemory", oLongArrayFromMemoryMethod),
        METHOD_RET("findValue", oDoubleArrayFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("reverse", oLongArrayReverseMethod),
        METHOD("sort", oDoubleArraySortMethod),

        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
		END_MEMBERS
	};


    //////////////////////////////////////////////////////////////////////
    ///
    //                 StructArray
    //

    typedef std::vector<char> oStructArray;
    struct oStructArrayStruct
    {
        long*                   pMethods;
        ulong                   refCount;
        oStructArray*           elements;
        ulong                   elementSize;
        ulong                   numElements;
        ForthStructVocabulary*  pVocab;
    };

    struct oStructArrayIterStruct
    {
        long*           pMethods;
        ulong			refCount;
        ForthObject		parent;
        ulong			cursor;
    };

    oStructArrayIterStruct* createStructArrayIterator(ForthCoreState* pCore, oStructArrayStruct* pArray)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIStructArrayIter);
        MALLOCATE_ITER(oStructArrayIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pArray);
        return pIter;
    }

    bool customStructArrayReader(const std::string& elementName, ForthObjectReader* reader)
    {
        oStructArrayStruct *dstArray = (oStructArrayStruct *)(reader->getCustomReaderContext().pData);
        if (elementName == "structType")
        {
            std::string structType;
            reader->getString(structType);
            dstArray->pVocab = ForthTypesManager::GetInstance()->GetStructVocabulary(structType.c_str());
            if (dstArray->pVocab == nullptr)
            {
                reader->throwError("unknown struct type for StructArray");
            }
            dstArray->elementSize = dstArray->pVocab->GetSize();
            dstArray->numElements = 0;
            return true;
        }
        else if (elementName == "elements")
        {
            if (dstArray->pVocab == nullptr)
            {
                reader->throwError("StructArray is missing struct type");
            }
            oStructArray& a = *(dstArray->elements);
            reader->getRequiredChar('[');
            int offset = 0;
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
                int oldSize = a.size();
                a.resize(oldSize + dstArray->elementSize);
                char* pDst = &(a[oldSize]);
                reader->getStruct(dstArray->pVocab, 0, pDst);
                dstArray->numElements++;
            }
            return true;
        }
        return false;
    }

    FORTHOP(oStructArrayNew)
    {
        ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        MALLOCATE_OBJECT(oStructArrayStruct, pArray, pClassVocab);
        pArray->pMethods = pClassVocab->GetMethods();
        pArray->refCount = 0;
        pArray->elements = new oStructArray;
        pArray->elementSize = 1;
        pArray->numElements = 0;
        pArray->pVocab = nullptr;
        PUSH_OBJECT(pArray);
    }

    FORTHOP(oStructArrayDeleteMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        delete pArray->elements;
        FREE_OBJECT(pArray);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayShowInnerMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        if (pArray->pVocab != nullptr)
        {
            ForthEngine *pEngine = GET_ENGINE;
            ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
            oStructArray& a = *(pArray->elements);

            pShowContext->BeginElement("structType");
            pShowContext->ShowQuotedText(pArray->pVocab->GetName());

            pShowContext->BeginElement("elements");
            pShowContext->ShowTextReturn("[");
            if (a.size() > 0)
            {
                pShowContext->BeginIndent();
                for (unsigned int i = 0; i < pArray->numElements; i++)
                {
                    if (i != 0)
                    {
                        pShowContext->ShowTextReturn(",");
                    }
                    pShowContext->ShowIndent();
                    void* pStruct = &(a[i * pArray->elementSize]);
                    pShowContext->BeginNestedShow();
                    pArray->pVocab->ShowData(pStruct, pCore, false);
                    pShowContext->EndNestedShow();
                }
                pShowContext->EndIndent();
                pShowContext->ShowIndent();
            }
            pShowContext->ShowTextReturn();
            pShowContext->ShowIndent("]");
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:show unknown struct type");
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayHeadIterMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;

        oStructArrayIterStruct* pIter = createStructArrayIterator(pCore, pArray);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayTailIterMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        pArray->refCount++;
        TRACK_KEEP;
        oStructArrayIterStruct* pIter = createStructArrayIterator(pCore, pArray);
        pIter->cursor = pArray->elements->size();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayCountMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        SPUSH(pArray->numElements);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayClearMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayGetMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (pArray->numElements > ix)
        {
            char* pDstStruct = (char *)SPOP;
            if (pDstStruct != nullptr)
            {
                char* pSrc = &(a[ix * pArray->elementSize]);
                memcpy(pDstStruct, pSrc, pArray->elementSize);
            }
            else
            {
                GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:get null destination pointer");
            }
        }
        else
        {
            ReportBadArrayIndex("StructArray:get", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArraySetMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (pArray->numElements > ix)
        {
            char* pSrcStruct = (char *)SPOP;
            if (pSrcStruct != nullptr)
            {
                char* pDst = &(a[ix * pArray->elementSize]);
                memcpy(pDst, pSrcStruct, pArray->elementSize);
            }
            else
            {
                GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:set null source pointer");
            }
        }
        else
        {
            ReportBadArrayIndex("StructArray:set", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayRefMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (pArray->numElements > ix)
        {
            SPUSH((long)&(a[ix * pArray->elementSize]));
        }
        else
        {
            ReportBadArrayIndex("StructArray:ref", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArraySwapMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        ulong jx = (ulong)SPOP;
        if ((pArray->numElements > ix) && (pArray->numElements > jx))
        {
            ix *= pArray->elementSize;
            jx *= pArray->elementSize;
            for (unsigned long i = 0; i < pArray->elementSize; ++i)
            {
                char t = a[ix];
                a[ix] = a[jx];
                a[jx] = t;
            }
        }
        else
        {
            ReportBadArrayIndex("StructArray:swap", ix, pArray->numElements);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayResizeMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        ulong newSize = (SPOP) * pArray->elementSize;
        ulong oldSize = a.size();
        a.resize(newSize);
        if (oldSize < newSize)
        {
            // growing - add null bytes to end of array
            char* pElement = &(a[oldSize]);
            memset(pElement, 0, newSize - oldSize);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayInsertMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);

        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)SPOP;
        if (pArray->numElements >= ix)
        {
            int elementSize = pArray->elementSize;
            char* insertedStruct = (char *)SPOP;
            if (insertedStruct != nullptr)
            {
                // add dummy element to end of array
                ulong oldSize = a.size();
                a.resize(oldSize + elementSize);
                char* pSrc = &(a[ix * elementSize]);
                if ((oldSize > 0) && (ix < pArray->numElements))
                {
                    // move old entries up by size of struct
                    char* pDst = pSrc + elementSize;
                    memmove(pDst, pSrc, elementSize * (pArray->numElements - ix));
                }
                memcpy(pSrc, insertedStruct, elementSize);
                ++pArray->numElements;
            }
            else
            {
                GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:insert null source pointer");
            }
        }
        else
        {
            ReportBadArrayIndex("StructArray:insert", ix, pArray->numElements);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayRemoveMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);

        ulong ix = (ulong)SPOP;
        oStructArray& a = *(pArray->elements);
        if (ix < pArray->numElements)
        {
            int elementSize = pArray->elementSize;
            char* removedStruct = (char *)SPOP;
            if (removedStruct != nullptr)
            {
                char* pSrc = &(a[ix * elementSize]);
                memcpy(removedStruct, pSrc, elementSize);
                // add dummy element to end of array
                ulong oldSize = a.size();
                if (ix < (pArray->numElements - 1))
                {
                    // move old entries down by size of struct
                    char* pDst = pSrc;
                    pSrc += elementSize;
                    memmove(pDst, pSrc, elementSize * ((pArray->numElements - 1) - ix));
                }
                a.resize(oldSize - elementSize);
                --pArray->numElements;
            }
            else
            {
                GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:remove null destination pointer");
            }
        }
        else
        {
            ReportBadArrayIndex("StructArray:remove", ix, pArray->numElements);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayPushMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        char* pSrc = (char *)SPOP;
        if (pSrc != nullptr)
        {
            int oldSize = a.size();
            a.resize(oldSize + pArray->elementSize);
            char* pDst = &(a[oldSize]);
            memcpy(pDst, pSrc, pArray->elementSize);
            pArray->numElements++;
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:push null source pointer");
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayPopMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        if (pArray->numElements > 0)
        {
            char* pDst = (char *)SPOP;
            if (pDst != nullptr)
            {
                int oldSize = a.size();
                char* pSrc = &(a[oldSize - pArray->elementSize]);
                memcpy(pDst, pSrc, pArray->elementSize);
                a.resize(oldSize - pArray->elementSize);
                pArray->numElements--;
            }
            else
            {
                GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray:pop null destination pointer");
            }
            // TODO: why push anything here?  if needed, why not push pDst?
            char val = a.back();
            a.pop_back();
            SPUSH((long)val);
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, " pop of empty StructArray");
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayBaseMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        SPUSH(pArray->numElements > 0 ? (long)&(a[0]) : NULL);
        METHOD_RETURN;
    }

    FORTHOP(oStructArraySetTypeMethod)
    {
        GET_THIS(oStructArrayStruct, pArray);
        oStructArray& a = *(pArray->elements);
        pArray->pVocab = (ForthStructVocabulary *)SPOP;
        if (pArray->pVocab != nullptr)
        {
            pArray->elementSize = pArray->pVocab->GetSize();
        }
        else
        {
            GET_ENGINE->SetError(kForthErrorBadParameter, "StructArray.setType unknown struct type");
        }
        METHOD_RETURN;
    }


    baseMethodEntry oStructArrayMembers[] =
    {
        METHOD("__newOp", oStructArrayNew),
        METHOD("delete", oStructArrayDeleteMethod),
        METHOD("showInner", oStructArrayShowInnerMethod),

        METHOD_RET("headIter", oStructArrayHeadIterMethod, RETURNS_OBJECT(kBCIStructArrayIter)),
        METHOD_RET("tailIter", oStructArrayTailIterMethod, RETURNS_OBJECT(kBCIStructArrayIter)),
        METHOD_RET("count", oStructArrayCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("clear", oStructArrayClearMethod),

        METHOD_RET("get", oStructArrayGetMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD("set", oStructArraySetMethod),
        METHOD_RET("ref", oStructArrayRefMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("swap", oStructArraySwapMethod),
        METHOD("resize", oStructArrayResizeMethod),
        METHOD("insert", oStructArrayInsertMethod),
        METHOD_RET("remove", oStructArrayRemoveMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD("push", oStructArrayPushMethod),
        METHOD_RET("pop", oStructArrayPopMethod, RETURNS_NATIVE(kBaseTypeByte)),
        METHOD_RET("base", oStructArrayBaseMethod, RETURNS_NATIVE(kBaseTypeByte | kDTIsPtr)),
        METHOD("setType", oStructArraySetTypeMethod),
        
        MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("elementSize", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("numElements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),
        MEMBER_VAR("__vocab", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
        END_MEMBERS
    };


    //////////////////////////////////////////////////////////////////////
    ///
    //                 StructArrayIter
    //

    FORTHOP(oStructArrayIterNew)
    {
        GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a StructArrayIter object");
    }

    FORTHOP(oStructArrayIterDeleteMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        SAFE_RELEASE(pCore, pIter->parent);
        FREE_ITER(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterSeekNextMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        if (pIter->cursor < pArray->numElements)
        {
            pIter->cursor++;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterSeekPrevMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        if (pIter->cursor > 0)
        {
            pIter->cursor--;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterSeekHeadMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        pIter->cursor = 0;
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterSeekTailMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        pIter->cursor = pArray->numElements;
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterAtHeadMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterAtTailMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        long retVal = (pIter->cursor == pArray->elements->size()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterNextMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        oStructArray& a = *(pArray->elements);
        if (pIter->cursor >= pArray->numElements)
        {
            SPUSH(0);
        }
        else
        {
            char* pElement = &(a[pIter->cursor * pArray->elementSize]);
            SPUSH((long)pElement);
            pIter->cursor++;
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterPrevMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        oStructArray& a = *(pArray->elements);
        if (pIter->cursor == 0)
        {
            SPUSH(0);
        }
        else
        {
            pIter->cursor--;
            char* pElement = &(a[pIter->cursor * pArray->elementSize]);
            SPUSH((long)pElement);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterCurrentMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        oStructArray& a = *(pArray->elements);
        if (pIter->cursor >= pArray->numElements)
        {
            SPUSH(0);
        }
        else
        {
            char* pElement = &(a[pIter->cursor * pArray->elementSize]);
            SPUSH((long)pElement);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterRemoveMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        oStructArray& a = *(pArray->elements);
        if (pIter->cursor < pArray->numElements)
        {
            int elementSize = pArray->elementSize;
            char* pDst = &(a[pIter->cursor * elementSize]);
            // add dummy element to end of array
            ulong oldSize = a.size();
            if (pIter->cursor < (pArray->numElements - 1))
            {
                // move old entries down by size of struct
                char* pSrc = pDst + elementSize;
                memmove(pDst, pSrc, elementSize * ((pArray->numElements - 1) - pIter->cursor));
            }
            a.resize(oldSize - elementSize);
            --pArray->numElements;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterCloneMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        pArray->refCount++;
        TRACK_KEEP;
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIStructArrayIter);
        MALLOCATE_ITER(oStructArrayIterStruct, pNewIter, pIterVocab);
        pNewIter->pMethods = pIter->pMethods;
        pNewIter->refCount = 0;
        pNewIter->parent = reinterpret_cast<ForthObject>(pArray);
        pNewIter->cursor = pIter->cursor;
        PUSH_OBJECT(pNewIter);
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterSeekMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);

        oStructArrayStruct* pArray = reinterpret_cast<oStructArrayStruct *>(pIter->parent);
        oStructArray& a = *(pArray->elements);
        ulong ix = (ulong)(SPOP);
        if (a.size() >= ix)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("StructArrayIter:seek", ix, a.size());
        }
        METHOD_RETURN;
    }

    FORTHOP(oStructArrayIterTellMethod)
    {
        GET_THIS(oStructArrayIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    baseMethodEntry oStructArrayIterMembers[] =
    {
        METHOD("__newOp", oStructArrayIterNew),
        METHOD("delete", oStructArrayIterDeleteMethod),

        METHOD("seekNext", oStructArrayIterSeekNextMethod),
        METHOD("seekPrev", oStructArrayIterSeekPrevMethod),
        METHOD("seekHead", oStructArrayIterSeekHeadMethod),
        METHOD("seekTail", oStructArrayIterSeekTailMethod),
        METHOD_RET("atHead", oStructArrayIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oStructArrayIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oStructArrayIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("prev", oStructArrayIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("current", oStructArrayIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oStructArrayIterRemoveMethod),
        METHOD_RET("clone", oStructArrayIterCloneMethod, RETURNS_OBJECT(kBCIStructArrayIter)),

        METHOD("seek", oStructArrayIterSeekMethod),
        METHOD_RET("tell", oStructArrayIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),

        MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStructArray)),
        MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

        // following must be last in table
        END_MEMBERS
    };



    //////////////////////////////////////////////////////////////////////
	///
	//                 Pair
	//

	struct oPairStruct
	{
        long*           pMethods;
        ulong           refCount;
		ForthObject	    a;
		ForthObject	    b;
	};

	struct oPairIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		int				cursor;
	};


    oPairIterStruct* createPairIterator(ForthCoreState* pCore, oPairStruct* pPair)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIPairIter);
        MALLOCATE_ITER(oPairIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->parent = reinterpret_cast<ForthObject>(pPair);
        return pIter;
    }

    FORTHOP(oPairNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oPairStruct, pPair, pClassVocab);
        pPair->pMethods = pClassVocab->GetMethods();
        pPair->refCount = 0;
        pPair->a = nullptr;
        pPair->b = nullptr;
		PUSH_OBJECT(pPair);
	}

	FORTHOP(oPairDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oPairStruct, pPair);
		ForthObject& oa = pPair->a;
		SAFE_RELEASE(pCore, oa);
		ForthObject& ob = pPair->b;
		SAFE_RELEASE(pCore, ob);
		FREE_OBJECT(pPair);
		METHOD_RETURN;
	}

    FORTHOP(oPairHeadIterMethod)
    {
        GET_THIS(oPairStruct, pPair);
        pPair->refCount++;
        TRACK_KEEP;

        oPairIterStruct* pIter = createPairIterator(pCore, pPair);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oPairTailIterMethod)
    {
        GET_THIS(oPairStruct, pPair);
        pPair->refCount++;
        TRACK_KEEP;
        oPairIterStruct* pIter = createPairIterator(pCore, pPair);
        pIter->cursor = 2;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oPairGetAMethod)
	{
		GET_THIS(oPairStruct, pPair);
		PUSH_OBJECT(pPair->a);
		METHOD_RETURN;
	}

	FORTHOP(oPairSetAMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pPair->a;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pPair->a = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairGetBMethod)
	{
		GET_THIS(oPairStruct, pPair);
		PUSH_OBJECT(pPair->b);
		METHOD_RETURN;
	}

	FORTHOP(oPairSetBMethod)
	{
		GET_THIS(oPairStruct, pPair);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pPair->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pPair->b = newObj;
		}
		METHOD_RETURN;
	}


	baseMethodEntry oPairMembers[] =
	{
		METHOD("__newOp", oPairNew),
		METHOD("delete", oPairDeleteMethod),

		METHOD_RET("headIter", oPairHeadIterMethod, RETURNS_OBJECT(kBCIPairIter)),
		METHOD_RET("tailIter", oPairTailIterMethod, RETURNS_OBJECT(kBCIPairIter)),
		//METHOD_RET( "find",                 oPairFindMethodOp, RETURNS_OBJECT(kBCIPairIter) ),
		//METHOD_RET( "clone",                oPairCloneMethodOp, RETURNS_OBJECT(kBCIPair) ),
		//METHOD_RET( "count",                oPairCountMethodOp, RETURNS_NATIVE(kBaseTypeInt) ),
		//METHOD(     "clear",                oPairClearMethod ),

        METHOD_RET("getA", oPairGetAMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("setA", oPairSetAMethod),
        METHOD_RET("getB", oPairGetBMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("setB", oPairSetBMethod),

		MEMBER_VAR("a", OBJECT_TYPE_TO_CODE(0, kBCIObject)),
		MEMBER_VAR("b", OBJECT_TYPE_TO_CODE(0, kBCIObject)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 PairIter
	//

	FORTHOP(oPairIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a PairIter object");
	}

	FORTHOP(oPairIterDeleteMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekNextMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		if (pIter->cursor < 2)
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekPrevMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekHeadMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oPairIterSeekTailMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		pIter->cursor = 2;
		METHOD_RETURN;
	}

    FORTHOP(oPairIterAtHeadMethod)
    {
        GET_THIS(oPairIterStruct, pIter);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oPairIterAtTailMethod)
    {
        GET_THIS(oPairIterStruct, pIter);
        long retVal = (pIter->cursor > 1) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oPairIterNextMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterPrevMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oPairIterCurrentMethod)
	{
		GET_THIS(oPairIterStruct, pIter);
		oPairStruct* pPair = reinterpret_cast<oPairStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			PUSH_OBJECT(pPair->a);
			SPUSH(~0);
			break;
		case 1:
			PUSH_OBJECT(pPair->b);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

    FORTHOP(oPairIterSeekMethod)
    {
        GET_THIS(oPairIterStruct, pIter);

        ulong ix = (ulong)(SPOP);
        if (ix < 2)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("PairIter:seek", ix, 2);
        }
        METHOD_RETURN;
    }

    FORTHOP(oPairIterTellMethod)
    {
        GET_THIS(oPairIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    baseMethodEntry oPairIterMembers[] =
	{
		METHOD("__newOp", oPairIterNew),
		METHOD("delete", oPairIterDeleteMethod),

		METHOD("seekNext", oPairIterSeekNextMethod),
		METHOD("seekPrev", oPairIterSeekPrevMethod),
		METHOD("seekHead", oPairIterSeekHeadMethod),
		METHOD("seekTail", oPairIterSeekTailMethod),
        METHOD_RET("atHead", oPairIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oPairIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oPairIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oPairIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oPairIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD(     "remove",				oPairIterRemoveMethod ),
		//METHOD_RET( "findNext",				oPairIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt) ),
		//METHOD_RET( "clone",                oPairIterCloneMethod, RETURNS_OBJECT(kBCIPairIter) ),

        METHOD("seek", oPairIterSeekMethod),
        METHOD_RET("tell", oPairIterTellMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIPair)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 Triple
	//

	struct oTripleStruct
	{
        long*           pMethods;
        ulong           refCount;
		ForthObject	    a;
		ForthObject	    b;
		ForthObject	    c;
	};

	struct oTripleIterStruct
	{
        long*           pMethods;
        ulong			refCount;
		ForthObject		parent;
		int				cursor;
	};


    oTripleIterStruct* createTripleIterator(ForthCoreState* pCore, oTripleStruct* pTriple)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCITripleIter);
        MALLOCATE_ITER(oTripleIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pTriple);
        return pIter;
    }

    FORTHOP(oTripleNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oTripleStruct, pTriple, pClassVocab);
        pTriple->pMethods = pClassVocab->GetMethods();
        pTriple->refCount = 0;
        pTriple->a = nullptr;
        pTriple->b = nullptr;
        pTriple->c = nullptr;
		PUSH_OBJECT(pTriple);
	}

	FORTHOP(oTripleDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oTripleStruct, pTriple);
		ForthObject& oa = pTriple->a;
		SAFE_RELEASE(pCore, oa);
		ForthObject& ob = pTriple->b;
		SAFE_RELEASE(pCore, ob);
		ForthObject& oc = pTriple->c;
		SAFE_RELEASE(pCore, oc);
		FREE_OBJECT(pTriple);
		METHOD_RETURN;
	}

    FORTHOP(oTripleHeadIterMethod)
    {
        GET_THIS(oTripleStruct, pTriple);
        pTriple->refCount++;
        TRACK_KEEP;

        oTripleIterStruct* pIter = createTripleIterator(pCore, pTriple);
        pIter->cursor = 0;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oTripleTailIterMethod)
    {
        GET_THIS(oTripleStruct, pTriple);
        pTriple->refCount++;
        TRACK_KEEP;
        oTripleIterStruct* pIter = createTripleIterator(pCore, pTriple);
        pIter->cursor = 3;

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oTripleGetAMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->a);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetAMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->a;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->a = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleGetBMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->b);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetBMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->b = newObj;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleGetCMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		PUSH_OBJECT(pTriple->c);
		METHOD_RETURN;
	}

	FORTHOP(oTripleSetCMethod)
	{
		GET_THIS(oTripleStruct, pTriple);
		ForthObject newObj;
		POP_OBJECT(newObj);
		ForthObject& oldObj = pTriple->b;
		if (OBJECTS_DIFFERENT(oldObj, newObj))
		{
			SAFE_RELEASE(pCore, oldObj);
			pTriple->c = newObj;
		}
		METHOD_RETURN;
	}

    baseMethodEntry oTripleMembers[] =
	{
		METHOD("__newOp", oTripleNew),
		METHOD("delete", oTripleDeleteMethod),

		METHOD_RET("headIter", oTripleHeadIterMethod, RETURNS_OBJECT(kBCITripleIter)),
		METHOD_RET("tailIter", oTripleTailIterMethod, RETURNS_OBJECT(kBCITripleIter)),
		//METHOD_RET( "find",                 oTripleFindMethod, RETURNS_OBJECT(kBCITripleIter) ),
		//METHOD_RET( "clone",                oTripleCloneMethod, RETURNS_OBJECT(kBCITriple) ),
		//METHOD_RET( "count",                oTripleCountMethod, RETURNS_NATIVE(kBaseTypeInt) ),
		//METHOD(     "clear",                oTripleClearMethod ),

        METHOD_RET("getA", oTripleGetAMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("setA", oTripleSetAMethod),
        METHOD_RET("getB", oTripleGetBMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("setB", oTripleSetBMethod),
        METHOD_RET("getC", oTripleGetCMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD("setC", oTripleSetCMethod),

		MEMBER_VAR("a", OBJECT_TYPE_TO_CODE(0, kBCIObject)),
		MEMBER_VAR("b", OBJECT_TYPE_TO_CODE(0, kBCIObject)),
		MEMBER_VAR("c", OBJECT_TYPE_TO_CODE(0, kBCIObject)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 TripleIter
	//

	FORTHOP(oTripleIterNew)
	{
		GET_ENGINE->SetError(kForthErrorIllegalOperation, " cannot explicitly create a TripleIter object");
	}

	FORTHOP(oTripleIterDeleteMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekNextMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		if (pIter->cursor < 3)
		{
			pIter->cursor++;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekPrevMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		if (pIter->cursor > 0)
		{
			pIter->cursor--;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekHeadMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		pIter->cursor = 0;
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterSeekTailMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		pIter->cursor = 3;
		METHOD_RETURN;
	}

    FORTHOP(oTripleIterAtHeadMethod)
    {
        GET_THIS(oTripleIterStruct, pIter);
        oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(pIter->parent);
        long retVal = (pIter->cursor == 0) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oTripleIterAtTailMethod)
    {
        GET_THIS(oTripleIterStruct, pIter);
        oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(pIter->parent);
        long retVal = (pIter->cursor >= 3) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oTripleIterNextMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 1:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor++;
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterPrevMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 1:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 2:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 3:
			pIter->cursor--;
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

	FORTHOP(oTripleIterCurrentMethod)
	{
		GET_THIS(oTripleIterStruct, pIter);
		oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(&(pIter->parent));
		switch (pIter->cursor)
		{
		case 0:
			PUSH_OBJECT(pTriple->a);
			SPUSH(~0);
			break;
		case 1:
			PUSH_OBJECT(pTriple->b);
			SPUSH(~0);
			break;
		case 2:
			PUSH_OBJECT(pTriple->c);
			SPUSH(~0);
			break;
		default:
			SPUSH(0);
			break;
		}
		METHOD_RETURN;
	}

    FORTHOP(oTripleIterSeekMethod)
    {
        GET_THIS(oTripleIterStruct, pIter);

        oTripleStruct* pTriple = reinterpret_cast<oTripleStruct *>(pIter->parent);
        ulong ix = (ulong)(SPOP);
        if (ix <= 2)
        {
            pIter->cursor = ix;
        }
        else
        {
            ReportBadArrayIndex("TripleIter:seek", ix, 3);
        }
        METHOD_RETURN;
    }

    FORTHOP(oTripleIterTellMethod)
    {
        GET_THIS(oTripleIterStruct, pIter);
        SPUSH(pIter->cursor);
        METHOD_RETURN;
    }

    baseMethodEntry oTripleIterMembers[] =
	{
		METHOD("__newOp", oTripleIterNew),
		METHOD("delete", oTripleIterDeleteMethod),

		METHOD("seekNext", oTripleIterSeekNextMethod),
		METHOD("seekPrev", oTripleIterSeekPrevMethod),
		METHOD("seekHead", oTripleIterSeekHeadMethod),
		METHOD("seekTail", oTripleIterSeekTailMethod),
		METHOD_RET("next", oTripleIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oTripleIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oTripleIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
		//METHOD(     "remove",				oTripleIterRemoveMethod ),
		//METHOD_RET( "findNext",				oTripleIterFindNextMethod, RETURNS_NATIVE(kBaseTypeInt) ),
		//METHOD_RET( "clone",                oTripleIterCloneMethod, RETURNS_OBJECT(kBCITripleIter) ),

        METHOD("seek", oTripleIterSeekMethod),
        METHOD("tell", oTripleIterTellMethod),
       
        MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCITriple)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


    void AddClasses(ForthEngine* pEngine)
	{
		gpArrayClassVocab = pEngine->AddBuiltinClass("Array", kBCIArray, kBCIIterable, oArrayMembers);
        gpArrayClassVocab->SetCustomObjectReader(customArrayReader);
        pEngine->AddBuiltinClass("ArrayIter", kBCIArrayIter, kBCIIter, oArrayIterMembers);

        ForthClassVocabulary* pVocab = pEngine->AddBuiltinClass("Bag", kBCIBag, kBCIIterable, oBagMembers);
        pVocab->SetCustomObjectReader(customBagReader);
        pEngine->AddBuiltinClass("BagIter", kBCIBagIter, kBCIIter, oBagIterMembers);

        pVocab = pEngine->AddBuiltinClass("ByteArray", kBCIByteArray, kBCIIterable, oByteArrayMembers);
        pVocab->SetCustomObjectReader(customByteArrayReader);
        pEngine->AddBuiltinClass("ByteArrayIter", kBCIByteArrayIter, kBCIIter, oByteArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("ShortArray", kBCIShortArray, kBCIIterable, oShortArrayMembers);
        pVocab->SetCustomObjectReader(customShortArrayReader);
        pEngine->AddBuiltinClass("ShortArrayIter", kBCIShortArrayIter, kBCIIter, oShortArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("IntArray", kBCIIntArray, kBCIIterable, oIntArrayMembers);
        pVocab->SetCustomObjectReader(customIntArrayReader);
        pEngine->AddBuiltinClass("IntArrayIter", kBCIIntArrayIter, kBCIIter, oIntArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("FloatArray", kBCIFloatArray, kBCIIterable, oFloatArrayMembers);
        pVocab->SetCustomObjectReader(customFloatArrayReader);
        pEngine->AddBuiltinClass("FloatArrayIter", kBCIFloatArrayIter, kBCIIter, oIntArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("LongArray", kBCILongArray, kBCIIterable, oLongArrayMembers);
        pVocab->SetCustomObjectReader(customLongArrayReader);
        pEngine->AddBuiltinClass("LongArrayIter", kBCILongArrayIter, kBCIIter, oLongArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("DoubleArray", kBCIDoubleArray, kBCIIterable, oDoubleArrayMembers);
        pVocab->SetCustomObjectReader(customDoubleArrayReader);
        pEngine->AddBuiltinClass("DoubleArrayIter", kBCIDoubleArrayIter, kBCIIter, oLongArrayIterMembers);

        pVocab = pEngine->AddBuiltinClass("StructArray", kBCIStructArray, kBCIIterable, oStructArrayMembers);
        pVocab->SetCustomObjectReader(customStructArrayReader);
        pEngine->AddBuiltinClass("StructArrayIter", kBCIStructArrayIter, kBCIIter, oStructArrayIterMembers);

        pEngine->AddBuiltinClass("Pair", kBCIPair, kBCIIterable, oPairMembers);
		pEngine->AddBuiltinClass("PairIter", kBCIPairIter, kBCIIter, oPairIterMembers);

		pEngine->AddBuiltinClass("Triple", kBCITriple, kBCIIterable, oTripleMembers);
		pEngine->AddBuiltinClass("TripleIter", kBCITripleIter, kBCIIter, oTripleIterMembers);
	}

} // namespace OArray
