//////////////////////////////////////////////////////////////////////
//
// ForthShowContext.cpp: implementation of the ForthShowContext class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ForthShowContext.h"

#include "ForthEngine.h"

//////////////////////////////////////////////////////////////////////
////
///
//                     ForthShowContext
// 

ForthShowContext::ForthShowContext()
	: mDepth(0)
	, mShowIDElement(true)
	, mShowRefCount(true)
    , mShowSpaces(true)
    , mArrayElementsPerLine(10)
    , mNumShown(0)
{
	mpEngine = ForthEngine::GetInstance();
}

ForthShowContext::~ForthShowContext()
{
}

void ForthShowContext::Reset()
{
	mShownObjects.clear();
	mObjects.clear();
	mDepth = 0;
}

ulong ForthShowContext::GetDepth()
{
	return mDepth;
}


void ForthShowContext::BeginIndent()
{
	mDepth++;
}

void ForthShowContext::EndIndent()
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
        mpEngine->SetError(kForthErrorIllegalOperation, "ShowContext with negative indent");
    }
    */
}

void ForthShowContext::ShowIndent(const char* pText)
{
    if (mShowSpaces)
    {
        for (int32 i = 0; i < mDepth; i++)
        {
            ShowText("  ");
        }

    }

	if (pText != NULL)
	{
		ShowText(pText);
	}
}

void ForthShowContext::BeginElement(const char* pName)
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

void ForthShowContext::BeginRawElement(const char* pName)
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

void ForthShowContext::BeginLinkElement(const ForthObject& obj)
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

void ForthShowContext::BeginArrayElement(int elementsPerLine)
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

void ForthShowContext::BeginFirstElement(const char* pText)
{
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ForthShowContext::BeginNextElement(const char* pText)
{
	ShowComma();
	ShowIndent("\"");
	ShowText(pText);
    ShowText(mShowSpaces ? "\" : " : "\":");
}

void ForthShowContext::EndElement(const char* pEndText)
{
    ShowText(pEndText);
}

void ForthShowContext::AddObject(ForthObject& obj)
{
	if (mShownObjects.insert(obj).second)
	{
		mObjects.push_back(obj);
	}
}

bool ForthShowContext::ObjectAlreadyShown(ForthObject& obj)
{
	return obj == nullptr
        || mShownObjects.find(obj) != mShownObjects.end();
}

std::vector<ForthObject>& ForthShowContext::GetObjects()
{
	return mObjects;
}

void ForthShowContext::ShowHeader(ForthCoreState* pCore, const char* pTypeName, const void* pData)
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

void ForthShowContext::ShowID(const char* pTypeName, const void* pData)
{
	char buffer[16];

	ShowText(pTypeName);
	sprintf(buffer, "_%08x", (uint32) pData);
	ShowText(buffer);
}

void ForthShowContext::ShowIDElement(const char* pTypeName, const void* pData)
{
	if (mShowIDElement)
	{
        ShowIndent(mShowSpaces ? "\"__id\" : \"" : "\"__id\":\"");
        ShowID(pTypeName, pData);
        ShowText("\"");
        mNumShown++;
    }
}

void ForthShowContext::ShowText(const char* pText)
{
    if (pText != NULL)
    {
        mpEngine->ConsoleOut(pText);
    }
}

void ForthShowContext::ShowQuotedText(const char* pText)
{
    if (pText != NULL)
    {
        mpEngine->ConsoleOut("\"");
        mpEngine->ConsoleOut(pText);
        mpEngine->ConsoleOut("\"");
    }
}

void ForthShowContext::ShowTextReturn(const char* pText)
{
    ShowText(pText);
    if (mShowSpaces)
    {
        ShowText("\n");
    }
}

void ForthShowContext::ShowComma()
{
    ShowText(mShowSpaces ? ", " : ",");
}

void ForthShowContext::ShowCommaReturn()
{
    ShowTextReturn(",");
}

void ForthShowContext::BeginNestedShow()
{
    mNumShownStack.push_back(mNumShown);
    mNumShown = 0;
}

void ForthShowContext::EndNestedShow()
{
    mNumShown = mNumShownStack.back();
    mNumShownStack.pop_back();
}

void ForthShowContext::BeginArray()
{
    ShowText("[");
    BeginNestedShow();
    BeginIndent();
}

void ForthShowContext::EndArray()
{
    EndIndent();
    EndNestedShow();
    ShowTextReturn();
    ShowIndent("]");
}


void ForthShowContext::BeginObject(const char* pName, const void* pData, bool showId)
{
    BeginNestedShow();
    ShowTextReturn("{");
    BeginIndent();
    if (showId)
    {
        ShowIDElement(pName, pData);
    }
}


void ForthShowContext::EndObject()
{
    EndIndent();
    ShowTextReturn();
    ShowIndent("}");
    EndNestedShow();
}

void ForthShowContext::ShowObjectLink(const ForthObject& obj)
{
    ShowText("\"@");

    const char* pTypeName = "Null";
    if (obj != nullptr)
    {
        const ForthClassObject* pClassObject = (const ForthClassObject *)(*((obj->pMethods) - 1));
        pTypeName = pClassObject->pVocab->GetName();
    }
    ShowID(pTypeName, obj);

    ShowText("\"");
}

