#pragma once
//////////////////////////////////////////////////////////////////////
//
// ObjectReader.h: interfaces for the JSON Object reader.
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

#include <map>
#include <string>
#include "Engine.h"
#include "BuiltinClasses.h"

class ClassVocabulary;

typedef struct
{
    StructVocabulary* pVocab;
    int objIndex;
    char* pData;
} CustomReaderContext;

class ObjectReader
{
public:
    ObjectReader();
    ~ObjectReader();

    // returns true if there were no errors
    bool ReadObjects(ForthObject& inStream, ForthObject& outObjects, CoreState* pCore);
    const std::string& GetError() const { return mError; }

    char getChar();
    char getRawChar();
    void getRequiredChar(char ch);
    void ungetChar(char ch);
    void getName(std::string& name);
    void getString(std::string& str);
    void getNumber(std::string& str);
    void skipWhitespace();
    void getObject(ForthObject* pDst);
    void getObjectOrLink(ForthObject* pDst);
    void getStruct(StructVocabulary* pVocab, int offset, char *pDstData);
    void processElement(const std::string& name);
    void processCustomElement(const std::string& name);
    void throwError(const char* message);
    void throwError(const std::string& message);
    CustomReaderContext& getCustomReaderContext();
    CoreState* GetCoreState() { return mpCore; }

private:

    typedef std::map<std::string, int> knownObjectMap;

    ForthObject mInStreamObject;
    ForthObject mOutArrayObject;
    oInStreamStruct* mInStream;
    //oArrayStruct* mOutArray;
    std::vector<ForthObject> mObjects;
    CustomReaderContext mContext;
    std::vector<CustomReaderContext> mContextStack;

    knownObjectMap mKnownObjects;

    Engine *mpEngine;
    CoreState* mpCore;

    std::string mError;

    char mSavedChar;
    bool mHaveSavedChar;

    int mLineNum;
    int mCharOffset;
};
