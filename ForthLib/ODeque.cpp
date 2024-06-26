//////////////////////////////////////////////////////////////////////
//
// ODeque.cpp: builtin deque class
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
#include <deque>

#include "Engine.h"
#include "OuterInterpreter.h"
#include "Vocabulary.h"
#include "Object.h"
#include "BuiltinClasses.h"
#include "ShowContext.h"
#include "ObjectReader.h"

#include "TypesManager.h"
#include "ClassVocabulary.h"

#include "ODeque.h"

namespace ODeque
{
    typedef std::deque<ForthObject> oDeque;
    struct oDequeStruct
    {
        forthop*        pMethods;
        REFCOUNTER      refCount;
        oDeque          *que;
    };

    /*
    struct oDequeIterStruct
    {
        int32_t*               pMethods;
        REFCOUNTER          refCount;
        ForthObject			parent;
        oDeque::iterator	*cursor;
    };
    */


    //////////////////////////////////////////////////////////////////////
    ///
    //                 Deque
    //

    /*
    oDequeIterStruct* createDequeIterator(CoreState* pCore, oDequeStruct* pDeq)
    {
        ClassVocabulary *pIterVocab = TypesManager::GetInstance()->GetClassVocabulary(kBCIDequeIter);
        ALLOCATE_ITER(oDequeIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
        pIter->parent = reinterpret_cast<ForthObject>(pDeq);
        return pIter;
    }
    */

    bool customDequeReader(const std::string& elementName, ObjectReader* reader)
    {
        if (elementName == "queue")
        {
            CoreState* pCore = reader->GetCoreState();
            oDequeStruct *dstDeque = (oDequeStruct *)(reader->getCustomReaderContext().pData);
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
                dstDeque->que->push_back(obj);
                // TODO: release obj here?
            }
            return true;
        }
        return false;
    }

    FORTHOP(oDequeNew)
    {
        ClassVocabulary *pClassVocab = (ClassVocabulary *)(SPOP);
        ALLOCATE_OBJECT(oDequeStruct, pDeque, pClassVocab);
        pDeque->pMethods = pClassVocab->GetMethods();
        pDeque->refCount = 0;
        pDeque->que = new oDeque;
        PUSH_OBJECT(pDeque);
    }

    FORTHOP(oDequeDeleteMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oDequeStruct, pDeque);
        oDeque* deq = pDeque->que;
        while (!deq->empty())
        {
            ForthObject o = deq->back();
            SAFE_RELEASE(pCore, o);
            deq->pop_back();
        }
        METHOD_RETURN;
    }

    FORTHOP(oDequeShowInnerMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque::iterator iter;
        oDeque& deq = *(pDeque->que);
        GET_SHOW_CONTEXT;
        pShowContext->BeginElement("queue");
        pShowContext->BeginArray();
        if (!deq.empty())
        {
            for (iter = deq.begin(); iter != deq.end(); ++iter)
            {
                ForthObject& o = *iter;
                pShowContext->BeginArrayElement(1);
                ForthShowObject(o, pCore);
            }
        }
        pShowContext->EndArray();
        METHOD_RETURN;
    }

    FORTHOP(oDequeCountMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        SPUSH((int32_t)(pDeque->que->size()));
        METHOD_RETURN;
    }

    FORTHOP(oDequeClearMethod)
    {
        // go through all elements and release any which are not null
        GET_THIS(oDequeStruct, pDeque);
        oDeque::iterator iter;
        oDeque& a = *(pDeque->que);
        for (iter = a.begin(); iter != a.end(); ++iter)
        {
            ForthObject& o = *iter;
            SAFE_RELEASE(pCore, o);
        }
        a.clear();
        METHOD_RETURN;
    }

    FORTHOP(oDequePushHeadMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        ForthObject fobj;
        POP_OBJECT(fobj);
        SAFE_KEEP(fobj);
        a.push_front(fobj);
        METHOD_RETURN;
    }

    FORTHOP(oDequePushTailMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        ForthObject fobj;
        POP_OBJECT(fobj);
        SAFE_KEEP(fobj);
        a.push_back(fobj);
        METHOD_RETURN;
    }

    FORTHOP(oDequePopHeadMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        if (!a.empty())
        {
            ForthObject fobj = a.front();
            a.pop_front();
            unrefObject(fobj);
            PUSH_OBJECT(fobj);
        }
        else
        {
            GET_ENGINE->SetError(ForthError::illegalOperation, " pop of empty ODeque");
        }
        METHOD_RETURN;
    }

    FORTHOP(oDequePopTailMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        if (!a.empty())
        {
            ForthObject fobj = a.back();
            a.pop_back();
            unrefObject(fobj);
            PUSH_OBJECT(fobj);
        }
        else
        {
            GET_ENGINE->SetError(ForthError::illegalOperation, " pop of empty ODeque");
        }
        METHOD_RETURN;
    }

    FORTHOP(oDequePeekHeadMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        ForthObject fobj = nullptr;
        if (!a.empty())
        {
            fobj = a.front();
        }
        PUSH_OBJECT(fobj);
        METHOD_RETURN;
    }

    FORTHOP(oDequePeekTailMethod)
    {
        GET_THIS(oDequeStruct, pDeque);
        oDeque& a = *(pDeque->que);
        ForthObject fobj = nullptr;
        if (!a.empty())
        {
            fobj = a.back();
        }
        PUSH_OBJECT(fobj);
        METHOD_RETURN;
    }

    baseMethodEntry oDequeMembers[] =
    {
        METHOD("__newOp", oDequeNew),
        METHOD("delete", oDequeDeleteMethod),
        METHOD("showInner", oDequeShowInnerMethod),

        METHOD_RET("count", oDequeCountMethod, RETURNS_NATIVE(BaseType::kInt)),
        METHOD("clear", oDequeClearMethod),

        METHOD("pushHead", oDequePushHeadMethod),
        METHOD("pushTail", oDequePushTailMethod),
        METHOD_RET("popHead", oDequePopHeadMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD_RET("popTail", oDequePopTailMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD_RET("peekHead", oDequePeekHeadMethod, RETURNS_OBJECT(kBCIContainedType)),
        METHOD_RET("peekTail", oDequePeekTailMethod, RETURNS_OBJECT(kBCIContainedType)),

        MEMBER_VAR("__queue", NATIVE_TYPE_TO_CODE(kDTIsPtr, BaseType::kUCell)),
        // following must be last in table
        END_MEMBERS
    };

    void AddClasses(OuterInterpreter* pOuter)
    {
        ClassVocabulary* pVocab = pOuter->AddBuiltinClass("Deque", kBCIDeque, kBCIObject, oDequeMembers);
        pVocab->SetCustomObjectReader(customDequeReader);
    }

} // namespace ODeque
