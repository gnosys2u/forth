//////////////////////////////////////////////////////////////////////
//
// ObjectReader.cpp: implementation of the JSON Object reader.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdexcept>
#include "ObjectReader.h"
#include "TypesManager.h"
#include "ClassVocabulary.h"
//#include "Engine.h"

/*
    todos:
    - handle right hand sides
    - how to handle current object data pointer when changing levels
    - run _init op for classes that have one
    ? look for elements in same order as they would be printed, or just search for them?
*/

ObjectReader::ObjectReader()
: mInStream(nullptr)
, mSavedChar('\0')
, mHaveSavedChar(false)
, mOutArrayObject(nullptr)
, mInStreamObject(nullptr)
{
    mpEngine = Engine::GetInstance();
}

ObjectReader::~ObjectReader()
{
}

bool ObjectReader::ReadObjects(ForthObject& inStream, ForthObject& outArray, CoreState *pCore)
{
    mInStreamObject = inStream;
    mOutArrayObject = outArray;
    mpCore = pCore;
    mInStream = reinterpret_cast<oInStreamStruct *>(inStream);
    mContextStack.clear();
    mLineNum = 1;
    mCharOffset = 0;

    mContext.pVocab = nullptr;
    mContext.objIndex = -1;
    mContext.pData = nullptr;

    bool itWorked = true;
    mError.clear();

    try
    {
        if ((mInStream->pInFuncs == nullptr) || (mInStream->pInFuncs->inChar == nullptr))
        {
            throwError("unusable input stream");
        }
        ForthObject obj;
        getObject(&obj);
        if (obj != nullptr)
        {
            oArrayStruct* outArray = reinterpret_cast<oArrayStruct *>(mOutArrayObject);
            outArray->elements->push_back(obj);
        }
    }
    catch (const std::exception& ex)
    {
        
        mError.assign(ex.what());
        itWorked = false;
    }

    return itWorked;
}


char ObjectReader::getRawChar()
{
    int ch;
    mInStream->pInFuncs->inChar(mpCore, mInStreamObject, ch);
    if (ch == -1)
    {
        throwError("unexpected EOF");
    }

    if (ch == '\n')
    {
        mLineNum++;
        mCharOffset = 0;
    }
    else
    {
        mCharOffset++;
    }

    return (char)ch;
}


char ObjectReader::getChar()
{
    if (mHaveSavedChar)
    {
        mHaveSavedChar = false;
        return mSavedChar;
    }

    skipWhitespace();
    if (mHaveSavedChar)
    {
        mHaveSavedChar = false;
        return mSavedChar;
    }
    return getRawChar();
}

void ObjectReader::getRequiredChar(char requiredChar)
{
    char actualChar = getChar();
    if (actualChar != requiredChar)
    {
        throwError("unexpected character");
    }
}

void ObjectReader::ungetChar(char ch)
{
    if (mHaveSavedChar)
    {
        throwError("can't unget more than one char");
    }
    else
    {
        mHaveSavedChar = true;
        mSavedChar = ch;
    }
}

void ObjectReader::getName(std::string& name)
{
    getString(name);
}

void ObjectReader::getString(std::string& str)
{
    getRequiredChar('\"');
    str.clear();
    char ch;
    while ((ch = getRawChar()) != '\"')
    {
        str.push_back(ch);
    }
}

void ObjectReader::getNumber(std::string& str)
{
    str.clear();
    skipWhitespace();
    while (true)
    {
        char ch = getChar();
        if (ch < '0' || ch > '9')
        {
            if (ch != '-' && ch != '.' && ch != 'e')
            {
                ungetChar(ch);
                break;
            }
        }
        str.push_back(ch);
    }
}

void ObjectReader::skipWhitespace()
{
    const char* whitespace = " \t\r\n";
    if (mHaveSavedChar)
    {
        if (strchr(whitespace, mSavedChar) == nullptr)
        {
            // there is a saved non-whitespace character, nothing to skip
            return;
        }
        mHaveSavedChar = false;
    }

    while (true)
    {
        char ch = getRawChar();
        if (strchr(whitespace, ch) == nullptr)
        {
            ungetChar(ch);
            break;
        }
    }
}

void ObjectReader::getObject(ForthObject* pDst)
{
    getRequiredChar('{');
    mContextStack.push_back(mContext);
    mContext.pVocab = nullptr;
    mContext.pData = nullptr;
    mContext.objIndex = -1;
    pDst = nullptr;

    bool done = false;
    while (!done)
    {
        char ch = getChar();

        if (ch == '}')
        {
            done = true;
        }
        else if (ch == '\"')
        {
            std::string elementName;
            ungetChar(ch);
            getName(elementName);
            getRequiredChar(':');
            processElement(elementName);
            char nextChar = getChar();
            if (nextChar != ',')
            {
                done = true;
                ungetChar(nextChar);
            }
        }
        else
        {
            throwError("unexpected char in getObject");
        }
    }
    getRequiredChar('}');
    if (mContext.objIndex >= 0)
    {
        *pDst = mObjects[mContext.objIndex];
    }
    mContext = mContextStack.back();
    mContextStack.pop_back();
}

void ObjectReader::getObjectOrLink(ForthObject* pDst)
{
    char ch = getChar();
    std::string str;

    if (ch == '\"')
    {
        // this must be an object link
        ungetChar(ch);
        getString(str);
        if (str.length() < 2)
        {
            throwError("object link too short");
        }
        else
        {
            if (str[0] == '@')
            {
                str = str.substr(1);
            }
            else
            {
                throwError("object link must begin with @");
            }
        }
        auto foundObj = mKnownObjects.find(str);
        if (foundObj != mKnownObjects.end())
        {
            ForthObject linkedObject = mObjects.at(foundObj->second);
            *pDst = linkedObject;

            // bump linked object refcount
            linkedObject->refCount += 1;
        }
        else
        {
            throwError("unknown linked object");
        }
    }
    else if (ch == '{')
    {
        // this must be an object
        ungetChar(ch);
        getObject(pDst);
    }
    else
    {
        throwError("unexpected char at start of object");
    }
}

void ObjectReader::getStruct(StructVocabulary* pVocab, int offset, char *pDstData)
{
    getRequiredChar('{');
    mContextStack.push_back(mContext);
    mContext.pVocab = pVocab;
    mContext.pData = pDstData;
    bool done = false;
    while (!done)
    {
        char ch = getChar();

        if (ch == '}')
        {
            done = true;
        }
        else if (ch == '\"')
        {
            std::string elementName;
            ungetChar(ch);
            getName(elementName);
            getRequiredChar(':');
            processElement(elementName);
            char nextChar = getChar();
            if (nextChar != ',')
            {
                done = true;
                ungetChar(nextChar);
            }
        }
        else
        {
            throwError("unexpected char in getStruct");
        }
    }
    getRequiredChar('}');
    mContext = mContextStack.back();
    mContextStack.pop_back();
}

void ObjectReader::processElement(const std::string& name)
{
    if (mContext.pVocab == nullptr)
    {
        // name has to be __id, value has to be string with form CLASSNAME_ID
        if (name == "__id")
        {
            // class name is part of __id up to last underscore
            std::string classId;
            getString(classId);
            size_t lastUnderscore = classId.find_last_of('_');
            if (lastUnderscore == classId.npos)
            {
                throwError("__id missing underscore");
            }
            if (lastUnderscore < 1)
            {
                throwError("__id has empty class name");
            }
            std::string className = classId.substr(0, lastUnderscore);

            CoreState *pCore = mpCore;
            StructVocabulary* newClassVocab = TypesManager::GetInstance()->GetStructVocabulary(className.c_str());
            if (newClassVocab->IsClass())
            {
                mContext.pVocab = newClassVocab;
                mContext.objIndex = (int) mObjects.size();

                int32_t initOpcode = mContext.pVocab->GetInitOpcode();
                SPUSH((cell)mContext.pVocab);
                mpEngine->FullyExecuteOp(pCore, (static_cast<ClassVocabulary *>(mContext.pVocab))->GetClassObject()->newOp);
                if (initOpcode != 0)
                {
                    // copy object data pointer to TOS to be used by init 
                    cell a = (GET_SP)[1];
                    SPUSH(a);
                    mpEngine->FullyExecuteOp(pCore, initOpcode);
                }

                ForthObject newObject;
                POP_OBJECT(newObject);
                // new object has refcount of 1
                newObject->refCount = 1;
                mObjects.push_back(newObject);
                mContext.pData = (char *) newObject;
                mKnownObjects[classId] = mContext.objIndex;
            }
            else
            {
                throwError("not a class vocabulary");
            }
        }
        else
        {
            throwError("first element is not __id");
        }
    }
    else
    {
        // ignore __refcount
        if (name == "__refCount")
        {
            // discard following ref count
            std::string unusedCount;
            getNumber(unusedCount);
        }
        else if (name == "__id")
        {
            // discard struct id
            std::string unusedId;
            getString(unusedId);
        }
        else
        {
            // lookup name in current vocabulary to see how to process
            forthop* pEntry = mContext.pVocab->FindSymbol(name.c_str());
            if (pEntry != nullptr)
            {
                // TODO - handle number, string, object, array
                int32_t elementSize = VOCABENTRY_TO_ELEMENT_SIZE(pEntry);
                if (elementSize == 0)
                {
                    throwError("attempt to read zero size element");
                }
                int32_t typeCode = VOCABENTRY_TO_TYPECODE(pEntry);
                int32_t byteOffset = VOCABENTRY_TO_FIELD_OFFSET(pEntry);
                BaseType baseType = CODE_TO_BASE_TYPE(typeCode);
                //bool isNative = CODE_IS_NATIVE(typeCode);
                bool isPtr = CODE_IS_PTR(typeCode);
                bool isArray = CODE_IS_ARRAY(typeCode);
                if (isPtr)
                {
                    baseType = BaseType::kCell;
                    isArray = false;
                }
                float fval;
                double dval;
                int ival;
                int64_t lval;
                std::string str;
                char *pDst = mContext.pData + byteOffset;
                int roomLeft = mContext.pVocab->GetSize() - byteOffset;

                if (isArray)
                {
                    getRequiredChar('[');
                }

                bool done = false;
                while (!done)
                {
                    int bytesConsumed = 0;
                    switch (baseType)
                    {
                    case BaseType::kByte:
                    case BaseType::kUByte:
                    {
                        getNumber(str);
                        if (roomLeft < 1)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse byte");
                        }
                        *pDst = (char)ival;
                        bytesConsumed = 1;
                        break;
                    }

                    case BaseType::kShort:
                    case BaseType::kUShort:
                    {
                        getNumber(str);
                        if (roomLeft < 2)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse short");
                        }
                        *(short *)pDst = (short)ival;
                        bytesConsumed = 2;
                        break;
                    }

                    case BaseType::kInt:
                    case BaseType::kUInt:
                    case BaseType::kOp:
                    {
                        getNumber(str);
                        if (roomLeft < 4)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%d", &ival) != 1)
                        {
                            throwError("failed to parse int");
                        }
                        *(int *)pDst = ival;
                        bytesConsumed = 4;
                        break;
                    }

                    case BaseType::kLong:          // 6 - long
                    case BaseType::kULong:         // 7 - ulong
                    {
                        getNumber(str);
                        if (roomLeft < 8)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%lld", &lval) != 1)
                        {
                            throwError("failed to parse int32_t");
                        }
                        *(int64_t *)pDst = lval;
                        bytesConsumed = 8;
                        break;
                    }

                    case BaseType::kFloat:
                    {
                        getNumber(str);
                        if (roomLeft < 4)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%g", &fval) != 1)
                        {
                            throwError("failed to parse float");
                        }
                        *(float *)pDst = (short)fval;
                        bytesConsumed = 4;
                        break;
                    }

                    case BaseType::kDouble:
                    {
                        getNumber(str);
                        if (roomLeft < 8)
                        {
                            throwError("data would overrun object end");
                        }
                        if (sscanf(str.c_str(), "%g", &dval) != 1)
                        {
                            throwError("failed to parse double");
                        }
                        *(double *)pDst = dval;
                        bytesConsumed = 8;
                        break;
                    }

                    case BaseType::kString:
                    {
                        getString(str);
                        int maxBytes = CODE_TO_STRING_BYTES(typeCode);
                        if (str.size() >= maxBytes)
                        {
                            throwError("string too big");
                        }
                        strcpy(pDst + 8, str.c_str());
                        bytesConsumed = 8 + (((maxBytes + 3) >> 2) << 2);
                        break;
                    }

                    case BaseType::kObject:
                    {
                        getObjectOrLink((ForthObject *) pDst);
                        bytesConsumed = 8;
                        break;
                    }

                    case BaseType::kStruct:
                    {
                        int typeIndex = CODE_TO_STRUCT_INDEX(typeCode);
                        ForthTypeInfo* structInfo = TypesManager::GetInstance()->GetTypeInfo(typeIndex);
                        getStruct(structInfo->pVocab, byteOffset, mContext.pData);
                        break;
                    }

                    default:
                        throwError("unexpected type found");
                        break;
                    }
                    roomLeft -= bytesConsumed;
                    pDst += bytesConsumed;

                    if (isArray)
                    {
                        char endCh = getChar();
                        if (endCh == ']')
                        {
                            ungetChar(endCh);
                            done = true;
                        }
                        else if (endCh != ',')
                        {
                            ungetChar(endCh);
                        }
                    }
                    else
                    {
                        done = true;
                    }
                }

                if (isArray)
                {
                    getRequiredChar(']');
                }
            }
            else
            {
                processCustomElement(name);
            }
        }
    }
}

void ObjectReader::processCustomElement(const std::string& name)
{
    if (mContext.pVocab->IsClass())
    {
        ClassVocabulary* pVocab = (ClassVocabulary *)mContext.pVocab;

        while (pVocab != nullptr)
        {
            CustomObjectReader customReader = pVocab->GetCustomObjectReader();
            if (customReader != nullptr)
            {
                if (customReader(name, this))
                {
                    // customReader processed this element, all done
                    return;
                }
            }
            pVocab = (ClassVocabulary *)pVocab->BaseVocabulary();
        }
    }
    throwError("name not found");
}

CustomReaderContext& ObjectReader::getCustomReaderContext()
{
    return mContext;
}

void ObjectReader::throwError(const char* message)
{
    // TODO: add line, offset to message
    char buffer[128];
    sprintf(buffer, "%s at line %d character %d",
        message, mLineNum, mCharOffset);

    throw std::runtime_error(buffer);
}

void ObjectReader::throwError(const std::string& message)
{
    // TODO: add line, offset to message
    throwError(message.c_str());
}
