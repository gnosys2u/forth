//////////////////////////////////////////////////////////////////////
//
// OMap.cpp: builtin map related classes
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

#include "OMap.h"

namespace OMap
{

	//////////////////////////////////////////////////////////////////////
	///
	//                 Map
	//

	typedef std::map<ForthObject, ForthObject> oMap;
	struct oMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oMap*	        elements;
	};

	struct oMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oMap::iterator*		cursor;
	};


    void setObjectMap(oMapStruct* pMap, ForthObject& keyObj, ForthObject& valueObj, ForthCoreState* pCore)
    {
        oMap& a = *(pMap->elements);
        oMap::iterator iter = a.find(keyObj);
        if (valueObj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, valueObj))
                {
                    SAFE_KEEP(valueObj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(keyObj);
                SAFE_KEEP(valueObj);
            }
            a[keyObj] = valueObj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                keyObj = iter->first;
                SAFE_RELEASE(pCore, keyObj);
                ForthObject& val = iter->second;
                SAFE_RELEASE(pCore, val);
                a.erase(iter);
            }
        }
    }

    bool customMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "__keys")
        {
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
                //dstArray->elements->push_back(obj);
                // TODO: release obj here?
            }
            return true;
        }
        else if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oMapStruct *dstMap = (oMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string number;
            ForthObject keyObj;
            ForthObject valueObj;
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
                reader->getObjectOrLink(&keyObj);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&valueObj);
                setObjectMap(dstMap, keyObj, valueObj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    oMapIterStruct* createMapIterator(ForthCoreState* pCore, oMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oMapIterStruct* pIter = new oMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oMap::iterator;
        return pIter;
    }

    FORTHOP(oMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
		pMap->refCount = 0;
		pMap->elements = new oMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject key = iter->first;
			SAFE_RELEASE(pCore, key);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oMapShowInnerMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        ForthObject key;

        // first, show any key objects that weren't already shown
        std::vector<ForthObject> keysToShow;
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject key = iter->first;
            if (!pShowContext->ObjectAlreadyShown(key))
            {
                keysToShow.push_back(key);
            }
        }

        pShowContext->BeginElement("__keys");
        pShowContext->BeginArray();
        for (ForthObject& keyObj : keysToShow)
        {
            pShowContext->BeginArrayElement(1);
            ForthShowObject(keyObj, pCore);
        }
        pShowContext->EndArray();

        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginIndent();
        pShowContext->BeginNestedShow();
        for (iter = a.begin(); iter != a.end(); ++iter)
		{
            ForthObject key = iter->first;
            pShowContext->AddObject(key);
            pShowContext->BeginLinkElement(key);
			ForthShowObject(iter->second, pCore);
        }
        pShowContext->EndNestedShow();
        pShowContext->EndIndent();
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
		METHOD_RETURN;
	}

    FORTHOP(oMapHeadIterMethod)
    {
        GET_THIS(oMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        oMapIterStruct* pIter = createMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oMapTailIterMethod)
    {
        GET_THIS(oMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oMapIterStruct* pIter = createMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oMapFindMethod)
    {
        GET_THIS(oMapStruct, pMap);
        long found = 0;

        oMap& a = *(pMap->elements);
        ForthObject key;
        POP_OBJECT(key);
        oMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;
            oMapIterStruct* pIter = createMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oMapCountMethod)
    {
        GET_THIS(oMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oMapStruct, pMap);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject key = iter->first;
			SAFE_RELEASE(pCore, key);
			ForthObject val = iter->second;
			SAFE_RELEASE(pCore, val);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oMapGrabMethod)
	{
		GET_THIS(oMapStruct, pMap);
        long found = 0;
		oMap& a = *(pMap->elements);
        ForthObject key;
        POP_OBJECT(key);
		oMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
            found = ~0;
		}
        SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oMapSetMethod)
	{
		GET_THIS(oMapStruct, pMap);
        ForthObject keyObj;
        POP_OBJECT(keyObj);
        ForthObject valueObj;
        POP_OBJECT(valueObj);
        setObjectMap(pMap, keyObj, valueObj, pCore);
		METHOD_RETURN;
	}

    FORTHOP(oMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oMapStruct, pMap);
        oMap::iterator iter;
        oMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject key = iter->first;
            SAFE_RELEASE(pCore, key);
            ForthObject& val = iter->second;
            SAFE_RELEASE(pCore, val);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            ForthObject key;
            POP_OBJECT(key);
            ForthObject newObj;
            POP_OBJECT(newObj);
            if (newObj != nullptr)
            {
                SAFE_KEEP(key);
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP(oMapFindValueMethod)
	{
		GET_THIS(oMapStruct, pMap);
        ForthObject retVal = nullptr;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oMap::iterator iter;
		oMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = ~0;
				retVal = iter->first;
                PUSH_OBJECT(retVal);
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oMapRemoveMethod)
	{
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		ForthObject key;
        POP_OBJECT(key);
		oMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
            ForthObject key = iter->first;
			SAFE_RELEASE(pCore, key);
			ForthObject& val = iter->second;
			SAFE_RELEASE(pCore, val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oMapStruct, pMap);
		oMap& a = *(pMap->elements);
		ForthObject key;
        POP_OBJECT(key);
        oMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject key = iter->first;
			unrefObject(key);
			ForthObject& val = iter->second;
			unrefObject(val);
			PUSH_OBJECT(val);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oMapMembers[] =
	{
		METHOD("__newOp", oMapNew),
		METHOD("delete", oMapDeleteMethod),
		METHOD("showInner", oMapShowInnerMethod),

		METHOD_RET("headIter", oMapHeadIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("tailIter", oMapTailIterMethod, RETURNS_OBJECT(kBCIMapIter)),
		METHOD_RET("find", oMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oMapClearMethod),

        METHOD_RET("grab", oMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oMapSetMethod),
        METHOD("load", oMapLoadMethod),
        METHOD_RET("findValue", oMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oMapRemoveMethod),
		METHOD("unref", oMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 MapIter
	//

	FORTHOP(oMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a MapIter object");
	}

	FORTHOP(oMapIterDeleteMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekHeadMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oMapIterSeekTailMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oMapIterAtHeadMethod)
    {
        GET_THIS(oMapIterStruct, pIter);
        oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oMapIterAtTailMethod)
    {
        GET_THIS(oMapIterStruct, pIter);
        oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oMapIterNextMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterPrevMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oMapIterCurrentMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oMapIterRemoveMethod)
	{
		GET_THIS(oMapIterStruct, pIter);
		oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
		if ((*pIter->cursor) != pMap->elements->end())
		{
            ForthObject key = (*pIter->cursor)->first;
			SAFE_RELEASE(pCore, key);
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

    FORTHOP(oMapIterCurrentPairMethod)
    {
        GET_THIS(oMapIterStruct, pIter);
        oMapStruct* pMap = reinterpret_cast<oMapStruct *>(pIter->parent);
        if ((*pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            ForthObject& o = (*pIter->cursor)->second;
            PUSH_OBJECT(o);
            ForthObject key = (*pIter->cursor)->first;
            PUSH_OBJECT(key);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }


    baseMethodEntry oMapIterMembers[] =
	{
		METHOD("__newOp", oMapIterNew),
		METHOD("delete", oMapIterDeleteMethod),

		METHOD("seekNext", oMapIterSeekNextMethod),
		METHOD("seekPrev", oMapIterSeekPrevMethod),
		METHOD("seekHead", oMapIterSeekHeadMethod),
		METHOD("seekTail", oMapIterSeekTailMethod),
        METHOD_RET("atHead", oMapIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oMapIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oMapIterRemoveMethod),

        METHOD_RET("currentPair", oMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 IntMap
	//

	typedef std::map<long, ForthObject> oIntMap;
	struct oIntMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oIntMap*		elements;
	};

	struct oIntMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oIntMap::iterator	*cursor;
	};


    void setIntMap(oIntMapStruct* pMap, int key, ForthObject& newObj, ForthCoreState* pCore)
    {
        oIntMap& a = *(pMap->elements);
        oIntMap::iterator iter = a.find(key);
        if (newObj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, newObj))
                {
                    SAFE_KEEP(newObj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                ForthObject& oldObj = iter->second;
                SAFE_RELEASE(pCore, oldObj);
                a.erase(iter);
            }
        }
    }

    bool customIntMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oIntMapStruct *dstMap = (oIntMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string number;
            ForthObject obj;
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
                reader->getString(number);
                int key;
                sscanf(number.c_str(), "%d", &key);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&obj);
                setIntMap(dstMap, key, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    oIntMapIterStruct* createIntMapIterator(ForthCoreState* pCore, oIntMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIIntMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oIntMapIterStruct* pIter = new oIntMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oIntMap::iterator;
        return pIter;
    }

    FORTHOP(oIntMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oIntMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
		pMap->elements = new oIntMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oIntMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapShowInnerMethod)
	{
		char buffer[20];
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
		{
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
				sprintf(buffer, "%d", iter->first);
				pShowContext->BeginElement(buffer);
				ForthShowObject(iter->second, pCore);
                pShowContext->EndElement();
            }
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
		METHOD_RETURN;
	}

    FORTHOP(oIntMapHeadIterMethod)
    {
        GET_THIS(oIntMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oIntMapIterStruct* pIter = createIntMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oIntMapTailIterMethod)
    {
        GET_THIS(oIntMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        oIntMapIterStruct* pIter = createIntMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oIntMapFindMethod)
    {
        GET_THIS(oIntMapStruct, pMap);
        long found = 0;

        oIntMap& a = *(pMap->elements);
        long key = SPOP;
        oIntMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;
            oIntMapIterStruct* pIter = createIntMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oIntMapCountMethod)
    {
        GET_THIS(oIntMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oIntMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oIntMapStruct, pMap);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oIntMapGrabMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
        long found = 0;

        oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject fobj = iter->second;
			PUSH_OBJECT(fobj);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

    FORTHOP(oIntMapSetMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
        long key = SPOP;
        ForthObject newObj;
        POP_OBJECT(newObj);
        setIntMap(pMap, key, newObj, pCore);
		METHOD_RETURN;
	}

    FORTHOP(oIntMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oIntMapStruct, pMap);
        oIntMap::iterator iter;
        oIntMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            long key = SPOP;
            ForthObject newObj;
            POP_OBJECT(newObj);
            if (newObj != nullptr)
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP(oIntMapFindValueMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oIntMap::iterator iter;
		oIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
                found = ~0;
                SPUSH(iter->first);
				break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oIntMapRemoveMethod)
	{
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oIntMapStruct, pMap);
		oIntMap& a = *(pMap->elements);
		long key = SPOP;
		oIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oIntMapMembers[] =
	{
		METHOD("__newOp", oIntMapNew),
		METHOD("delete", oIntMapDeleteMethod),
		METHOD("showInner", oIntMapShowInnerMethod),

		METHOD_RET("headIter", oIntMapHeadIterMethod, RETURNS_OBJECT(kBCIIntMapIter)),
		METHOD_RET("tailIter", oIntMapTailIterMethod, RETURNS_OBJECT(kBCIIntMapIter)),
		METHOD_RET("find", oIntMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oIntMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oIntMapClearMethod),

        METHOD_RET("grab", oIntMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oIntMapSetMethod),
        METHOD("load", oIntMapLoadMethod),
        METHOD_RET("findValue", oIntMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oIntMapRemoveMethod),
		METHOD("unref", oIntMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 IntMapIter
	//

	FORTHOP(oIntMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create an IntMapIter object");
	}

	FORTHOP(oIntMapIterDeleteMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekNextMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekPrevMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekHeadMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterSeekTailMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oIntMapIterAtHeadMethod)
    {
        GET_THIS(oIntMapIterStruct, pIter);
        oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oIntMapIterAtTailMethod)
    {
        GET_THIS(oIntMapIterStruct, pIter);
        oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oIntMapIterNextMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterPrevMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oIntMapIterCurrentMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oIntMapIterRemoveMethod)
	{
		GET_THIS(oIntMapIterStruct, pIter);
		oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

    FORTHOP(oIntMapIterCurrentPairMethod)
    {
        GET_THIS(oIntMapIterStruct, pIter);
        oIntMapStruct* pMap = reinterpret_cast<oIntMapStruct *>(pIter->parent);
        if ((*pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            ForthObject& o = (*pIter->cursor)->second;
            PUSH_OBJECT(o);
            SPUSH((*pIter->cursor)->first);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }


	baseMethodEntry oIntMapIterMembers[] =
	{
		METHOD("__newOp", oIntMapIterNew),
		METHOD("delete", oIntMapIterDeleteMethod),

		METHOD("seekNext", oIntMapIterSeekNextMethod),
		METHOD("seekPrev", oIntMapIterSeekPrevMethod),
		METHOD("seekHead", oIntMapIterSeekHeadMethod),
		METHOD("seekTail", oIntMapIterSeekTailMethod),
        METHOD_RET("atHead", oIntMapIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oIntMapIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oIntMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oIntMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oIntMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oIntMapIterRemoveMethod),

        METHOD_RET("currentPair", oIntMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIIntMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 FloatMap
	//

	typedef std::map<float, ForthObject> oFloatMap;
	struct oFloatMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oFloatMap*		elements;
	};

	struct oFloatMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oFloatMap::iterator	*cursor;
	};


    void setFloatMap(oFloatMapStruct* pMap, float key, ForthObject& newObj, ForthCoreState* pCore)
    {
        oFloatMap& a = *(pMap->elements);
        oFloatMap::iterator iter = a.find(key);
        if (newObj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, newObj))
                {
                    SAFE_KEEP(newObj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                ForthObject& oldObj = iter->second;
                SAFE_RELEASE(pCore, oldObj);
                a.erase(iter);
            }
        }
    }

    bool customFloatMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oFloatMapStruct *dstMap = (oFloatMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string number;
            ForthObject obj;
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
                reader->getString(number);
                float key;
                sscanf(number.c_str(), "%f", &key);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&obj);
                setFloatMap(dstMap, key, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    oFloatMapIterStruct* createFloatMapIterator(ForthCoreState* pCore, oFloatMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIFloatMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oFloatMapIterStruct* pIter = new oFloatMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oFloatMap::iterator;
        return pIter;
    }

    FORTHOP(oFloatMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oFloatMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
		pMap->elements = new oFloatMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oFloatMapShowInnerMethod)
	{
		char buffer[20];
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                float fval = *((float *)(&(iter->first)));
                sprintf(buffer, "%f", fval);
                pShowContext->BeginElement(buffer);
                ForthShowObject(iter->second, pCore);
                pShowContext->EndElement();
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oFloatMapHeadIterMethod)
    {
        GET_THIS(oFloatMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oFloatMapIterStruct* pIter = createFloatMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oFloatMapTailIterMethod)
    {
        GET_THIS(oFloatMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        oFloatMapIterStruct* pIter = createFloatMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oFloatMapFindMethod)
    {
        GET_THIS(oFloatMapStruct, pMap);
        long found = 0;

        oFloatMap& a = *(pMap->elements);
        float key = FPOP;
        oFloatMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;
            oFloatMapIterStruct* pIter = createFloatMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

	FORTHOP(oFloatMapGrabMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
        long found = 0;

        oFloatMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            ForthObject fobj = iter->second;
            PUSH_OBJECT(fobj);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

	FORTHOP(oFloatMapSetMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		float key = FPOP;
		ForthObject newObj;
		POP_OBJECT(newObj);
        setFloatMap(pMap, key, newObj, pCore);
		METHOD_RETURN;
	}

    FORTHOP(oFloatMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oFloatMapStruct, pMap);
        oFloatMap::iterator iter;
        oFloatMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            float key = FPOP;
            ForthObject newObj;
            POP_OBJECT(newObj);
            if (newObj != nullptr)
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        METHOD_RETURN;
    }

	FORTHOP(oFloatMapFindValueMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oFloatMap::iterator iter;
		oFloatMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = ~0;
				FPUSH(iter->first);
				break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapRemoveMethod)
	{
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		oFloatMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oFloatMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oFloatMapStruct, pMap);
		oFloatMap& a = *(pMap->elements);
		float key = FPOP;
		oFloatMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oFloatMapMembers[] =
	{
		METHOD("__newOp", oFloatMapNew),
		METHOD("showInner", oFloatMapShowInnerMethod),

		METHOD_RET("headIter", oFloatMapHeadIterMethod, RETURNS_OBJECT(kBCIFloatMapIter)),
		METHOD_RET("tailIter", oFloatMapTailIterMethod, RETURNS_OBJECT(kBCIFloatMapIter)),
		METHOD_RET("find", oFloatMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),

        METHOD_RET("grab", oFloatMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oFloatMapSetMethod),
        METHOD("load", oFloatMapLoadMethod),
        METHOD_RET("findValue", oFloatMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oFloatMapRemoveMethod),
		METHOD("unref", oFloatMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongMap
	//

    ForthClassVocabulary* gpLongMapClassVocab = nullptr;

    oLongMapStruct* createLongMapObject(ForthClassVocabulary *pClassVocab)
    {
        MALLOCATE_OBJECT(oLongMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
        pMap->elements = new oLongMap;
        return pMap;
    }

    void setLongMap(oLongMapStruct* pMap, stackInt64& key, ForthObject& newObj, ForthCoreState* pCore)
    {
        oLongMap& a = *(pMap->elements);
        oLongMap::iterator iter = a.find(key.s64);
        if (newObj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, newObj))
                {
                    SAFE_KEEP(newObj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(newObj);
            }
            a[key.s64] = newObj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                ForthObject& oldObj = iter->second;
                SAFE_RELEASE(pCore, oldObj);
                a.erase(iter);
            }
        }
    }

    bool customLongMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oLongMapStruct *dstMap = (oLongMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string number;
            ForthObject obj;
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
                reader->getString(number);
                stackInt64 key;
                sscanf(number.c_str(), "%lld", &key.s64);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&obj);
                setLongMap(dstMap, key, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    oLongMapIterStruct* createLongMapIterator(ForthCoreState* pCore, oLongMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCILongMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oLongMapIterStruct* pIter = new oLongMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oLongMap::iterator;
        return pIter;
    }

    FORTHOP(oLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
        ForthObject newMap = (ForthObject)createLongMapObject(pClassVocab);
        PUSH_OBJECT(newMap);
	}

	FORTHOP(oLongMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapShowInnerMethod)
	{
        char buffer[32];
        GET_THIS(oLongMapStruct, pMap);
        oLongMap::iterator iter;
        oLongMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                sprintf(buffer, "%lld", iter->first);
                pShowContext->BeginElement(buffer);
                ForthShowObject(iter->second, pCore);
                pShowContext->EndElement();
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oLongMapHeadIterMethod)
    {
        GET_THIS(oLongMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oLongMapIterStruct* pIter = createLongMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oLongMapTailIterMethod)
    {
        GET_THIS(oLongMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        oLongMapIterStruct* pIter = createLongMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oLongMapFindMethod)
    {
        GET_THIS(oLongMapStruct, pMap);
        long found = 0;

        oLongMap& a = *(pMap->elements);
        stackInt64 key;
        LPOP(key);
        oLongMap::iterator iter = a.find(key.s64);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;
            oLongMapIterStruct* pIter = createLongMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oLongMapCountMethod)
    {
        GET_THIS(oLongMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oLongMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oLongMapStruct, pMap);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapGrabMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
		stackInt64 key;
		LPOP(key);
        long found = 0;

        oLongMap::iterator iter = a.find(key.s64);
        if (iter != a.end())
        {
            ForthObject fobj = iter->second;
            PUSH_OBJECT(fobj);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

	FORTHOP(oLongMapSetMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
        stackInt64 key;
        LPOP(key);
        ForthObject newObj;
		POP_OBJECT(newObj);
        setLongMap(pMap, key, newObj, pCore);
		METHOD_RETURN;
	}

    FORTHOP(oLongMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oLongMapStruct, pMap);
        oLongMap::iterator iter;
        oLongMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            stackInt64 key;
            LPOP(key);
            ForthObject newObj;
            POP_OBJECT(newObj);
            if (newObj != nullptr)
            {
                SAFE_KEEP(newObj);
            }
            a[key.s64] = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP(oLongMapFindValueMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		stackInt64 retVal;
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oLongMap::iterator iter;
		oLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = ~0;
				retVal.s64 = iter->first;
                LPUSH(retVal);
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oLongMapRemoveMethod)
	{
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
        stackInt64 key;
        LPOP(key);
        oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oLongMapStruct, pMap);
		oLongMap& a = *(pMap->elements);
        stackInt64 key;
        LPOP(key);
        oLongMap::iterator iter = a.find(key.s64);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oLongMapMembers[] =
	{
		METHOD("__newOp", oLongMapNew),
		METHOD("delete", oLongMapDeleteMethod),
		METHOD("showInner", oLongMapShowInnerMethod),

		METHOD_RET("headIter", oLongMapHeadIterMethod, RETURNS_OBJECT(kBCILongMapIter)),
		METHOD_RET("tailIter", oLongMapTailIterMethod, RETURNS_OBJECT(kBCILongMapIter)),
		METHOD_RET("find", oLongMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oLongMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oLongMapClearMethod),

        METHOD_RET("grab", oLongMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oLongMapSetMethod),
        METHOD("load", oLongMapLoadMethod),
        METHOD_RET("findValue", oLongMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oLongMapRemoveMethod),
		METHOD("unref", oLongMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 LongMapIter
	//

	FORTHOP(oLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a LongMapIter object");
	}

	FORTHOP(oLongMapIterDeleteMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekHeadMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterSeekTailMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oLongMapIterAtHeadMethod)
    {
        GET_THIS(oLongMapIterStruct, pIter);
        oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oLongMapIterAtTailMethod)
    {
        GET_THIS(oLongMapIterStruct, pIter);
        oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oLongMapIterNextMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterPrevMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oLongMapIterCurrentMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oLongMapIterRemoveMethod)
	{
		GET_THIS(oLongMapIterStruct, pIter);
		oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

    FORTHOP(oLongMapIterCurrentPairMethod)
    {
        GET_THIS(oLongMapIterStruct, pIter);
        oLongMapStruct* pMap = reinterpret_cast<oLongMapStruct *>(pIter->parent);
        if ((*pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            ForthObject& o = (*pIter->cursor)->second;
            PUSH_OBJECT(o);
            stackInt64 key;
            key.s64 = (*pIter->cursor)->first;
            LPUSH(key);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }


	baseMethodEntry oLongMapIterMembers[] =
	{
		METHOD("__newOp", oLongMapIterNew),
		METHOD("delete", oLongMapIterDeleteMethod),

		METHOD("seekNext", oLongMapIterSeekNextMethod),
		METHOD("seekPrev", oLongMapIterSeekPrevMethod),
		METHOD("seekHead", oLongMapIterSeekHeadMethod),
		METHOD("seekTail", oLongMapIterSeekTailMethod),
        METHOD_RET("atHead", oLongMapIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oLongMapIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oLongMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oLongMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oLongMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oLongMapIterRemoveMethod),

        METHOD_RET("currentPair", oLongMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
        
        MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCILongMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 DoubleMap
	//

	typedef std::map<double, ForthObject> oDoubleMap;
	struct oDoubleMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oDoubleMap*	    elements;
	};

	struct oDoubleMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oDoubleMap::iterator*	cursor;
	};


    void setDoubleMap(oDoubleMapStruct* pMap, double key, ForthObject& newObj, ForthCoreState* pCore)
    {
        oDoubleMap& a = *(pMap->elements);
        oDoubleMap::iterator iter = a.find(key);
        if (newObj != nullptr)
        {
            if (iter != a.end())
            {
                ForthObject oldObj = iter->second;
                if (OBJECTS_DIFFERENT(oldObj, newObj))
                {
                    SAFE_KEEP(newObj);
                    SAFE_RELEASE(pCore, oldObj);
                }
            }
            else
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        else
        {
            // remove element associated with key from map
            if (iter != a.end())
            {
                ForthObject& oldObj = iter->second;
                SAFE_RELEASE(pCore, oldObj);
                a.erase(iter);
            }
        }
    }

    bool customDoubleMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oDoubleMapStruct *dstMap = (oDoubleMapStruct *)(reader->getCustomReaderContext().pData);
            reader->getRequiredChar('{');
            std::string number;
            ForthObject obj;
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
                reader->getString(number);
                double key;
                sscanf(number.c_str(), "%lf", &key);
                reader->getRequiredChar(':');
                reader->getObjectOrLink(&obj);
                setDoubleMap(dstMap, key, obj, pCore);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    oDoubleMapIterStruct* createDoubleMapIterator(ForthCoreState* pCore, oDoubleMapStruct* pMap, ForthObject& obj)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIDoubleMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oDoubleMapIterStruct* pIter = new oDoubleMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oDoubleMap::iterator;
        return pIter;
    }

    FORTHOP(oDoubleMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oDoubleMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
		pMap->elements = new oDoubleMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oDoubleMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapShowInnerMethod)
	{
        char buffer[64];
        GET_THIS(oDoubleMapStruct, pMap);
        oDoubleMap::iterator iter;
        oDoubleMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                sprintf(buffer, "%g", iter->first);
                pShowContext->BeginElement(buffer);
                ForthShowObject(iter->second, pCore);
                pShowContext->EndElement();
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapHeadIterMethod)
    {
        GET_THIS(oDoubleMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        ForthObject obj;

        oDoubleMapIterStruct* pIter = createDoubleMapIterator(pCore, pMap, obj);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(obj);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapTailIterMethod)
    {
        GET_THIS(oDoubleMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;
        ForthObject obj;

        oDoubleMapIterStruct* pIter = createDoubleMapIterator(pCore, pMap, obj);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(obj);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapFindMethod)
    {
        GET_THIS(oDoubleMapStruct, pMap);
        long found = 0;

        oDoubleMap& a = *(pMap->elements);
        double key = DPOP;
        oDoubleMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;
            ForthObject obj;

            oDoubleMapIterStruct* pIter = createDoubleMapIterator(pCore, pMap, obj);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(obj);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapCountMethod)
    {
        GET_THIS(oDoubleMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			SAFE_RELEASE(pCore, o);
		}
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapGrabMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
        long found = 0;

        oDoubleMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            ForthObject fobj = iter->second;
            PUSH_OBJECT(fobj);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

	FORTHOP(oDoubleMapSetMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		double key = DPOP;
		ForthObject newObj;
		POP_OBJECT(newObj);
        setDoubleMap(pMap, key, newObj, pCore);
		METHOD_RETURN;
	}

    FORTHOP(oDoubleMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oDoubleMapStruct, pMap);
        oDoubleMap::iterator iter;
        oDoubleMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = iter->second;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            double key = DPOP;
            ForthObject newObj;
            POP_OBJECT(newObj);
            if (newObj != nullptr)
            {
                SAFE_KEEP(newObj);
            }
            a[key] = newObj;
        }
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapFindValueMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		long found = 0;
		ForthObject soughtObj;
		POP_OBJECT(soughtObj);
		oDoubleMap::iterator iter;
		oDoubleMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			ForthObject& o = iter->second;
			if (OBJECTS_SAME(o, soughtObj))
			{
				found = ~0;
                DPUSH(iter->first);
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapRemoveMethod)
	{
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		oDoubleMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& oldObj = iter->second;
			SAFE_RELEASE(pCore, oldObj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapUnrefMethod)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		GET_THIS(oDoubleMapStruct, pMap);
		oDoubleMap& a = *(pMap->elements);
		double key = DPOP;
		oDoubleMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			ForthObject& fobj = iter->second;
			unrefObject(fobj);
			PUSH_OBJECT(fobj);
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oDoubleMapMembers[] =
	{
		METHOD("__newOp", oDoubleMapNew),
		METHOD("delete", oDoubleMapDeleteMethod),
		METHOD("showInner", oDoubleMapShowInnerMethod),

		METHOD_RET("headIter", oDoubleMapHeadIterMethod, RETURNS_OBJECT(kBCIDoubleMapIter)),
		METHOD_RET("tailIter", oDoubleMapTailIterMethod, RETURNS_OBJECT(kBCIDoubleMapIter)),
		METHOD_RET("find", oDoubleMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oDoubleMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oDoubleMapClearMethod),

        METHOD_RET("grab", oDoubleMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oDoubleMapSetMethod),
        METHOD("load", oDoubleMapLoadMethod),
        METHOD_RET("findValue", oDoubleMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oDoubleMapRemoveMethod),
		METHOD("unref", oDoubleMapUnrefMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 DoubleMapIter
	//

	FORTHOP(oDoubleMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a DoubleMapIter object");
	}

	FORTHOP(oDoubleMapIterDeleteMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekNextMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekPrevMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekHeadMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterSeekTailMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oDoubleMapIterAtHeadMethod)
    {
        GET_THIS(oDoubleMapIterStruct, pIter);
        oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapIterAtTailMethod)
    {
        GET_THIS(oDoubleMapIterStruct, pIter);
        oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oDoubleMapIterNextMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterPrevMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			(*pIter->cursor)--;
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oDoubleMapIterCurrentMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		if ((*pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			ForthObject& o = (*(pIter->cursor))->second;
			PUSH_OBJECT(o);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oDoubleMapIterRemoveMethod)
	{
		GET_THIS(oDoubleMapIterStruct, pIter);
		oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
		if ((*pIter->cursor) != pMap->elements->end())
		{
			ForthObject& o = (*pIter->cursor)->second;
			SAFE_RELEASE(pCore, o);
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}

    FORTHOP(oDoubleMapIterCurrentPairMethod)
    {
        GET_THIS(oDoubleMapIterStruct, pIter);
        oDoubleMapStruct* pMap = reinterpret_cast<oDoubleMapStruct *>(pIter->parent);
        if ((*pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            ForthObject& o = (*pIter->cursor)->second;
            PUSH_OBJECT(o);
            double key = (*pIter->cursor)->first;
            DPUSH(key);
            SPUSH(~0);
        }
        METHOD_RETURN;
    }


    baseMethodEntry oDoubleMapIterMembers[] =
	{
		METHOD("__newOp", oDoubleMapIterNew),
		METHOD("delete", oDoubleMapIterDeleteMethod),

		METHOD("seekNext", oDoubleMapIterSeekNextMethod),
		METHOD("seekPrev", oDoubleMapIterSeekPrevMethod),
		METHOD("seekHead", oDoubleMapIterSeekHeadMethod),
		METHOD("seekTail", oDoubleMapIterSeekTailMethod),
		METHOD_RET("next", oDoubleMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oDoubleMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oDoubleMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oDoubleMapIterRemoveMethod),

        METHOD_RET("currentPair", oDoubleMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIDoubleMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringIntMap
	//

	typedef std::map<std::string, int> oStringIntMap;

	struct oStringIntMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oStringIntMap*	elements;
	};

	struct oStringIntMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oStringIntMap::iterator	*cursor;
	};



    bool customStringIntMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oStringIntMapStruct *dstMap = (oStringIntMapStruct *)(reader->getCustomReaderContext().pData);
            oStringIntMap& a = *(dstMap->elements);
            reader->getRequiredChar('{');
            std::string number;
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
                std::string key;
                reader->getString(key);
                reader->getRequiredChar(':');
                reader->getNumber(number);
                std::string number;
                int value;
                sscanf(number.c_str(), "%d", &value);
                a[key] = value;
            }
            return true;
        }
        return false;
    }

    oStringIntMapIterStruct* createStringIntMapIterator(ForthCoreState* pCore, oStringIntMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIStringIntMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oStringIntMapIterStruct* pIter = new oStringIntMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oStringIntMap::iterator;
        return pIter;
    }

    FORTHOP(oStringIntMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oStringIntMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
		pMap->elements = new oStringIntMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oStringIntMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringIntMapStruct, pMap);
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapShowInnerMethod)
	{
		char buffer[20];
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		ForthEngine *pEngine = ForthEngine::GetInstance();
		ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
		{
			pShowContext->BeginIndent();
			for (iter = a.begin(); iter != a.end(); ++iter)
			{
                pShowContext->BeginElement(iter->first.c_str());
				sprintf(buffer, "%d", iter->second);
                pShowContext->EndElement(buffer);
            }
			pShowContext->EndIndent();
			pShowContext->ShowIndent();
		}
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapHeadIterMethod)
    {
        GET_THIS(oStringIntMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oStringIntMapIterStruct* pIter = createStringIntMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapTailIterMethod)
    {
        GET_THIS(oStringIntMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oStringIntMapIterStruct* pIter = createStringIntMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapFindMethod)
    {
        GET_THIS(oStringIntMapStruct, pMap);
        long found = 0;

        oStringIntMap& a = *(pMap->elements);
        std::string key;
        key = (const char*)(SPOP);
        oStringIntMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;

            oStringIntMapIterStruct* pIter = createStringIntMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapCountMethod)
    {
        GET_THIS(oStringIntMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapGrabMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
        long found = 0;

        oStringIntMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            SPUSH(iter->second);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

	FORTHOP(oStringIntMapSetMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		int newValue = SPOP;
		a[key] = newValue;
		METHOD_RETURN;
	}

    FORTHOP(oStringIntMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oStringIntMapStruct, pMap);
        oStringIntMap::iterator iter;
        oStringIntMap& a = *(pMap->elements);
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            std::string key;
            key = (const char*)(SPOP);
            int val = SPOP;
            a[key] = val;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapFindValueMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		long found = 0;
		int soughtVal = SPOP;

		oStringIntMap::iterator iter;
		oStringIntMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (iter->second == soughtVal)
			{
				found = ~0;
                SPUSH(((long)(iter->first.c_str())));
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapRemoveMethod)
	{
		GET_THIS(oStringIntMapStruct, pMap);
		oStringIntMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringIntMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringIntMapMembers[] =
	{
		METHOD("__newOp", oStringIntMapNew),
		METHOD("delete", oStringIntMapDeleteMethod),
		METHOD("showInner", oStringIntMapShowInnerMethod),

		METHOD_RET("headIter", oStringIntMapHeadIterMethod, RETURNS_OBJECT(kBCIStringIntMapIter)),
		METHOD_RET("tailIter", oStringIntMapTailIterMethod, RETURNS_OBJECT(kBCIStringIntMapIter)),
		METHOD_RET("find", oStringIntMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oStringIntMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oStringIntMapClearMethod),

		METHOD_RET("grab", oStringIntMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oStringIntMapSetMethod),
        METHOD("load", oStringIntMapLoadMethod),
        METHOD_RET("findValue", oStringIntMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringIntMapRemoveMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringIntMapIter
	//

	FORTHOP(oStringIntMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a StringIntMapIter object");
	}

	FORTHOP(oStringIntMapIterDeleteMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekNextMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekPrevMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekHeadMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterSeekTailMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

    FORTHOP(oStringIntMapIterAtHeadMethod)
    {
        GET_THIS(oStringIntMapIterStruct, pIter);
        oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->begin()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapIterAtTailMethod)
    {
        GET_THIS(oStringIntMapIterStruct, pIter);
        oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
        long retVal = (*(pIter->cursor) == pMap->elements->end()) ? ~0 : 0;
        SPUSH(retVal);
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapIterNextMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterPrevMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringIntMapIterCurrentMethod)
	{
		GET_THIS(oStringIntMapIterStruct, pIter);
		oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			int val = (*(pIter->cursor))->second;
			SPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oStringIntMapIterRemoveMethod)
    {
        GET_THIS(oStringIntMapIterStruct, pIter);
        oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
        if (*(pIter->cursor) != pMap->elements->end())
        {
            pMap->elements->erase((*pIter->cursor));
            (*pIter->cursor)++;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringIntMapIterCurrentPairMethod)
    {
        GET_THIS(oStringIntMapIterStruct, pIter);
        oStringIntMapStruct* pMap = reinterpret_cast<oStringIntMapStruct *>(pIter->parent);
        if (*(pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            int val = (*(pIter->cursor))->second;
            SPUSH(val);
            SPUSH((long)(*(pIter->cursor))->first.c_str());
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

	baseMethodEntry oStringIntMapIterMembers[] =
	{
		METHOD("__newOp", oStringIntMapIterNew),
		METHOD("delete", oStringIntMapIterDeleteMethod),

		METHOD("seekNext", oStringIntMapIterSeekNextMethod),
		METHOD("seekPrev", oStringIntMapIterSeekPrevMethod),
		METHOD("seekHead", oStringIntMapIterSeekHeadMethod),
		METHOD("seekTail", oStringIntMapIterSeekTailMethod),
        METHOD_RET("atHead", oStringIntMapIterAtHeadMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("atTail", oStringIntMapIterAtTailMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("next", oStringIntMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oStringIntMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oStringIntMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oStringIntMapIterRemoveMethod),

        METHOD_RET("currentPair", oStringIntMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
        
        MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStringIntMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};

	//////////////////////////////////////////////////////////////////////
	///
	//                 StringFloatMap
	//

    bool customStringFloatMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oStringIntMapStruct *dstMap = (oStringIntMapStruct *)(reader->getCustomReaderContext().pData);
            oStringIntMap& a = *(dstMap->elements);
            reader->getRequiredChar('{');
            std::string number;
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
                std::string key;
                reader->getString(key);
                reader->getRequiredChar(':');
                reader->getNumber(number);
                std::string number;
                float value;
                sscanf(number.c_str(), "%f", &value);
                a[key] = *((int *)&value);
            }
            return true;
        }
        return false;
    }

    FORTHOP(oStringFloatMapShowInnerMethod)
    {
        char buffer[20];
        GET_THIS(oStringIntMapStruct, pMap);
        oStringIntMap::iterator iter;
        oStringIntMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                pShowContext->BeginElement(iter->first.c_str());
                float fval = *((float *)&(iter->second));
                sprintf(buffer, "%f", fval);
                pShowContext->EndElement(buffer);
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }


	baseMethodEntry oStringFloatMapMembers[] =
	{
		METHOD("__newOp", oStringIntMapNew),
		METHOD("showInner", oStringFloatMapShowInnerMethod),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringLongMap
	//

	typedef std::map<std::string, long long> oStringLongMap;

	struct oStringLongMapStruct
	{
        long*           pMethods;
        ulong           refCount;
		oStringLongMap*	elements;
	};

	struct oStringLongMapIterStruct
	{
        long*               pMethods;
        ulong				refCount;
		ForthObject			parent;
		oStringLongMap::iterator	*cursor;
	};



    bool customStringLongMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oStringLongMapStruct *dstMap = (oStringLongMapStruct *)(reader->getCustomReaderContext().pData);
            oStringLongMap& a = *(dstMap->elements);
            reader->getRequiredChar('{');
            std::string number;
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
                std::string key;
                reader->getString(key);
                reader->getRequiredChar(':');
                reader->getNumber(number);
                std::string number;
                long long value;
                sscanf(number.c_str(), "%lld", &value);
                a[key] = *((int *)&value);
            }
            return true;
        }
        return false;
    }

    oStringLongMapIterStruct* createStringLongMapIterator(ForthCoreState* pCore, oStringLongMapStruct* pMap)
    {
        ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIStringLongMapIter);
        // needed to use new instead of malloc otherwise the iterator isn't setup right and
        //   a crash happens when you assign to it
        oStringLongMapIterStruct* pIter = new oStringLongMapIterStruct;
        TRACK_ITER_NEW;
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pMap);
        pIter->cursor = new oStringLongMap::iterator;
        return pIter;
    }

    FORTHOP(oStringLongMapNew)
	{
		ForthClassVocabulary *pClassVocab = (ForthClassVocabulary *)(SPOP);
		MALLOCATE_OBJECT(oStringLongMapStruct, pMap, pClassVocab);
        pMap->pMethods = pClassVocab->GetMethods();
        pMap->refCount = 0;
		pMap->elements = new oStringLongMap;
		PUSH_OBJECT(pMap);
	}

	FORTHOP(oStringLongMapDeleteMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringLongMapStruct, pMap);
		delete pMap->elements;
		FREE_OBJECT(pMap);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapShowInnerMethod)
	{
        char buffer[32];
        GET_THIS(oStringLongMapStruct, pMap);
        oStringLongMap::iterator iter;
        oStringLongMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                pShowContext->BeginElement(iter->first.c_str());
                sprintf(buffer, "%lld", iter->second);
                pShowContext->EndElement(buffer);
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapHeadIterMethod)
    {
        GET_THIS(oStringLongMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oStringLongMapIterStruct* pIter = createStringLongMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->begin();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapTailIterMethod)
    {
        GET_THIS(oStringLongMapStruct, pMap);
        pMap->refCount++;
        TRACK_KEEP;

        oStringLongMapIterStruct* pIter = createStringLongMapIterator(pCore, pMap);
        *(pIter->cursor) = pMap->elements->end();

        PUSH_OBJECT(pIter);
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapFindMethod)
    {
        GET_THIS(oStringLongMapStruct, pMap);
        long found = 0;

        oStringLongMap& a = *(pMap->elements);
        std::string key;
        key = (const char*)(SPOP);
        oStringLongMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            pMap->refCount++;
            TRACK_KEEP;

            oStringLongMapIterStruct* pIter = createStringLongMapIterator(pCore, pMap);
            *(pIter->cursor) = iter;

            PUSH_OBJECT(pIter);
            found = ~0;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapCountMethod)
    {
        GET_THIS(oStringLongMapStruct, pMap);
        SPUSH((long)(pMap->elements->size()));
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapClearMethod)
	{
		// go through all elements and release any which are not null
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		a.clear();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapGrabMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		stackInt64 val;
        long found = 0;

        oStringLongMap::iterator iter = a.find(key);
        if (iter != a.end())
        {
            val.s64 = iter->second;
            LPUSH(val);
            found = ~0;
        }

        SPUSH(found);
        METHOD_RETURN;
	}

	FORTHOP(oStringLongMapSetMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		stackInt64 newValue;
		LPOP(newValue);
		a[key] = newValue.s64;
		METHOD_RETURN;
	}

    FORTHOP(oStringLongMapLoadMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oStringLongMapStruct, pMap);
        oStringLongMap::iterator iter;
        oStringLongMap& a = *(pMap->elements);
        a.clear();
        int n = SPOP;
        for (int i = 0; i < n; i++)
        {
            std::string key;
            key = (const char*)(SPOP);
            stackInt64 val;
            LPOP(val);
            a[key] = val.s64;
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapFindValueMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		long found = 0;
		stackInt64 soughtVal;
		LPOP(soughtVal);

		oStringLongMap::iterator iter;
		oStringLongMap& a = *(pMap->elements);
		for (iter = a.begin(); iter != a.end(); ++iter)
		{
			if (iter->second == soughtVal.s64)
			{
				found = ~0;
                SPUSH(((long)(iter->first.c_str())));
                break;
			}
		}
		SPUSH(found);
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapRemoveMethod)
	{
		GET_THIS(oStringLongMapStruct, pMap);
		oStringLongMap& a = *(pMap->elements);
		std::string key;
		key = (const char*)(SPOP);
		oStringLongMap::iterator iter = a.find(key);
		if (iter != a.end())
		{
			a.erase(iter);
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringLongMapMembers[] =
	{
		METHOD("__newOp", oStringLongMapNew),
		METHOD("delete", oStringLongMapDeleteMethod),
		METHOD("showInner", oStringLongMapShowInnerMethod),
		
		METHOD_RET("headIter", oStringLongMapHeadIterMethod, RETURNS_OBJECT(kBCIStringLongMapIter)),
		METHOD_RET("tailIter", oStringLongMapTailIterMethod, RETURNS_OBJECT(kBCIStringLongMapIter)),
		METHOD_RET("find", oStringLongMapFindMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("count", oStringLongMapCountMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("clear", oStringLongMapClearMethod),

		METHOD_RET("grab", oStringLongMapGrabMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("set", oStringLongMapSetMethod),
        METHOD("load", oStringLongMapLoadMethod),
        METHOD_RET("findValue", oStringLongMapFindValueMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD("remove", oStringLongMapRemoveMethod),

		MEMBER_VAR("__elements", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringLongMapIter
	//

	FORTHOP(oStringLongMapIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a StringLongMapIter object");
	}

	FORTHOP(oStringLongMapIterDeleteMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		SAFE_RELEASE(pCore, pIter->parent);
		delete pIter->cursor;
		delete pIter;
		TRACK_ITER_DELETE;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekNextMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		(*pIter->cursor)++;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekPrevMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		(*pIter->cursor)--;
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekHeadMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->begin();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterSeekTailMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		*(pIter->cursor) = pMap->elements->end();
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterNextMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			(*pIter->cursor)++;
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterPrevMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->begin())
		{
			SPUSH(0);
		}
		else
		{
			pIter->cursor--;
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

	FORTHOP(oStringLongMapIterCurrentMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) == pMap->elements->end())
		{
			SPUSH(0);
		}
		else
		{
			stackInt64 val;
			val.s64 = (*(pIter->cursor))->second;
			LPUSH(val);
			SPUSH(~0);
		}
		METHOD_RETURN;
	}

    FORTHOP(oStringLongMapIterCurrentPairMethod)
    {
        GET_THIS(oStringLongMapIterStruct, pIter);
        oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
        if (*(pIter->cursor) == pMap->elements->end())
        {
            SPUSH(0);
        }
        else
        {
            stackInt64 val;
            val.s64 = (*(pIter->cursor))->second;
            LPUSH(val);
            SPUSH((long)(*(pIter->cursor))->first.c_str());
            SPUSH(~0);
        }
        METHOD_RETURN;
    }

    FORTHOP(oStringLongMapIterRemoveMethod)
	{
		GET_THIS(oStringLongMapIterStruct, pIter);
		oStringLongMapStruct* pMap = reinterpret_cast<oStringLongMapStruct *>(pIter->parent);
		if (*(pIter->cursor) != pMap->elements->end())
		{
			pMap->elements->erase((*pIter->cursor));
			(*pIter->cursor)++;
		}
		METHOD_RETURN;
	}


	baseMethodEntry oStringLongMapIterMembers[] =
	{
		METHOD("__newOp", oStringLongMapIterNew),
		METHOD("delete", oStringLongMapIterDeleteMethod),

		METHOD("seekNext", oStringLongMapIterSeekNextMethod),
		METHOD("seekPrev", oStringLongMapIterSeekPrevMethod),
		METHOD("seekHead", oStringLongMapIterSeekHeadMethod),
		METHOD("seekTail", oStringLongMapIterSeekTailMethod),
		METHOD_RET("next", oStringLongMapIterNextMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("prev", oStringLongMapIterPrevMethod, RETURNS_NATIVE(kBaseTypeInt)),
		METHOD_RET("current", oStringLongMapIterCurrentMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD_RET("currentPair", oStringLongMapIterCurrentPairMethod, RETURNS_NATIVE(kBaseTypeInt)),
        METHOD("remove", oStringLongMapIterRemoveMethod),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIStringLongMap)),
		MEMBER_VAR("__cursor", NATIVE_TYPE_TO_CODE(0, kBaseTypeInt)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 StringDoubleMap
	//

    bool customStringDoubleMapReader(const std::string& elementName, ForthObjectReader* reader)
    {
        if (elementName == "map")
        {
            ForthCoreState* pCore = reader->GetCoreState();
            oStringLongMapStruct *dstMap = (oStringLongMapStruct *)(reader->getCustomReaderContext().pData);
            oStringLongMap& a = *(dstMap->elements);
            reader->getRequiredChar('{');
            std::string number;
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
                std::string key;
                reader->getString(key);
                reader->getRequiredChar(':');
                reader->getNumber(number);
                std::string number;
                double value;
                sscanf(number.c_str(), "%lf", &value);
                a[key] = *((long long *)&value);
            }
            return true;
        }
        return false;
    }

    FORTHOP(oStringDoubleMapShowInnerMethod)
	{
        char buffer[32];
        GET_THIS(oStringLongMapStruct, pMap);
        oStringLongMap::iterator iter;
        oStringLongMap& a = *(pMap->elements);
        ForthEngine *pEngine = ForthEngine::GetInstance();
        ForthShowContext* pShowContext = static_cast<ForthThread*>(pCore->pThread)->GetShowContext();
        pShowContext->BeginElement("map");
        pShowContext->ShowTextReturn("{");
        pShowContext->BeginNestedShow();
        if (a.size() > 0)
        {
            pShowContext->BeginIndent();
            for (iter = a.begin(); iter != a.end(); ++iter)
            {
                pShowContext->BeginElement(iter->first.c_str());
                stackInt64 val;
                val.s64 = iter->second;
                stackInt64 valb;
                valb.s32[0] = val.s32[1];
                valb.s32[1] = val.s32[0];
                double dvalb = *((double *)&valb);
                sprintf(buffer, "%f", dvalb);
                pShowContext->EndElement(buffer);
            }
            pShowContext->EndIndent();
            pShowContext->ShowIndent();
        }
        pShowContext->ShowTextReturn();
        pShowContext->ShowIndent();
        pShowContext->EndElement("}");
        METHOD_RETURN;
    }

	baseMethodEntry oStringDoubleMapMembers[] =
	{
		METHOD("__newOp", oStringLongMapNew),
		METHOD("showInner", oStringDoubleMapShowInnerMethod),

		// following must be last in table
		END_MEMBERS
	};


    // TODO: string-double map iter, it can just be string-long map iter, but parent member should be type kBCIStringDoubleMap

	void AddClasses(ForthEngine* pEngine)
	{
		ForthClassVocabulary* pVocab = pEngine->AddBuiltinClass("Map", kBCIMap, kBCIIterable, oMapMembers);
        pVocab->SetCustomObjectReader(customMapReader);
        pEngine->AddBuiltinClass("MapIter", kBCIMapIter, kBCIIter, oMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("IntMap", kBCIIntMap, kBCIIterable, oIntMapMembers);
        pVocab->SetCustomObjectReader(customIntMapReader);
        pEngine->AddBuiltinClass("IntMapIter", kBCIIntMapIter, kBCIIter, oIntMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("FloatMap", kBCIFloatMap, kBCIIntMap, oFloatMapMembers);
        pVocab->SetCustomObjectReader(customFloatMapReader);
        pEngine->AddBuiltinClass("FloatMapIter", kBCIFloatMapIter, kBCIIter, oIntMapIterMembers);

        gpLongMapClassVocab = pEngine->AddBuiltinClass("LongMap", kBCILongMap, kBCIIterable, oLongMapMembers);
        gpLongMapClassVocab->SetCustomObjectReader(customLongMapReader);
        pEngine->AddBuiltinClass("LongMapIter", kBCILongMapIter, kBCIIter, oLongMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("DoubleMap", kBCIDoubleMap, kBCILongMap, oDoubleMapMembers);
        pVocab->SetCustomObjectReader(customDoubleMapReader);
        pEngine->AddBuiltinClass("DoubleMapIter", kBCIDoubleMapIter, kBCIIter, oLongMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("StringIntMap", kBCIStringIntMap, kBCIIterable, oStringIntMapMembers);
        pVocab->SetCustomObjectReader(customStringIntMapReader);
        pEngine->AddBuiltinClass("StringIntMapIter", kBCIStringIntMapIter, kBCIIter, oStringIntMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("StringFloatMap", kBCIStringFloatMap, kBCIStringIntMap, oStringFloatMapMembers);
        pVocab->SetCustomObjectReader(customStringFloatMapReader);
        pEngine->AddBuiltinClass("StringFloatMapIter", kBCIStringFloatMapIter, kBCIIter, oStringIntMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("StringLongMap", kBCIStringLongMap, kBCIIterable, oStringLongMapMembers);
        pVocab->SetCustomObjectReader(customStringLongMapReader);
        pEngine->AddBuiltinClass("StringLongMapIter", kBCIStringLongMapIter, kBCIIter, oLongMapIterMembers);

        pVocab = pEngine->AddBuiltinClass("StringDoubleMap", kBCIStringDoubleMap, kBCIStringLongMap, oStringDoubleMapMembers);
        pVocab->SetCustomObjectReader(customStringDoubleMapReader);
        pEngine->AddBuiltinClass("StringDoubleMapIter", kBCIStringDoubleMapIter, kBCIIter, oLongMapIterMembers);
	}

} // namespace OMap

