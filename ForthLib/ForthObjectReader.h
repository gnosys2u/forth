#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthObjectReader.h: interfaces for the JSON Object reader.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include <string>
#include "ForthEngine.h"
#include "ForthBuiltinClasses.h"

class ClassVocabulary;

typedef struct
{
    StructVocabulary* pVocab;
    int objIndex;
    char* pData;
} CustomReaderContext;

class ForthObjectReader
{
public:
    ForthObjectReader();
    ~ForthObjectReader();

    // returns true if there were no errors
    bool ReadObjects(ForthObject& inStream, ForthObject& outObjects, ForthCoreState* pCore);
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
    ForthCoreState* GetCoreState() { return mpCore; }

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

    ForthEngine *mpEngine;
    ForthCoreState* mpCore;

    std::string mError;

    char mSavedChar;
    bool mHaveSavedChar;

    int mLineNum;
    int mCharOffset;
};
