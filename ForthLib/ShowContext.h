#pragma once
//////////////////////////////////////////////////////////////////////
//
// ShowContext.h: interface for the ShowContext class.
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

#include "Forth.h"
#include <set>
#include <vector>

class Engine;

class ShowContext
{
public:
	ShowContext();
	~ShowContext();

	void Reset();
	ucell GetDepth();

	void BeginIndent();
	void EndIndent();
	void ShowIndent(const char* pText = NULL);
    void BeginElement(const char* pName);
    // BeginRawElement doesn't print quotes around pName
    void BeginRawElement(const char* pName);
    void BeginLinkElement(const ForthObject& obj);
    void BeginArrayElement(int elementsPerLine = 0);
    void BeginFirstElement(const char* pText);
    void BeginNextElement(const char* pText);
	void EndElement(const char* pEndText = NULL);
	void ShowHeader(CoreState* pCore, const char* pTypeName, const void* pData);
	void ShowID(const char* pTypeName, const void* pData);
	void ShowIDElement (const char* pTypeName, const void* pData);
    void ShowObjectLink(const ForthObject& obj);
    void ShowText(const char* pText);
    void ShowQuotedText(const char* pText);
    void ShowTextReturn(const char* pText = nullptr);
    void ShowComma();
    void ShowCommaReturn();
    void SetArrayElementsPerLine(int numPerLine) { mArrayElementsPerLine = numPerLine; }
    void BeginArray();
    void EndArray();
    void BeginObject(const char* pName, const void* pData, bool showId);
    void EndObject();

    void AddObject(ForthObject& obj);
	// returns true IFF object has already been shown
	bool ObjectAlreadyShown(ForthObject& obj);

	std::vector<ForthObject>& GetObjects();

	void SetShowIDElement(bool inShow) { mShowIDElement = inShow; }
	bool GetShowIDElement(void) { return mShowIDElement;  }
	void SetShowRefCount(bool inShow) { mShowRefCount = inShow; }
	bool GetShowRefCount(void) { return mShowRefCount; }

    int GetNumShown() { return mNumShown; }
    int IncrementNumShown() { ++mNumShown; return mNumShown; }

    // show code calls BeginNestedShow before showing a nested object,
    // and calls EndNestedShow after showing the nested object
    void BeginNestedShow();
    void EndNestedShow();

private:
	ucell mDepth;
    int mNumShown; // num elements shown at current depth
    int mArrayElementsPerLine;
	std::set<void *> mShownObjects;
	std::vector<ForthObject> mObjects;
    std::vector<int> mNumShownStack;
	Engine* mpEngine;
	bool mShowIDElement;
	bool mShowRefCount;
    bool mShowSpaces;
};

