//////////////////////////////////////////////////////////////////////
//
// ForthStructs.cpp: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "Vocabulary.h"
#include "Shell.h"
#include "Forgettable.h"
#include "ParseInfo.h"
#include "ShowContext.h"
#include "TypesManager.h"
#include "NativeType.h"
#include "LocalVocabulary.h"

// symbol entry layout for struct vocabulary (fields and method symbols
// offset   contents
//  0..3    field offset or method index
//  4..7    field data type or method return type
//  8..12   element size for arrays
//  13      1-byte symbol length (not including padding)
//  14..    symbol characters
//
// see forth.h for most current description of struct symbol entry fields

#define STRUCTS_EXPANSION_INCREMENT     16

//////////////////////////////////////////////////////////////////////
////
///     StructVocabulary
//
//

StructVocabulary::StructVocabulary( const char    *pName,
                                              int           typeIndex )
: Vocabulary( pName, NUM_STRUCT_VOCAB_VALUE_LONGS, DEFAULT_VOCAB_STORAGE )
, mTypeIndex( typeIndex )
, mAlignment( 1 )
, mNumBytes( 0 )
, mMaxNumBytes( 0 )
, mpSearchNext( NULL )
, mInitOpcode( 0 )
{
    mType = VocabularyType::kStruct;
}

StructVocabulary::~StructVocabulary()
{
}

// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
StructVocabulary::ForgetSymbol( const char *pSymName )
{
    return false;
}


// forget all ops with a greater op#
void
StructVocabulary::ForgetOp( forthop op )
{
}

void
StructVocabulary::DefineInstance( void )
{
    // do one of the following:
    // - define a global instance of this struct type
    // - define a local instance of this struct type
    // - define a field of this struct type
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    char *pInstanceName = pOuter->GetNextSimpleToken();
    int nBytes = mMaxNumBytes;
    char *pHere;
    int32_t val = 0;
    Vocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode;
    bool isPtr = false;
    TypesManager* pManager = TypesManager::GetInstance();
    CoreState *pCore = mpEngine->GetCoreState();        // so we can GET_VAR_OPERATION

    // if new instance name ends in '!', chop the '!' and initialize the new instance
    size_t instanceNameLen = strlen(pInstanceName);
    bool doInitializationVarop = false;
    if (instanceNameLen > 1 && pInstanceName[instanceNameLen - 1] == '!' && pOuter->CheckFeature(kFFAllowVaropSuffix))
    {
        instanceNameLen--;
        pInstanceName[instanceNameLen] = '\0';
        doInitializationVarop = true;
    }

    int32_t numElements = pOuter->GetArraySize();
    bool isArray = (numElements != 0);
    int32_t arrayFlag = (isArray) ? kDTIsArray : 0;
    pOuter->SetArraySize( 0 );
    typeCode = STRUCT_TYPE_TO_CODE( arrayFlag, mTypeIndex );

    if (pOuter->CheckFlag( kEngineFlagIsPointer ) )
    {
        pOuter->ClearFlag( kEngineFlagIsPointer );
        nBytes = sizeof(char *);
        typeCode |= kDTIsPtr;
        isPtr = true;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if (pOuter->IsCompiling() )
    {
        // define local struct
        pVocab = pOuter->GetLocalVocabulary();
        if ( isArray )
        {
            pOuter->SetArraySize( numElements );
            pOuter->AddLocalArray( pInstanceName, typeCode, nBytes );

			if (!isPtr && (mInitOpcode != 0))
			{
				// initialize the local struct array at runtime
				int offsetCells = pOuter->GetLocalVocabulary()->GetFrameCells();
                int offsetLongs = (offsetCells << CELL_SHIFT) >> 2;
				pOuter->CompileOpcode(kOpLocalRef, offsetLongs);
				pOuter->CompileOpcode(kOpConstant, numElements);
				pOuter->CompileOpcode(kOpConstant, mTypeIndex);
				pOuter->CompileBuiltinOpcode(OP_INIT_STRUCT_ARRAY);
			}
        }
        else
        {
            pHere = (char *) (mpEngine->GetDP());
            bool bCompileInstanceOp = pOuter->GetLastCompiledIntoPtr() == (((forthop *)pHere) - 1);
            pOuter->AddLocalVar( pInstanceName, typeCode, nBytes );
            if ( isPtr )
            {
                if (doInitializationVarop)
                {
                    // local var name ended with '!', so compile op for this local var with varop Set
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    forthop op = COMPILED_OP(kOpLocalCell, pEntry[0]);
                    pOuter->CompileOpcode(op | ((forthop)VarOperation::kVarSet) << 20);
                }
                else if (bCompileInstanceOp)
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    pOuter->CompileOpcode(pEntry[0]);
                }
            }
			else
			{
				if (mInitOpcode != 0)
				{
                    int offsetCells = pOuter->GetLocalVocabulary()->GetFrameCells();
                    int offsetLongs = (offsetCells << CELL_SHIFT) >> 2;
                    pOuter->CompileOpcode(kOpLocalRef, offsetLongs);
					pOuter->CompileOpcode(mInitOpcode);
				}
			}
        }
    }
    else
    {
		if ( pOuter->InStructDefinition() )
		{
			pManager->GetNewestStruct()->AddField( pInstanceName, typeCode, numElements );
			return;
		}

        // define global struct
        pOuter->AddUserOp( pInstanceName );
        pEntry = pOuter->GetDefinitionVocabulary()->GetNewestEntry();
        if ( isArray )
        {
            int32_t padding = 0;
            int32_t alignMask = mAlignment - 1;
            if ( nBytes & alignMask )
            {
                padding = mAlignment - (nBytes & alignMask);
            }
			int elementSize = nBytes + padding;
			if (isPtr)
            {
                pOuter->CompileBuiltinOpcode( OP_DO_CELL_ARRAY );
            }
            else
            {
                pOuter->CompileBuiltinOpcode( OP_DO_STRUCT_ARRAY );
				pOuter->CompileInt(elementSize);
            }
            pHere = (char *) (mpEngine->GetDP());
			mpEngine->AllotLongs(((elementSize * (numElements - 1)) + nBytes + 3) >> 2);
			memset(pHere, 0, (elementSize * numElements));
			if (!isPtr && (mInitOpcode != 0))
			{
				SPUSH((cell)pHere);
				SPUSH(numElements);
				SPUSH(mTypeIndex);
				mpEngine->FullyExecuteOp(pCore, gCompiledOps[OP_INIT_STRUCT_ARRAY]);
			}
        }
        else
        {
            pOuter->CompileBuiltinOpcode( isPtr ? OP_DO_CELL : OP_DO_STRUCT );
            pHere = (char *) (mpEngine->GetDP());
            mpEngine->AllotLongs( (nBytes  + 3) >> 2 );
            memset( pHere, 0, nBytes );
            if (isPtr)
            {
                if (doInitializationVarop)
                {
                    SET_VAR_OPERATION(VarOperation::kVarSet);
                }

                if (GET_VAR_OPERATION == VarOperation::kVarSet)
                {
                    // var definition was preceeded by "->", so initialize var
                    mpEngine->FullyExecuteOp(pCore, pEntry[0]);
                }
            }
            else
            {
                if (mInitOpcode != 0)
                {
                    SPUSH((cell)pHere);
                    mpEngine->FullyExecuteOp(pCore, mInitOpcode);
                }
            }
		}
        pEntry[1] = typeCode;
    }
}

void
StructVocabulary::AddField( const char* pName, int32_t fieldType, int numElements )
{
    // a struct vocab entry has the following value fields:
    // - field offset in bytes
    // - field type
    // - padded element size (valid only for array fields)

    int32_t fieldBytes, alignment, alignMask, padding;
    TypesManager* pManager = TypesManager::GetInstance();
    bool isPtr = CODE_IS_PTR( fieldType );
    bool isNative = CODE_IS_NATIVE( fieldType );

    pManager->GetFieldInfo( fieldType, fieldBytes, alignment );

    if ( mNumBytes == 0 )
    {
        // this is first field in struct, so this field defines structs' alignment
        // to allow union types, the first field in each union subtype can set
        //   the alignment, but it can't reduce the alignment size
        if ( alignment > mAlignment )
        {
            mAlignment = alignment;
        }
    }
    alignMask = alignment - 1;

    // handle alignment of start of this struct (element 0 in array case)
    if ( mNumBytes & alignMask )
    {
        padding = alignment - (mNumBytes & alignMask);
        SPEW_STRUCTS( "AddField padding %d bytes before field\n", padding );
        mNumBytes += padding;
    }

    forthop* pEntry = AddSymbol(pName, mNumBytes);
    pEntry[1] = fieldType;
    pEntry[2] = fieldBytes;
    if ( numElements != 0 )
    {
        // TBD: handle alignment for elements after 0
        if ( fieldBytes & alignMask )
        {
           padding = alignment - (fieldBytes & alignMask);
           SPEW_STRUCTS( "AddField padding %d bytes between elements\n", padding );
           fieldBytes = (fieldBytes * numElements) + (padding * (numElements - 1));
           pEntry[2] += padding;
        }
        else
        {
           // field size matches alignment
           fieldBytes *= numElements;
        }
    }
	
	if (!isPtr)
	{
		ForthFieldInitInfo initInfo;

		bool isArray = CODE_IS_ARRAY(fieldType);
		BaseType baseType = (BaseType) CODE_TO_BASE_TYPE(fieldType);
		initInfo.numElements = numElements;
		initInfo.offset = mNumBytes;
		initInfo.typeIndex = 0;
		initInfo.len = 0;

		if (baseType == BaseType::kString)
		{
			initInfo.len = CODE_TO_STRING_BYTES(fieldType);
			initInfo.fieldType = (isArray) ? kFSITStringArray : kFSITString;
			pManager->AddFieldInitInfo(initInfo);
		}
		else if (baseType == BaseType::kStruct)
		{
			int typeIndex = CODE_TO_STRUCT_INDEX(fieldType);
			ForthTypeInfo* structInfo = pManager->GetTypeInfo(typeIndex);
			if (structInfo->pVocab->GetInitOpcode() != 0)
			{
				initInfo.typeIndex = typeIndex;
				initInfo.len = fieldBytes;
				initInfo.fieldType = (isArray) ? kFSITStructArray : kFSITStruct;
				pManager->AddFieldInitInfo(initInfo);
			}
		}
	}

    mNumBytes += fieldBytes;
    if ( mNumBytes > mMaxNumBytes )
    {
        mMaxNumBytes = mNumBytes;
    }
    SPEW_STRUCTS( "AddField %s code 0x%x isPtr %d isNative %d elements %d\n",
                  pName, fieldType, isPtr, isNative, numElements );
}

int
StructVocabulary::GetAlignment( void )
{
    // return alignment of first field
    return mAlignment;
}

int
StructVocabulary::GetSize( void )
{
    // return size of struct
    return mMaxNumBytes;
}

void
StructVocabulary::StartUnion()
{
    // when a union subtype is started, the current size is zeroed, but
    //   the maximum size is left untouched
    // if this struct is an extension of another struct, the size is set to
    //   the size of the parent struct
    mNumBytes = (mpSearchNext) ? ((StructVocabulary *) mpSearchNext)->GetSize() : 0;
}

void
StructVocabulary::Extends( StructVocabulary *pParentStruct )
{
    // this new struct is an extension of an existing struct - it has all
    //   the fields of the parent struct
    mpSearchNext = pParentStruct;
    mNumBytes = pParentStruct->GetSize();
    mMaxNumBytes = mNumBytes;
    mAlignment = pParentStruct->GetAlignment();
}

const char*
StructVocabulary::GetDescription( void )
{
    return "struct";
}

// return ptr to vocabulary entry for symbol
forthop *
StructVocabulary::FindSymbol( const char *pSymName, ucell serial )
{
    int32_t tmpSym[SYM_MAX_LONGS];
    forthop* pEntry;
    ParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );
    StructVocabulary* pVocab = this;
    while ( pVocab )
    {
        pEntry = pVocab->Vocabulary::FindSymbol( &parseInfo, serial );
        if ( pEntry )
        {
            return pEntry;
        }
        pVocab = pVocab->mpSearchNext;
    }

    return NULL;
}

void
StructVocabulary::PrintEntry( forthop*   pEntry )
{
#define BUFF_SIZE 256
    char buff[BUFF_SIZE];
    char nameBuff[128];
    CoreState* pCore = mpEngine->GetCoreState();
    int32_t typeCode = pEntry[1];

    // print out the base class stuff - name and value fields
    sprintf( buff, "  %08x    ", *pEntry );
    CONSOLE_STRING_OUT( buff );

    for ( int j = 1; j < mValueLongs; j++ )
    {
        sprintf( buff, "%08x    ", pEntry[j] );
        CONSOLE_STRING_OUT( buff );
    }

    GetEntryName( pEntry, nameBuff, sizeof(nameBuff) );
    if ( strlen( nameBuff ) > 32 )
    {
        sprintf( buff, "%s    ", nameBuff );
    }
    else
    {
        sprintf( buff, "%32s    ", nameBuff );
    }
    CONSOLE_STRING_OUT( buff );

    TypecodeToString( typeCode, buff, sizeof(buff) );
    CONSOLE_STRING_OUT( buff );

    sprintf( buff, " @ offset %d", pEntry[0] );
    CONSOLE_STRING_OUT( buff );
}

void
StructVocabulary::TypecodeToString( int32_t typeCode, char* outBuff, size_t outBuffSize )
{
    char buff[BUFF_SIZE];
    char buff2[64];
    buff[0] = '\0';
    if ( CODE_IS_ARRAY( typeCode ) )
    {
        strcpy( buff, "array of " );
    }
    if ( CODE_IS_PTR( typeCode ) )
    {
        strcat( buff, "pointer to " );
    }
    if ( CODE_IS_NATIVE( typeCode ) )
    {
        BaseType baseType = CODE_TO_BASE_TYPE( typeCode );
        sprintf( buff2, "%s", gpNativeTypes[(ucell)baseType]->GetName() );
        strcat( buff, buff2 );
        if ( baseType == BaseType::kString )
        {
            sprintf( buff2, " strLen=%d", CODE_TO_STRING_BYTES( typeCode ) );
            strcat( buff, buff2 );
        }
    }
    else
    {
        BaseType baseType = CODE_TO_BASE_TYPE( typeCode );
        if (baseType == BaseType::kObject)
        {
            int32_t containedTypeIndex = CODE_TO_CONTAINED_CLASS_INDEX(typeCode);
            ForthTypeInfo* pContainedInfo = TypesManager::GetInstance()->GetTypeInfo(containedTypeIndex);
            if (pContainedInfo)
            {
                int32_t containerTypeIndex = CODE_TO_CONTAINER_CLASS_INDEX(typeCode);
                if (containerTypeIndex == kBCIInvalid)
                {
                    sprintf(buff2, "%s", pContainedInfo->pVocab->GetName());
                }
                else
                {
                    ForthTypeInfo* pContainerInfo = TypesManager::GetInstance()->GetTypeInfo(containerTypeIndex);
                    if (pContainerInfo)
                    {
                        sprintf(buff2, "%s of %s", pContainerInfo->pVocab->GetName(), pContainedInfo->pVocab->GetName());
                    }
                    else
                    {
                        sprintf(buff2, "<UNKNOWN CONTAINER CLASS INDEX %d!>", containerTypeIndex);
                    }
                }
            }
            else
            {
                sprintf(buff2, "<UNKNOWN CLASS INDEX %d!>", containedTypeIndex);
            }
        }
        else if (baseType == BaseType::kStruct)
        {
            int32_t typeIndex = CODE_TO_STRUCT_INDEX( typeCode );
            ForthTypeInfo* pInfo = TypesManager::GetInstance()->GetTypeInfo( typeIndex );
            if ( pInfo )
            {
                sprintf( buff2, "%s", pInfo->pVocab->GetName() );
            }
            else
            {
                sprintf( buff2, "<UNKNOWN STRUCT INDEX %d!>", typeIndex );
            }
        }
        else if (baseType == BaseType::kUserDefinition)
        {
            strcpy(buff2, "user defined forthop");
        }
        else if (baseType == BaseType::kVoid)
        {
            strcpy(buff2, "void");
        }
        else
        {
            sprintf(buff2, "UNKNOWN BASE TYPE %d", (int)baseType);
        }
        strcat( buff, buff2 );
    }
    outBuff[ outBuffSize - 1 ] = '\0';
    strncpy( outBuff, buff, outBuffSize - 1 );
}

void StructVocabulary::EndDefinition()
{
    mNumBytes = mMaxNumBytes;
}

const char *
StructVocabulary::GetTypeName( void )
{
    return "structVocabulary";
}

void
StructVocabulary::ShowData(const void* pData, CoreState* pCore, bool showId)
{
    GET_SHOW_CONTEXT;

    pShowContext->BeginObject(GetName(), pData, showId);

    StructVocabulary* pVocab = this;

    while (pVocab != nullptr)
    {
        pVocab->ShowDataInner(pData, pCore);
        pVocab = pVocab->BaseVocabulary();
    }

    pShowContext->EndObject();
}

int
StructVocabulary::ShowDataInner(const void* pData, CoreState* pCore, StructVocabulary* pEndVocab)
{
    forthop* pEntry = GetNewestEntry();
    if (pEntry == nullptr)
    {
        return 0;
    }

    char buffer[256];
    const char* pStruct = (const char*)pData;
    GET_SHOW_CONTEXT;
    StructVocabulary* pVocab = this;

    const char* pVocabName = pVocab->GetName();
    forthop* pEntriesEnd = pVocab->GetEntriesEnd();
	int previousOffset = pVocab->GetSize();
    while (pEntry < pEntriesEnd)
    {
        int32_t elementSize = VOCABENTRY_TO_ELEMENT_SIZE(pEntry);

        if (elementSize != 0)
        {
            int32_t typeCode = VOCABENTRY_TO_TYPECODE(pEntry);
            int32_t byteOffset = VOCABENTRY_TO_FIELD_OFFSET(pEntry);
            // this relies on the fact that entries come up in reverse order of base offset
            int32_t numElements = (previousOffset - byteOffset) / elementSize;
            previousOffset = byteOffset;
            BaseType baseType = CODE_TO_BASE_TYPE(typeCode);
            bool isNative = CODE_IS_NATIVE(typeCode);
            bool isPtr = CODE_IS_PTR(typeCode);
            bool isArray = CODE_IS_ARRAY(typeCode);
            int sval;
            uint32_t uval;

            // skip displaying __refCount (at offset 0) if the showRefCount flag is false
            if ((baseType != BaseType::kUserDefinition) && (baseType != BaseType::kVoid)
                && ((byteOffset != 0) || pShowContext->GetShowRefCount() || strcmp(buffer, "__refCount")))
            {
                pVocab->GetEntryName(pEntry, buffer, sizeof(buffer));
                pShowContext->BeginElement(buffer);

                // mark buffer as empty by default
                buffer[0] = '\0';

                if (isArray)
                {
                    pShowContext->BeginArray();
                }
                else
                {
                    numElements = 1;
                }

                if (isPtr)
                {
                    // hack to print all pointers in hex
                    baseType = BaseType::kOp;
                }

                while (numElements > 0)
                {
                    if (isArray)
                    {
                        pShowContext->BeginArrayElement();
                    }

                    switch (baseType)
                    {
                    case BaseType::kByte:
                        sval = *((const char*)(pStruct + byteOffset));
                        sprintf(buffer, "%d", sval);
                        break;

                    case BaseType::kUByte:
                        uval = *((const unsigned char*)(pStruct + byteOffset));
                        sprintf(buffer, "%u", uval);
                        break;

                    case BaseType::kShort:
                        sval = *((const short*)(pStruct + byteOffset));
                        sprintf(buffer, "%d", sval);
                        break;

                    case BaseType::kUShort:
                        uval = *((const unsigned short*)(pStruct + byteOffset));
                        sprintf(buffer, "%u", uval);
                        break;

                    case BaseType::kInt:
                        sval = *((const int*)(pStruct + byteOffset));
                        sprintf(buffer, "%d", sval);
                        break;

                    case BaseType::kUInt:
                        uval = *((const uint32_t*)(pStruct + byteOffset));
                        sprintf(buffer, "%u", uval);
                        break;

                    case BaseType::kLong:
                        sprintf(buffer, "%lld", *((const int64_t*)(pStruct + byteOffset)));
                        break;

                    case BaseType::kULong:
                        sprintf(buffer, "%llu", *((const uint64_t*)(pStruct + byteOffset)));
                        break;

                    case BaseType::kFloat:
                        sprintf(buffer, "%f", *((const float*)(pStruct + byteOffset)));
                        break;

                    case BaseType::kDouble:
                        sprintf(buffer, "%f", *((const double*)(pStruct + byteOffset)));
                        break;

                    case BaseType::kString:
                        pShowContext->ShowQuotedText(pStruct + byteOffset + 8);
                        break;

                    case BaseType::kOp:
                        uval = *((const uint32_t*)(pStruct + byteOffset));
                        sprintf(buffer, "0x%x", uval);
                        break;

                    case BaseType::kStruct:
                    {
                        pShowContext->BeginNestedShow();

                        ForthTypeInfo* pStructInfo = TypesManager::GetInstance()->GetTypeInfo(CODE_TO_STRUCT_INDEX(typeCode));
                        pStructInfo->pVocab->ShowData(pStruct + byteOffset, pCore, false);
                        //elementSize = pStructInfo->pVocab->GetSize();

                        pShowContext->EndNestedShow();
                        break;
                    }

                    case BaseType::kObject:
                    {
                        //pShowContext->BeginNestedShow();

                        ForthObject obj = *((ForthObject*)(pStruct + byteOffset));
                        ForthShowObject(obj, pCore);
                        //mpEngine->FullyExecuteMethod(pCore, obj, kMethodInner);

                        //pShowContext->EndNestedShow();
                        break;
                    }

                    default:
                        /*
                        BaseType::kUserDefinition,                // 14 - user defined forthop
                        BaseType::kVoid,							// 15 - void
                        */
                        break;
                    }

                    // if something was put in the buffer, print it
                    if (buffer[0])
                    {
                        pShowContext->ShowText(buffer);
                    }
                    byteOffset += elementSize;
                    --numElements;

                }  // end while numElements > 0

                if (isArray)
                {
                    pShowContext->EndArray();
                }
            }
        }
        pEntry = NextEntry(pEntry);
    }

    return pShowContext->GetNumShown();
}

void StructVocabulary::SetInitOpcode(forthop op)
{
	mInitOpcode = op;
}

