//////////////////////////////////////////////////////////////////////
//
// ShowContext.cpp: implementation of the ShowContext class.
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"

#include "ShowContext.h"

#include "Engine.h"
#include "ClassVocabulary.h"


//////////////////////////////////////////////////////////////////////
////
///
//                     ShowContext
// 

ShowContext::ShowContext()
	: mDepth(0)
	, mShowIDElement(true)
	, mShowRefCount(true)
    , mShowSpaces(true)
    , mArrayElementsPerLine(10)
    , mNumShown(0)
{
	mpEngine = Engine::GetInstance();
}

ShowContext::~ShowContext()
{
}

void ShowContext::Reset()
{
	mShownObjects.clear();
	mObjects.clear();
	mDepth = 0;
}

ucell ShowContext::GetDepth()
{
	return mDepth;
}


void ShowContext::BeginIndent()
{
	mDepth++;
}

void ShowContext::EndIndent()
{
    if (mDepth > 0)
    {
        mDepth--;
        if (mDepth == 0)
        {
            Reset();
        }
    }
    /*
    // TODO: reporting an error here causes a hang if output is redirected
    else
    {
        mpEngine->SetError(ForthError::illegalOperation, "ShowContext with negative indent");
    }
    */
}

void ShowContext::ShowIndent(const char* pText)
{
    if (mShowSpaces)
    {
        for (ucell i = 0; i < mDepth; i++)
        {
            ShowText("  ");
        }

    }

	if (pText != NULL)
	{
		ShowText(pText);
	}
}

void ShowContext::BeginElement(const char* pName)
{
    if (mNumShown != 0)
    {
        ShowCommaReturn();
    }

    ShowIndent("\"");
    ShowText(pName);
    ShowText(mShowSpaces ? "\" : " : "\":");
    mNumShown++;
}

void ShowContext::BeginRawElement(const char* pName)
{
    if (mNumShown != 0)
    {
        ShowCommaReturn();
    }

    ShowIndent();
    ShowText(pName);
    ShowText(mShowSpaces ? " : " : ":");
    mNumShown++;
}

void ShowContext::BeginLinkElement(const ForthObject& obj)
{
    if (mNumShown != 0)
    {
        ShowCommaReturn();
    }

    ShowIndent();
    ShowObjectLink(obj);
    ShowText(mShowSpaces ? " : " : ":");
    mNumShown++;
}

void ShowContext::BeginArrayElement(int elementsPerLine)
{
    if (elementsPerLine == 0)
    {
        elementsPerLine = mArrayElementsPerLine;
    }

    if (mNumShown != 0)
    {
        ShowComma();
    }

    if (elementsPerLine == 1 || (mNumShown % elementsPerLine) == 0)
    {
        ShowTextReturn();
        ShowIndent();
    }
    mNumShown++;
}

void ShowContext::BeginFirstElement(const char* pText)
{
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ShowContext::BeginNextElement(const char* pText)
{
	ShowComma();
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ShowContext::EndElement(const char* pEndText)
{
    ShowText(pEndText);
}

void ShowContext::AddObject(ForthObject& obj)
{
	if (mShownObjects.insert(obj).second)
	{
		mObjects.push_back(obj);
	}
}

bool ShowContext::ObjectAlreadyShown(ForthObject& obj)
{
	return obj == nullptr
        || mShownObjects.find(obj) != mShownObjects.end();
}

std::vector<ForthObject>& ShowContext::GetObjects()
{
	return mObjects;
}

void ShowContext::ShowHeader(CoreState* pCore, const char* pTypeName, const void* pData)
{
	char buffer[16];

	ShowTextReturn("{");
    mNumShown = 0;

    ShowIDElement(pTypeName, pData);

	if (mShowRefCount)
	{
        if (mNumShown != 0)
        {
            ShowComma();
        }

        ShowIndent();
        ShowText(mShowSpaces ? "\"__refCount\" : " : "\"__refCount\":");
		sprintf(buffer, "%d,", *(int *)(pData));
		EndElement(buffer);
        mNumShown++;
	}
}

void ShowContext::ShowID(const char* pTypeName, const void* pData)
{
	char buffer[32];

	ShowText(pTypeName);
#ifdef FORTH64
	sprintf(buffer, "_%016llx", (uint64_t) pData);
#else
    sprintf(buffer, "_%08x", (uint32_t)pData);
#endif
	ShowText(buffer);
}

void ShowContext::ShowIDElement(const char* pTypeName, const void* pData)
{
	if (mShowIDElement)
	{
        ShowIndent(mShowSpaces ? "\"__id\" : \"" : "\"__id\":\"");
        ShowID(pTypeName, pData);
        ShowText("\"");
        mNumShown++;
    }
}

void ShowContext::ShowText(const char* pText)
{
    if (pText != NULL)
    {
        mpEngine->ConsoleOut(pText);
    }
}

void ShowContext::ShowQuotedText(const char* pText)
{
    if (pText != NULL)
    {
        mpEngine->ConsoleOut("\"");
        mpEngine->ConsoleOut(pText);
        mpEngine->ConsoleOut("\"");
    }
}

void ShowContext::ShowTextReturn(const char* pText)
{
    ShowText(pText);
    if (mShowSpaces)
    {
        ShowText("\n");
    }
}

void ShowContext::ShowComma()
{
    ShowText(mShowSpaces ? ", " : ",");
}

void ShowContext::ShowCommaReturn()
{
    ShowTextReturn(",");
}

void ShowContext::BeginNestedShow()
{
    mNumShownStack.push_back(mNumShown);
    mNumShown = 0;
}

void ShowContext::EndNestedShow()
{
    mNumShown = mNumShownStack.back();
    mNumShownStack.pop_back();
}

void ShowContext::BeginArray()
{
    ShowText("[");
    BeginNestedShow();
    BeginIndent();
}

void ShowContext::EndArray()
{
    EndIndent();
    EndNestedShow();
    ShowTextReturn();
    ShowIndent("]");
}


void ShowContext::BeginObject(const char* pName, const void* pData, bool showId)
{
    BeginNestedShow();
    ShowTextReturn("{");
    BeginIndent();
    if (showId)
    {
        ShowIDElement(pName, pData);
    }
}


void ShowContext::EndObject()
{
    EndIndent();
    ShowTextReturn();
    ShowIndent("}");
    EndNestedShow();
}

void ShowContext::ShowObjectLink(const ForthObject& obj)
{
    ShowText("\"@");

    const char* pTypeName = "Null";
    if (obj != nullptr)
    {
        const ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
        pTypeName = pClassObject->pVocab->GetName();
    }
    ShowID(pTypeName, obj);

    ShowText("\"");
}

