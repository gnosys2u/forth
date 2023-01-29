//////////////////////////////////////////////////////////////////////
//
// ForthStructs.cpp: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthEngine.h"
#include "OuterInterpreter.h"
#include "ForthVocabulary.h"
#include "ForthShell.h"
#include "ForthForgettable.h"
#include "ForthBuiltinClasses.h"
#include "ForthParseInfo.h"
#include "ForthShowContext.h"

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

NativeType gNativeTypeByte( "byte", 1, BaseType::kByte );

NativeType gNativeTypeUByte( "ubyte", 1, BaseType::kUByte );

NativeType gNativeTypeShort( "short", 2, BaseType::kShort );

NativeType gNativeTypeUShort( "ushort", 2, BaseType::kUShort );

NativeType gNativeTypeInt( "int", 4, BaseType::kInt );

NativeType gNativeTypeUInt( "uint", 4, BaseType::kUInt );

NativeType gNativeTypeLong( "long", 8, BaseType::kLong );

NativeType gNativeTypeULong( "ulong", 8, BaseType::kULong );

NativeType gNativeTypeFloat( "float", 4, BaseType::kFloat );

NativeType gNativeTypeDouble( "double", 8, BaseType::kDouble );

NativeType gNativeTypeString( "string", 12, BaseType::kString );

NativeType gNativeTypeOp( "op", 4, BaseType::kOp );

NativeType gNativeTypeObject( "object", 8, BaseType::kObject );

NativeType *gpNativeTypes[] =
{
    &gNativeTypeByte,
    &gNativeTypeUByte,
    &gNativeTypeShort,
    &gNativeTypeUShort,
    &gNativeTypeInt,
    &gNativeTypeUInt,
    &gNativeTypeLong,
    &gNativeTypeULong,
    &gNativeTypeFloat,
    &gNativeTypeDouble,
    &gNativeTypeString,
    &gNativeTypeOp,
    &gNativeTypeObject
};

//////////////////////////////////////////////////////////////////////
////
///     TypesManager
//
//

TypesManager *TypesManager::mpInstance = NULL;


TypesManager::TypesManager()
	: Forgettable(NULL, 0)
	, mpSavedDefinitionVocab(NULL)
	, mpClassMethods(NULL)
	, mNewestTypeIndex(0)
{
	ASSERT(mpInstance == NULL);
	mpInstance = this;
	mpCodeGenerator = new ForthStructCodeGenerator(this);

	ForthTypeInfo structInfo;
	structInfo.pVocab = NULL;
	structInfo.op = OP_ABORT;
	structInfo.typeIndex = static_cast<int32_t>(kBCIInvalid);
	for (int i = 0; i < kNumBuiltinClasses; ++i)
	{
		mStructInfo.emplace_back(ForthTypeInfo(NULL, OP_ABORT, static_cast<int32_t>(kBCIInvalid)));
	}
}

TypesManager::~TypesManager()
{
	delete mpCodeGenerator;
    mpInstance = NULL;
    mStructInfo.clear();
}

void
TypesManager::ForgetCleanup( void *pForgetLimit, forthop op )
{
    // remove struct info for forgotten struct types
	int numStructs;

	int oldNumStructs = static_cast<int>(mStructInfo.size());
	for (numStructs = oldNumStructs; numStructs > 0; numStructs--)
    {
        if ( FORTH_OP_VALUE( mStructInfo[numStructs - 1].op ) >= op )
        {
            // this struct is among forgotten ops
			SPEW_STRUCTS("Forgetting struct vocab %s\n", mStructInfo[numStructs - 1].pVocab->GetName());
        }
        else
        {
            break;
        }
    }

	if (numStructs != oldNumStructs)
	{
		mStructInfo.resize(numStructs);
	}
}


StructVocabulary*
TypesManager::StartStructDefinition( const char *pName )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthVocabulary* pDefinitionsVocab = pEngine->GetOuterInterpreter()->GetDefinitionVocabulary();

    forthop *pEntry = pEngine->GetOuterInterpreter()->StartOpDefinition( pName, true, kOpUserDefImmediate );
	mNewestTypeIndex = static_cast<int>(mStructInfo.size());
	StructVocabulary* pVocab = new StructVocabulary(pName, (int)mNewestTypeIndex);
	mStructInfo.emplace_back(ForthTypeInfo(pVocab, *pEntry, (int)mNewestTypeIndex));
	SPEW_STRUCTS("StartStructDefinition %s struct index %d\n", pName, mNewestTypeIndex);
    return pVocab;
}

void
TypesManager::EndStructDefinition()
{
    SPEW_STRUCTS( "EndStructDefinition\n" );
    ForthEngine *pEngine = ForthEngine::GetInstance();
    pEngine->GetOuterInterpreter()->EndOpDefinition( true );
    GetNewestStruct()->EndDefinition();
	DefineInitOpcode();
}

void
TypesManager::DefineInitOpcode()
{
	ForthTypeInfo *pInfo = &(mStructInfo[mNewestTypeIndex]);
	StructVocabulary *pVocab = pInfo->pVocab;

	// TODO: define new init opcode
	if (mFieldInitInfos.size() > 0)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
        OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

		ForthVocabulary* pOldDefinitionsVocab = pOuter->GetDefinitionVocabulary();
        pOuter->SetDefinitionVocabulary(pVocab);

		forthop* pEntry = pOuter->StartOpDefinition("_init", true, kOpUserDef);
		pEntry[1] = (forthop)BASE_TYPE_TO_CODE(BaseType::kUserDefinition);
		int32_t structInitOp = *pEntry;
		pVocab->SetInitOpcode(structInitOp);

		for (uint32_t i = 0; i < mFieldInitInfos.size(); i++)
		{
			const ForthFieldInitInfo& initInfo = mFieldInitInfos[i];
			if (i != (mFieldInitInfos.size() - 1))
			{
				// compile a dup opcode for all but the last initialized field
                pOuter->CompileBuiltinOpcode(OP_DUP);
			}

			switch (initInfo.fieldType)
			{

			case kFSITSuper:
			{
				StructVocabulary *pParentVocab = pVocab->BaseVocabulary();
				ASSERT(pParentVocab != NULL);
				int32_t parentInitOp = pVocab->BaseVocabulary()->GetInitOpcode();
				ASSERT(parentInitOp != 0);
                pOuter->CompileOpcode(parentInitOp);
				break;
			}

			case kFSITString:
			{
				pOuter->CompileOpcode(kOpOffset, initInfo.offset + 8);
				pOuter->CompileOpcode(kOpConstant, initInfo.len);
				pOuter->CompileBuiltinOpcode(OP_INIT_STRING);
				break;
			}

			case kFSITStruct:
			{
				StructVocabulary *pStructVocab = mStructInfo[initInfo.typeIndex].pVocab;
				ASSERT(pStructVocab != NULL);
				int32_t structFieldInitOpcode = pStructVocab->GetInitOpcode();
				ASSERT(structFieldInitOpcode != 0);
				if (initInfo.offset != 0)
				{
					pOuter->CompileOpcode(kOpOffset, initInfo.offset);
				}
				pOuter->CompileOpcode(structFieldInitOpcode);
				break;
			}

			case kFSITStringArray:
			{
				// TOS: maximum length, number of elements, ptr to first char of first element
				pOuter->CompileOpcode(kOpOffset, initInfo.offset + 8);
				pOuter->CompileOpcode(kOpConstant, initInfo.numElements);
				pOuter->CompileOpcode(kOpConstant, initInfo.len);
				pOuter->CompileBuiltinOpcode(OP_INIT_STRING_ARRAY);
				break;
			}

			case kFSITStructArray:
			{
				// TOS: struct index, number of elements, ptr to first struct
				if (initInfo.offset != 0)
				{
					pOuter->CompileOpcode(kOpOffset, initInfo.offset);
				}
				pOuter->CompileOpcode(kOpConstant, initInfo.numElements);
				pOuter->CompileOpcode(kOpConstant, initInfo.typeIndex);
				pOuter->CompileBuiltinOpcode(OP_INIT_STRUCT_ARRAY);
				break;
			}

			default:
				// TODO!
				ASSERT(false);
				break;
			}

		}
		pOuter->CompileBuiltinOpcode(OP_DO_EXIT);
		pOuter->EndOpDefinition(true);
		mFieldInitInfos.clear();

		pOuter->SetDefinitionVocabulary(pOldDefinitionsVocab);
	}
	else
	{
		// newest struct/class doesn't have any fields which need initializers
		StructVocabulary *pParentVocab = pVocab->BaseVocabulary();
		if (pParentVocab != NULL)
		{
			int32_t parentInitOp = pVocab->BaseVocabulary()->GetInitOpcode();
			if (parentInitOp != 0)
			{
				pVocab->SetInitOpcode(parentInitOp);
			}
		}
	}
}

ClassVocabulary*
TypesManager::StartClassDefinition(const char *pName, int classIndex, bool isInterface)
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthVocabulary* pDefinitionsVocab = pOuter->GetDefinitionVocabulary();
	ForthTypeInfo *pInfo = NULL;

	if (classIndex >= kNumBuiltinClasses)
	{
		classIndex = static_cast<eBuiltinClassIndex>(mStructInfo.size());
		mStructInfo.emplace_back(ForthTypeInfo(NULL, OP_ABORT, classIndex));
	}
	mNewestTypeIndex = classIndex;
	pInfo = &(mStructInfo[classIndex]);

    // can't smudge class definition, since method definitions will be nested inside it
    forthop* pEntry = pOuter->StartOpDefinition( pName, false, kOpUserDefImmediate );
	pInfo->pVocab = isInterface ? new InterfaceVocabulary(pName, classIndex)
        : new ClassVocabulary(pName, classIndex);
    pInfo->op = *pEntry;
	SPEW_STRUCTS("StartClassDefinition %s struct index %d\n", pName, classIndex);
	pInfo->typeIndex = classIndex;
	mpSavedDefinitionVocab = pOuter->GetDefinitionVocabulary();
    pOuter->SetDefinitionVocabulary( pInfo->pVocab );
    return (ClassVocabulary*) (pInfo->pVocab);
}

void
TypesManager::EndClassDefinition()
{
    SPEW_STRUCTS( "EndClassDefinition\n" );
    ForthEngine *pEngine = ForthEngine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->EndOpDefinition( false );
	DefineInitOpcode();
	pOuter->SetDefinitionVocabulary(mpSavedDefinitionVocab);
    mpSavedDefinitionVocab = NULL;
}

StructVocabulary*
TypesManager::GetStructVocabulary( forthop op )
{
    //TBD: replace this with a map
	for (const ForthTypeInfo& info : mStructInfo)
    {
        if ( info.op == op )
        {
            return info.pVocab;
        }
    }
	return nullptr;
}

StructVocabulary*
TypesManager::GetStructVocabulary( const char* pName )
{
    //TBD: replace this with a map
	for (const ForthTypeInfo& info : mStructInfo)
	{
		StructVocabulary* pVocab = info.pVocab;
        if ((pVocab != nullptr) && (strcmp( info.pVocab->GetName(), pName ) == 0))
        {
            return pVocab;
        }
    }
	return nullptr;
}

ForthTypeInfo*
TypesManager::GetTypeInfo( int typeIndex )
{
	int numStructs = static_cast<int>(mStructInfo.size());
	if (typeIndex >= numStructs)
    {
        SPEW_STRUCTS( "GetTypeInfo error: typeIndex is %d, only %d structs exist\n",
			typeIndex, numStructs);
        return NULL;
    }
    return &(mStructInfo[ typeIndex ]);
}

ClassVocabulary*
TypesManager::GetClassVocabulary(int typeIndex) const
{
	int numStructs = static_cast<int>(mStructInfo.size());
	if (typeIndex >= numStructs)
	{
		SPEW_STRUCTS("GetClassVocabulary error: typeIndex is %d, only %d types exist\n", typeIndex, numStructs);
		return nullptr;
	}
	StructVocabulary* pVocab = mStructInfo[typeIndex].pVocab;
	if ((pVocab == nullptr) || !pVocab->IsClass())
	{
		return nullptr;

	}
	return static_cast<ClassVocabulary *>(pVocab);
}

ForthInterface*
TypesManager::GetClassInterface(int typeIndex, int interfaceIndex) const
{
	int numStructs = static_cast<int>(mStructInfo.size());
	if (typeIndex >= numStructs)
	{
		SPEW_STRUCTS("GetClassInterface error: typeIndex is %d, only %d types exist\n", typeIndex, numStructs);
		return nullptr;
	}
	StructVocabulary* pVocab = mStructInfo[typeIndex].pVocab;
	if (!pVocab->IsClass())
	{
		SPEW_STRUCTS("GetClassInterface error: vocabulary %s is not a class\n", pVocab->GetName());
		return nullptr;

	}
	ClassVocabulary* pClassVocab = static_cast<ClassVocabulary *>(pVocab);
	return pClassVocab->GetInterface(interfaceIndex);
}

TypesManager*
TypesManager::GetInstance( void )
{
    ASSERT( mpInstance != NULL );
    return mpInstance;
}

void
TypesManager::GetFieldInfo( int32_t fieldType, int32_t& fieldBytes, int32_t& alignment )
{
    BaseType subType = CODE_TO_BASE_TYPE(fieldType);
    if ( CODE_IS_PTR( fieldType ) )
    {
        fieldBytes = sizeof(char *);
        alignment = fieldBytes;
    }
    else if ( CODE_IS_NATIVE( fieldType ) )
    {
        alignment = gpNativeTypes[(ucell)subType]->GetAlignment();
        if ( subType == BaseType::kString )
        {
            // add in for maxLen, curLen fields & terminating null byte
            fieldBytes = (12 + CODE_TO_STRING_BYTES( fieldType )) & ~(alignment - 1);
        }
        else
        {
            fieldBytes = gpNativeTypes[(ucell)subType]->GetSize();
        }
    }
    else
    {
        int32_t typeIndex = (subType == BaseType::kObject) ? CODE_TO_CONTAINED_CLASS_INDEX(fieldType) : CODE_TO_STRUCT_INDEX(fieldType);
        ForthTypeInfo* pInfo = GetTypeInfo(typeIndex);
        if ( pInfo )
        {
            alignment = pInfo->pVocab->GetAlignment();
			fieldBytes = pInfo->pVocab->IsClass() ? sizeof(ForthObject) : pInfo->pVocab->GetSize();
        }
    }
}

StructVocabulary *
TypesManager::GetNewestStruct( void )
{
    TypesManager* pThis = GetInstance();
	return pThis->mStructInfo[mNewestTypeIndex].pVocab;
}

ClassVocabulary *
TypesManager::GetNewestClass( void )
{
    TypesManager* pThis = GetInstance();
	StructVocabulary* pVocab = pThis->mStructInfo[mNewestTypeIndex].pVocab;
	if (pVocab && !pVocab->IsClass())
    {
        pVocab = NULL;
    }
    return (ClassVocabulary *) pVocab;
}


// compile/interpret symbol if is a valid structure accessor
bool
TypesManager::ProcessSymbol( ForthParseInfo *pInfo, OpResult& exitStatus )
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    ForthCoreState* pCore = pEngine->GetCoreState();
    ForthVocabulary *pFoundVocab = NULL;
    // ProcessSymbol will compile opcodes into temporary buffer mCode
    forthop *pDst = &(mCode[0]);

	bool result = mpCodeGenerator->Generate( pInfo, pDst, MAX_ACCESSOR_LONGS );
	if ( result )
	{
		// when done, either compile (copy) or execute code in mCode buffer
		if ( pOuter->IsCompiling() )
		{
			int nLongs = (int)(pDst - &(mCode[0]));
			if ( mpCodeGenerator->UncompileLastOpcode() )
			{
				pOuter->UncompileLastOpcode();
			}
			for ( int i = 0; i < nLongs; i++ )
			{
				pOuter->CompileOpcode( mCode[i] );
			}
		}
		else
		{
			*pDst++ = gCompiledOps[ OP_DONE ];
			exitStatus = pEngine->ExecuteOps(pCore, &(mCode[0]));
		}
	}
    return result;
}

// compile symbol if it is a member variable or method
bool
TypesManager::ProcessMemberSymbol( ForthParseInfo *pInfo, OpResult& exitStatus, VarOperation varop)
{
    ForthEngine *pEngine = ForthEngine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop *pDst = &(mCode[0]);
    ForthVocabulary *pFoundVocab = NULL;
    ForthCoreState* pCore = pEngine->GetCoreState();

    ClassVocabulary* pVocab = GetNewestClass();
    if ( pVocab == NULL )
    {
        // TBD: report no newest class??
        return false;
    }

    const char* pToken = pInfo->GetToken();
    forthop* pEntry = pVocab->FindSymbol( pToken );
    if ( pEntry )
    {
        int32_t offset = *pEntry;
        int32_t typeCode = pEntry[1];
        uint32_t opType;
        bool isNative = CODE_IS_NATIVE( typeCode );
        bool isPtr = CODE_IS_PTR( typeCode );
        bool isArray = CODE_IS_ARRAY( typeCode );
        BaseType baseType = CODE_TO_BASE_TYPE( typeCode );

		if ( CODE_IS_USER_DEFINITION( typeCode ) )
		{
			// this is a normal forthop, let outer interpreter process it
			return false;
		}
        if ( CODE_IS_METHOD( typeCode ) )
        {
            // this is a method invocation on current object
            opType = kOpMethodWithThis;
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
        }
        else
        {
            // this is a member variable of current object
            if ( isPtr )
            {
                SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
                opType = (isArray) ? kOpMemberCellArray : kOpMemberCell;
            }
            else
            {
                if ( baseType == BaseType::kStruct )
                {
                    if ( isArray )
                    {
                        *pDst++ = COMPILED_OP( kOpMemberRef, offset );
                        opType = kOpArrayOffset;
                        offset = pEntry[2];
                    }
                    else
                    {
                        opType = kOpMemberRef;
                    }
                }
                else
                {
                    opType = (uint32_t)((isArray) ? kOpMemberByteArray : kOpMemberByte) + (uint32_t)CODE_TO_BASE_TYPE( typeCode );
                }
            }
            offset |= (((int)varop) << 20);
            SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, offset ) );
            *pDst++ = COMPILED_OP( opType, offset );
        }
    }
    else
    {
        // token isn't in current class vocabulary
        return false;
    }
    // when done, compile (copy) the code in mCode buffer
    int nLongs = (int)(pDst - &(mCode[0]));
    if ( nLongs )
    {
        for (int i = 0; i < nLongs; ++i)
        {
            pOuter->CompileOpcode(mCode[i]);
        }
    }
    return true;
}


NativeType*
TypesManager::GetNativeTypeFromName( const char* typeName )
{
    for ( int i = 0; i <= (int)BaseType::kObject; i++ )
    {
        if ( strcmp( gpNativeTypes[i]->GetName(), typeName ) == 0 )
        {
            return gpNativeTypes[i];
        }
    }
    return NULL;
}


BaseType
TypesManager::GetBaseTypeFromName( const char* typeName )
{
	NativeType* pNative = GetNativeTypeFromName( typeName );
	return (pNative != NULL) ? pNative->GetBaseType() : BaseType::kUnknown;
}


int32_t
TypesManager::GetBaseTypeSizeFromName( const char* typeName )
{
    for ( int i = 0; i <= (int)BaseType::kObject; i++ )
    {
        if ( strcmp( gpNativeTypes[i]->GetName(), typeName ) == 0 )
        {
            return gpNativeTypes[i]->GetSize();
        }
    }
    return -1;
}

// TypesManager::AddBuiltinClasses is defined in ForthBuiltinClasses.cpp

forthop*
TypesManager::GetClassMethods()
{
    return mpClassMethods;
}

void TypesManager::AddFieldInitInfo(const ForthFieldInitInfo& fieldInitInfo)
{
	mFieldInitInfos.push_back(fieldInitInfo);
}

//////////////////////////////////////////////////////////////////////
////
///     StructVocabulary
//
//

StructVocabulary::StructVocabulary( const char    *pName,
                                              int           typeIndex )
: ForthVocabulary( pName, NUM_STRUCT_VOCAB_VALUE_LONGS, DEFAULT_VOCAB_STORAGE )
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
    ForthVocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode;
    bool isPtr = false;
    TypesManager* pManager = TypesManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();        // so we can GET_VAR_OPERATION

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
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );
    StructVocabulary* pVocab = this;
    while ( pVocab )
    {
        pEntry = pVocab->ForthVocabulary::FindSymbol( &parseInfo, serial );
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
    ForthCoreState* pCore = mpEngine->GetCoreState();
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
StructVocabulary::ShowData(const void* pData, ForthCoreState* pCore, bool showId)
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
StructVocabulary::ShowDataInner(const void* pData, ForthCoreState* pCore, StructVocabulary* pEndVocab)
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

//////////////////////////////////////////////////////////////////////
////
///     ClassVocabulary
//
//
ClassVocabulary* ClassVocabulary::smpObjectClass = NULL;

ClassVocabulary::ClassVocabulary( const char*     pName,
                                            int             typeIndex )
: StructVocabulary( pName, typeIndex )
, mpParentClass( NULL )
, mCurrentInterface( 0 )
, mCustomReader(nullptr)
, mpClassObject(nullptr)
{
    mpClassObject = new ForthClassObject;
    // mpClassObject->pMethods will be filled in the first time GetClassObject is called
    //  after the Class class is defined
    mpClassObject->pMethods = nullptr;
    mpClassObject->refCount = 1;				// TBD: should this be 0? or a huge number?
    mpClassObject->pVocab = this;
    mpClassObject->newOp = gCompiledOps[OP_ALLOC_OBJECT];
    ForthInterface* pPrimaryInterface = new ForthInterface( this );
    mInterfaces.push_back( pPrimaryInterface );

	if ( strcmp( pName, "Object" ) == 0 )
	{
		smpObjectClass = this;
	}
	else
	{
		if ( strcmp( pName, "Class" ) != 0 )
		{
			Extends( smpObjectClass );
		}
	}

    mType = VocabularyType::kClass;
}


ClassVocabulary::~ClassVocabulary()
{
    for ( uint32_t i = 0; i < mInterfaces.size(); i++ )
    {
        delete mInterfaces[i];
    }
    delete mpClassObject;
}


void
ClassVocabulary::DefineInstance( void )
{
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    char* pInstanceName = pOuter->GetNextSimpleToken();
    char* pContainedClassName = nullptr;
    if (::strcmp(pInstanceName, "of") == 0)
    {
        pContainedClassName = pOuter->AddTempString(pOuter->GetNextSimpleToken());
        pInstanceName = pOuter->AddTempString(pOuter->GetNextSimpleToken());
    }
    DefineInstance(pInstanceName, pContainedClassName);
}


void
ClassVocabulary::DefineInstance(char* pInstanceName, const char* pContainedClassName)
{
    // do one of the following:
    // - define a global instance of this class type
    // - define a local instance of this class type
    // - define a field of this class type
    int nBytes = sizeof(ForthObject *);
    ForthObject* pHere;
    ForthVocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode;
    bool isPtr = false;
    TypesManager* pManager = TypesManager::GetInstance();
    ForthCoreState *pCore = mpEngine->GetCoreState();
    OuterInterpreter* pOuter = mpEngine->GetOuterInterpreter();
    int32_t typeIndex = mTypeIndex;

    // if new instance name ends in '!', chop the '!' and initialize the new instance
    size_t instanceNameLen = strlen(pInstanceName);
    bool doInitializationVarop = false;
    if (instanceNameLen > 1 && pInstanceName[instanceNameLen - 1] == '!' && pOuter->CheckFeature(kFFAllowVaropSuffix))
    {
        instanceNameLen--;
        pInstanceName[instanceNameLen] = '\0';
        doInitializationVarop = true;
    }

    if (pContainedClassName != nullptr)
    {
        ForthVocabulary* pFoundVocab;
        pEntry = pOuter->GetVocabularyStack()->FindSymbol(pContainedClassName, &pFoundVocab);
        if (pEntry != nullptr)
        {
            ClassVocabulary* pContainedClassVocab = (ClassVocabulary *)(pManager->GetStructVocabulary(pEntry[0]));
            if (pContainedClassVocab && pContainedClassVocab->IsClass())
            {
                typeIndex = (typeIndex << 16) | pContainedClassVocab->GetTypeIndex();
            }
            else
            {
                mpEngine->SetError(ForthError::kUnknownSymbol, "class define instance: bad contained class");
                return;
            }
        }
        else
        {
            mpEngine->SetError(ForthError::kUnknownSymbol, "class define instance: contained class not found");
            return;
        }
    }
    int32_t numElements = pOuter->GetArraySize();
    bool isArray = (numElements != 0);
    int32_t arrayFlag = (isArray) ? kDTIsArray : 0;
    pOuter->SetArraySize( 0 );
    typeCode = OBJECT_TYPE_TO_CODE( arrayFlag, typeIndex );

    if (pOuter->CheckFlag( kEngineFlagIsPointer ) )
    {
        pOuter->ClearFlag( kEngineFlagIsPointer );
        nBytes = sizeof(ForthObject **);
        typeCode |= kDTIsPtr;
        isPtr = true;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( pOuter->IsCompiling() )
    {
        // define local object
        pVocab = pOuter->GetLocalVocabulary();
        if ( isArray )
        {
            pOuter->SetArraySize( numElements );
            pOuter->AddLocalArray( pInstanceName, typeCode, nBytes );
        }
        else
        {
            pHere = (ForthObject*)mpEngine->GetDP();
            bool bCompileInstanceOp = pOuter->GetLastCompiledIntoPtr() == (((forthop *)pHere) - 1);
            pOuter->AddLocalVar( pInstanceName, typeCode, nBytes );
            if (doInitializationVarop)
            {
                // local var name ended with '!', so compile op for this local var with varop Set
                //  so it will be initialized
                forthop* pEntry = pVocab->GetNewestEntry();
                forthop op = COMPILED_OP((isPtr ? kOpLocalCell : kOpLocalObject), pEntry[0]);
                pOuter->CompileOpcode(op | ((forthop)VarOperation::kVarSet) << 20);
            }
            else if (bCompileInstanceOp)
            {
                // local var definition was preceeded by "->", so compile the op for this local var
                //  so it will be initialized
                forthop* pEntry = pVocab->GetNewestEntry();
                pOuter->CompileOpcode( (isPtr ? kOpLocalCell : kOpLocalObject), pEntry[0] );
            }
        }
    }
    else
    {
		if ( pOuter->CheckFlag( kEngineFlagInStructDefinition ) )
		{
			pManager->GetNewestStruct()->AddField( pInstanceName, typeCode, numElements );
			return;
		}

        // define global object(s)
        int32_t newGlobalOp = pOuter->AddUserOp( pInstanceName );

		// create object which will release object referenced by this global when it is forgotten
		new ForthForgettableGlobalObject( pInstanceName, mpEngine->GetDP(), newGlobalOp, isArray ? numElements : 1 );

        pEntry = pOuter->GetDefinitionVocabulary()->GetNewestEntry();
        if ( isArray )
        {
            pOuter->CompileBuiltinOpcode( isPtr ? OP_DO_CELL_ARRAY : OP_DO_OBJECT_ARRAY );
            pHere = (ForthObject*)mpEngine->GetDP();
            if (!isPtr)
            {
                pOuter->AddGlobalObjectVariable(pHere, this, pInstanceName);
            }
            mpEngine->AllotLongs((nBytes * numElements) >> 2);
            memset( pHere, 0, (nBytes * numElements) );
            if ( !(typeCode & kDTIsPtr) )
            {
                for ( int i = 0; i < numElements; i++ )
                {
                    pHere[i] = 0;
                }
            }
        }
        else
        {

            pOuter->CompileBuiltinOpcode( isPtr ? OP_DO_CELL : OP_DO_OBJECT );
            pHere = (ForthObject*)mpEngine->GetDP();
            mpEngine->AllotLongs( nBytes >> 2 );
            memset( pHere, 0, nBytes );
            if ( !isPtr )
            {
                pOuter->AddGlobalObjectVariable(pHere, this, pInstanceName);
            }

            if ( GET_VAR_OPERATION == VarOperation::kVarSet || doInitializationVarop)
            {
                ForthObject srcObj = (ForthObject)SPOP;
                if ( isPtr )
                {
                    *pHere = srcObj;
                }
                else
                {
					// bump objects refcount
					if (srcObj != nullptr)
					{
                        srcObj->refCount += 1;
					}
                    *pHere = srcObj;
                }
                CLEAR_VAR_OPERATION;
            }
        }
        pEntry[1] = typeCode;
    }
}

int
ClassVocabulary::AddMethod( const char*    pName,
								 int			methodIndex,
                                 forthop        op )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
    if ( methodIndex < 0 )
    {
        // method name was not found in current interface
        if ( mCurrentInterface == 0 )
        {
            // add new method to primary interface
            methodIndex = pCurInterface->AddMethod( op );
        }
        else
        {
            // TBD: report error - trying to add a method to a secondary interface
        }
	}
    else
    {
        // overwrite method
        pCurInterface->SetMethod( methodIndex, op );
    }
    return methodIndex;
}

int
ClassVocabulary::FindMethod( const char* pName )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
    return pCurInterface->GetMethodIndex( pName );
}


// TODO: find a better way to do this
extern forthop gObjectDeleteOpcode;
extern forthop gObjectShowInnerOpcode;


void
ClassVocabulary::Extends( ClassVocabulary *pParentClass )
{
	if ( pParentClass->IsClass() )
	{
		int32_t numInterfaces = pParentClass->GetNumInterfaces();
		for ( int i = 1; i < numInterfaces; i++ )
		{
			delete mInterfaces[i];
			mInterfaces[i] = NULL;
		}
		mpParentClass = pParentClass;
		numInterfaces = mpParentClass->GetNumInterfaces();
		mInterfaces.resize( numInterfaces );
		bool isPrimaryInterface = true;
		for ( int i = 0; i < numInterfaces; i++ )
		{
			if ( !isPrimaryInterface )
			{
				mInterfaces[i] = new ForthInterface;
			}
			mInterfaces[i]->Copy( mpParentClass->GetInterface( i ), isPrimaryInterface );

			isPrimaryInterface = false;
		}
        mInterfaces[0]->GetMethods()[kMethodDelete] = gObjectDeleteOpcode;
        mInterfaces[0]->GetMethods()[kMethodShowInner] = gObjectShowInnerOpcode;
        mpClassObject->newOp = pParentClass->mpClassObject->newOp;
	}

	StructVocabulary::Extends( pParentClass );
}


void
ClassVocabulary::Implements( const char* pName )
{
	StructVocabulary* pVocab = TypesManager::GetInstance()->GetStructVocabulary( pName );

	if ( pVocab )
	{
		if ( pVocab->IsClass() )
		{
			ClassVocabulary* pClassVocab = reinterpret_cast<ClassVocabulary *>(pVocab);
            int32_t interfaceIndex = FindInterfaceIndex( pClassVocab->GetClassId() );
            if ( interfaceIndex > 0 )
            {
                // this is an interface we have already inherited from superclass
                mCurrentInterface = interfaceIndex;
            }
            else if ( interfaceIndex < 0 )
            {
                // this is an interface which this class doesn't already have
                ForthInterface* pNewInterface = new ForthInterface(pClassVocab);
                pNewInterface->Implements( pClassVocab );
                mCurrentInterface = mInterfaces.size();
                mInterfaces.push_back( pNewInterface );
            }
            else
            {
                // report error - target of "implements" is same class!
                mpEngine->SetError(ForthError::kBadSyntax, "interface is base class");
            }
		}
		else
		{
			// report that vocab is struct, not class
            mpEngine->SetError(ForthError::kBadSyntax, "interface is struct, not class");
        }
	
	}
	else
	{
        mpEngine->SetError(ForthError::kBadSyntax, "interface unknown");
    }
}


void
ClassVocabulary::EndImplements()
{
	// TBD: report error if not all methods implemented
    mCurrentInterface = 0;
}

ForthInterface*
ClassVocabulary::GetInterface( int32_t index )
{
	return (index < static_cast<int32_t>(mInterfaces.size())) ? mInterfaces[index] : nullptr;
}

ForthInterface* ClassVocabulary::GetCurrentInterface()
{
    return GetInterface(mCurrentInterface);
}

forthop* ClassVocabulary::GetMethods()
{
    return mInterfaces[0]->GetMethods();
}

int32_t
ClassVocabulary::FindInterfaceIndex( int32_t classId )
{
    for ( uint32_t i = 0; i < mInterfaces.size(); i++ )
    {
        ClassVocabulary* pVocab = mInterfaces[i]->GetDefiningClass();
        if ( pVocab->GetClassId() == classId )
        {
            return i;
        }
    }
	return -1;
}


int32_t
ClassVocabulary::GetNumInterfaces( void )
{
	return (int32_t) mInterfaces.size();
}


ForthClassObject*
ClassVocabulary::GetClassObject( void )
{
    if (mpClassObject->pMethods == nullptr)
    {
        // this is gross, but it gets around an order of creation circular dependancy
        mpClassObject->pMethods = TypesManager::GetInstance()->GetClassMethods();
    }
    
    return mpClassObject;
}


void ClassVocabulary::FixClassObjectMethods(void)
{
    // this needs to be called on all class vocabularies which are created before
    // the "Class" vocabulary - namely "Object" and "Class" itself
    // this is a little gross, but it gets around an order of creation circular dependancy
    //   and avoids Class and Object having no bellybuttons
    mpClassObject->pMethods = TypesManager::GetInstance()->GetClassMethods();
}


void
ClassVocabulary::PrintEntry(forthop*   pEntry )
{
    char buff[BUFF_SIZE];
    char nameBuff[128];
    int32_t methodNum = *pEntry;
    int32_t typeCode = pEntry[1];
    BaseType baseType = CODE_TO_BASE_TYPE(typeCode);
    
    if (baseType == BaseType::kUserDefinition)
    {
        ForthVocabulary::PrintEntry(pEntry);
        return;
    }

    bool isMethod = CODE_IS_METHOD( typeCode );
    if ( !isMethod )
    {
        StructVocabulary::PrintEntry( pEntry );
        return;
    }

    ForthCoreState* pCore = mpEngine->GetCoreState();
    forthop* pMethods = GetMethods();

    sprintf( buff, "  %08x    ", methodNum );
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

    sprintf( buff, "method # %d returning ", methodNum );
    CONSOLE_STRING_OUT( buff );

    if ( CODE_IS_ARRAY( typeCode ) )
    {
        CONSOLE_STRING_OUT( "array of " );
    }

    if ( CODE_IS_PTR( typeCode ) )
    {
        CONSOLE_STRING_OUT( "pointer to " );
    }

    if ( CODE_IS_NATIVE( typeCode ) )
    {
        BaseType baseType = CODE_TO_BASE_TYPE( typeCode );
        sprintf( buff, "%s ", gpNativeTypes[(ucell)baseType]->GetName() );
        CONSOLE_STRING_OUT( buff );
        if ( baseType == BaseType::kString )
        {
            sprintf( buff, "strLen=%d, ", CODE_TO_STRING_BYTES( typeCode ) );
            CONSOLE_STRING_OUT( buff );
        }
    }
    else
    {
        BaseType baseType = CODE_TO_BASE_TYPE(typeCode);
        if (baseType == BaseType::kObject)
        {
            int32_t containedTypeIndex = CODE_TO_CONTAINED_CLASS_INDEX(typeCode);
            ForthTypeInfo* pContainedInfo = TypesManager::GetInstance()->GetTypeInfo(containedTypeIndex);
            if (pContainedInfo)
            {
                int32_t containerTypeIndex = CODE_TO_CONTAINER_CLASS_INDEX(typeCode);
                if (containerTypeIndex == kBCIInvalid)
                {
                    sprintf(buff, "%s", pContainedInfo->pVocab->GetName());
                }
                else
                {
                    ForthTypeInfo* pContainerInfo = TypesManager::GetInstance()->GetTypeInfo(containerTypeIndex);
                    if (pContainerInfo)
                    {
                        sprintf(buff, "%s of %s", pContainerInfo->pVocab->GetName(), pContainedInfo->pVocab->GetName());
                    }
                    else
                    {
                        sprintf(buff, "<UNKNOWN CONTAINER CLASS INDEX %d!>", containerTypeIndex);
                    }
                }
            }
            else
            {
                sprintf(buff, "<UNKNOWN CLASS INDEX %d!>", containedTypeIndex);
            }
        }
        else if (baseType == BaseType::kStruct)
        {
            int32_t typeIndex = CODE_TO_STRUCT_INDEX(typeCode);
            ForthTypeInfo* pInfo = TypesManager::GetInstance()->GetTypeInfo(typeIndex);
            if (pInfo)
            {
                sprintf(buff, "%s ", pInfo->pVocab->GetName());
            }
            else
            {
                sprintf(buff, "<UNKNOWN STRUCT INDEX %d!> ", typeIndex);
            }
        }
        else if (baseType == BaseType::kUserDefinition)
        {
            strcpy(buff, "user defined forthop ");
        }
        else if (baseType == BaseType::kVoid)
        {
            strcpy(buff, "void ");
        }
        else
        {
            sprintf(buff, "UNKNOWN BASE TYPE %d", (int)baseType);
        }
        CONSOLE_STRING_OUT( buff );
    }
    forthop* pMethod = pMethods + methodNum;
    sprintf( buff, " opcode=%02x:%06x", GetEntryType(pMethod), GetEntryValue(pMethod) );
    CONSOLE_STRING_OUT( buff );
}

ClassVocabulary*
ClassVocabulary::ParentClass( void )
{
	return ((mpSearchNext != nullptr) && mpSearchNext->IsClass()) ? (ClassVocabulary *) mpSearchNext : NULL;
}

const char *
ClassVocabulary::GetTypeName( void )
{
    return "classVocabulary";
}

void ClassVocabulary::SetCustomObjectReader(CustomObjectReader reader)
{
    mCustomReader = reader;
}

CustomObjectReader ClassVocabulary::GetCustomObjectReader()
{
    return mCustomReader;
}

// TBD: implement FindSymbol which iterates over all interfaces


//////////////////////////////////////////////////////////////////////
////
///     InterfaceVocabulary
//
//

InterfaceVocabulary::InterfaceVocabulary(const char* pName, int typeIndex)
    : ClassVocabulary(pName, typeIndex)
{
    mType = VocabularyType::kInterface;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthInterface
//
//

// INTERFACE_SKIPPED_ENTRIES is the number of longword entries at start of mMethods
//   which is used to hold the class vocabulary pointer
#if defined(FORTH64)
#define INTERFACE_SKIPPED_ENTRIES 2
#else
#define INTERFACE_SKIPPED_ENTRIES 1
#endif

ForthInterface::ForthInterface( ClassVocabulary* pDefiningClass )
: mpDefiningClass( pDefiningClass )
, mNumAbstractMethods( 0 )
{
    ForthClassObject* classObject = pDefiningClass->GetClassObject();
#if defined(FORTH64)
    // TODO: do we need to worry about this not having 64-bit alignment?
    mMethods.push_back(0);
    mMethods.push_back(0);
    ForthClassObject** pDst = (ForthClassObject **)(&mMethods[0]);
    *pDst = classObject;
#else
    mMethods.push_back((forthop)classObject);
#endif
}


ForthInterface::~ForthInterface()
{
}


void
ForthInterface::Copy( ForthInterface* pInterface, bool isPrimaryInterface )
{
	if ( !isPrimaryInterface )
	{
	    mpDefiningClass = pInterface->GetDefiningClass();
	}
    mNumAbstractMethods = pInterface->mNumAbstractMethods;
    int numMethods = (int)(pInterface->mMethods.size());
    mMethods.resize( numMethods );
    for ( int i = INTERFACE_SKIPPED_ENTRIES; i < numMethods; i++ )
    {
        mMethods[i] = pInterface->mMethods[i];
    }
}


ClassVocabulary*
ForthInterface::GetDefiningClass()
{
    return mpDefiningClass;
}


forthop*
ForthInterface::GetMethods( void )
{
	return &(mMethods[0]) + INTERFACE_SKIPPED_ENTRIES;
}


forthop
ForthInterface::GetMethod( int index )
{
    // TBD: check index in bounds
	return mMethods[index + INTERFACE_SKIPPED_ENTRIES];
}


void
ForthInterface::SetMethod( int index, forthop method )
{
    index += INTERFACE_SKIPPED_ENTRIES;
    if (index < mMethods.size())
    {
        if (mMethods[index] != method)
        {
            if (method != gCompiledOps[OP_UNIMPLEMENTED])
            {
                mNumAbstractMethods--;
            }
            mMethods[index] = method;
            // NOTE: we don't support the case where the old method was concrete and the new method is abstract
        }
    }
    else
    {
        ForthEngine::GetInstance()->SetError(ForthError::kBadArrayIndex, "attempt to set interface method with out-of-bounds index");
    }
}


extern FORTHOP(oInterfaceDeleteMethod);
void ForthInterface::Implements( ClassVocabulary* pVocab )
{
    ForthInterface* pInterface = pVocab->GetInterface( 0 );
    int numMethods = (int)(pInterface->mMethods.size());
    mMethods.resize( numMethods );
	for ( int i = INTERFACE_SKIPPED_ENTRIES; i < numMethods; i++ )
	{
        mMethods[i] = pInterface->mMethods[i];
	}
    
    mNumAbstractMethods = numMethods;
}


int
ForthInterface::AddMethod( forthop method )
{
    int methodIndex = (int)(mMethods.size() - INTERFACE_SKIPPED_ENTRIES);
	mMethods.push_back( method );
    if ( method == gCompiledOps[OP_UNIMPLEMENTED] )
    {
        mNumAbstractMethods++;
    }
    return methodIndex;
}


int
ForthInterface::GetNumMethods( void )
{
    return static_cast<int32_t>( mMethods.size() - INTERFACE_SKIPPED_ENTRIES);
}


int
ForthInterface::GetNumAbstractMethods( void )
{
    return mNumAbstractMethods;
}


int
ForthInterface::GetMethodIndex( const char* pName )
{
	forthop* pEntry = NULL;
	bool done = false;
	int methodIndex = -1;
	StructVocabulary* pVocab = mpDefiningClass;

	while ( !done )
	{
		pEntry = pVocab->FindNextSymbol( pName, pEntry );
		if ( pEntry )
		{
			int32_t typeCode = pEntry[1];
			if ( CODE_IS_METHOD( typeCode ) )
			{
				methodIndex = pEntry[0];
				done = true;
			}
		}
		else
		{
			pVocab = pVocab->BaseVocabulary();
			if ( (pVocab == NULL) || !pVocab->IsClass() )
			{
				done = true;
			}
		}
	}
	return methodIndex;
}


//////////////////////////////////////////////////////////////////////
////
///     NativeType
//
//

NativeType::NativeType( const char*       pName,
                                  int               numBytes,
                                  BaseType   baseType )
: mpName( pName )
, mNumBytes( numBytes )
, mBaseType( baseType )
{
}

NativeType::~NativeType()
{
}

void
NativeType::DefineInstance( ForthEngine *pEngine, void *pInitialVal, int32_t flags )
{
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pToken = pOuter->GetNextSimpleToken();
    int nBytes = mNumBytes;
    char *pHere;
    int i;
    int32_t val = 0;
    BaseType baseType = mBaseType;
    ForthVocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode, len, varOffset, storageLen;
    char *pStr;
    ForthCoreState *pCore = pEngine->GetCoreState();        // so we can SPOP maxbytes
    TypesManager* pManager = TypesManager::GetInstance();
    int tokenLen = (int)strlen(pToken);

    // if new instance name ends in '!', chop the '!' and initialize the new instance
    bool doInitializationVarop = false;
    if (tokenLen > 1 && pToken[tokenLen - 1] == '!' && pOuter->CheckFeature(kFFAllowVaropSuffix))
    {
        tokenLen--;
        pToken[tokenLen] = '\0';
        doInitializationVarop = true;
    }

    bool isString = (baseType == BaseType::kString);

	if (!isString && ((pOuter->GetFlags() & kEngineFlagInEnumDefinition) != 0))
	{
		// byte/short/int/int32_t inside an enum definition sets the number of bytes an enum of this type requires
        ForthEnumInfo* pNewestEnum = pOuter->GetNewestEnumInfo();
        if (pNewestEnum != nullptr)
        {
            pNewestEnum->size = mNumBytes;
        }
		return;
	}

	if ( isString )
    {
        // get maximum string length
        if ( pOuter->IsCompiling() )
        {
            // the symbol just before "string" should have been an integer constant
            if ( !pOuter->GetLastConstant( len ) )
            {
                SET_ERROR( ForthError::kMissingSize );
            }
        }
        else
        {
            len = (int32_t)(SPOP);
        }
    }

    int32_t numElements = pOuter->GetArraySize();
    if ( numElements != 0 )
    {
        flags |= kDTIsArray;
    }
    pOuter->SetArraySize( 0 );
    if ( isString )
    {
        typeCode = STRING_TYPE_TO_CODE( flags, len );
    }
    else
    {
        typeCode = NATIVE_TYPE_TO_CODE( flags, baseType );
    }

    bool isPointer = pOuter->CheckFlag(kEngineFlagIsPointer);
    if (isPointer)
    {
        // outside of a struct definition, any native variable or array defined with "ptrTo"
        //  is the same thing as an int variable or array, since native types have no fields
        pOuter->ClearFlag(kEngineFlagIsPointer);
        baseType = BaseType::kCell;
        nBytes = sizeof(char *);
        pInitialVal = &val;
        typeCode |= kDTIsPtr;
        isString = false;
    }

    if ( pOuter->InStructDefinition() && !pOuter->IsCompiling() )
    {
        pManager->GetNewestStruct()->AddField( pToken, typeCode, numElements );
        return;
    }

    // get next symbol, add it to vocabulary with type "user op"
    if ( !isString )
    {
        if ( pOuter->IsCompiling() )
        {
            // define local variable
            pVocab = pOuter->GetLocalVocabulary();
            if ( numElements )
            {
                pOuter->SetArraySize( numElements );
                pOuter->AddLocalArray( pToken, typeCode, nBytes );
            }
            else
            {
                pHere = (char *) (pEngine->GetDP());
                bool bCompileInstanceOp = pOuter->GetLastCompiledIntoPtr() == (((forthop *)pHere) - 1);
                pOuter->AddLocalVar(pToken, typeCode, nBytes);
                if (doInitializationVarop)
                {
                    // local var name ended with '!', so compile op for this local var with varop Set
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    pOuter->CompileOpcode(pEntry[0] | ((forthop) VarOperation::kVarSet) << 20);
                }
                else if (bCompileInstanceOp)
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    pOuter->CompileOpcode(pEntry[0]);
                }
            }
        }
        else
        {
            // define global variable
            pOuter->AddUserOp( pToken );
            pEntry = pOuter->GetDefinitionVocabulary()->GetNewestEntry();
            pEntry[1] = typeCode;

            if ( numElements )
            {
                // define global array
                pOuter->CompileBuiltinOpcode(OP_DO_BYTE_ARRAY + (uint32_t)CODE_TO_BASE_TYPE((int)baseType));
                pHere = (char *) (pEngine->GetDP());
                pEngine->AllotLongs( ((nBytes * numElements) + 3) >> 2 );
                for ( i = 0; i < numElements; i++ )
                {
                    memcpy( pHere, pInitialVal, nBytes );
                    pHere += nBytes;
                }
            }
            else
            {
                // define global single variable
                pOuter->CompileBuiltinOpcode(OP_DO_BYTE + (uint32_t)CODE_TO_BASE_TYPE((int)baseType));
                pHere = (char *) (pEngine->GetDP());
                pEngine->AllotLongs( (nBytes  + 3) >> 2 );
                if (doInitializationVarop)
                {
                    SET_VAR_OPERATION(VarOperation::kVarSet);
                }

                if (GET_VAR_OPERATION == VarOperation::kVarSet )
                {
                    // var definition was preceeded by "->", or name ended in '!', so initialize var
#if 1
                    if (nBytes == 8)
                    {
#if defined(FORTH64)
                        cell val = SPOP;
                        *((int64_t *)pHere) = val;
#else
                        if (baseType == BaseType::kDouble)
                        {
                            *((double*)pHere) = DPOP;
                        }
                        else
                        {
                            stackInt64 sval;
                            LPOP(sval);
                            *((int64_t*)pHere) = sval.s64;
                        }
#endif
                    }
                    else
                    {
                        cell val = SPOP;
                        switch (nBytes)
                        {
                        case 1:
                            *pHere = (char)val;
                            break;
                        case 2:
                            *((short *)pHere) = (short)val;
                            break;
                        case 4:
                            *((int *)pHere) = (int)val;
                            break;
                        default:
                            // TODO! complain bad int size
                            break;
                        }
                    }
                    CLEAR_VAR_OPERATION;
#else
                    pEngine->ExecuteOp(pCore, pEntry[0]);
#endif
                }
                else
                {
                    memcpy( pHere, pInitialVal, nBytes );
                }
            }
        }
    }
    else
    {
        // instance is string
        if ( pOuter->IsCompiling() )
        {
            pVocab = pOuter->GetLocalVocabulary();
            // uncompile the integer contant opcode - it is the string maxLen
            pOuter->UncompileLastOpcode();
            storageLen = ((len >> 2) + 3) << 2;

            if ( numElements )
            {
                // define local string array
				pOuter->SetArraySize(numElements);
				varOffset = pOuter->AddLocalArray(pToken, typeCode, storageLen);
				pOuter->CompileOpcode(kOpLocalRef, varOffset - 2);
				pOuter->CompileOpcode(kOpConstant, numElements);
                pOuter->CompileOpcode( kOpConstant, len );
                pOuter->CompileBuiltinOpcode( OP_INIT_STRING_ARRAY );
            }
            else
            {
                // define local string variable
                pHere = (char *) (pEngine->GetDP());
                bool bCompileInstanceOp = pOuter->GetLastCompiledIntoPtr() == (((forthop *)pHere) - 1);
                varOffset = pOuter->AddLocalVar( pToken, typeCode, storageLen );
                // compile initLocalString op
                varOffset = (varOffset << 12) | len;
                pOuter->CompileOpcode( kOpLocalStringInit, varOffset );
                if (doInitializationVarop)
                {
                    // local var name ended with '!', so compile op for this local var with varop Set
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    pOuter->CompileOpcode(pEntry[0] | ((forthop)VarOperation::kVarSet) << 20);
                }
                else if (bCompileInstanceOp)
                {
                    // local var definition was preceeded by "->", so compile the op for this local var
                    //  so it will be initialized
                    forthop* pEntry = pVocab->GetNewestEntry();
                    pOuter->CompileOpcode(pEntry[0]);
                }
            }
        }
        else
        {
            pOuter->AddUserOp( pToken );
            pEntry = pOuter->GetDefinitionVocabulary()->GetNewestEntry();
            pEntry[1] = typeCode;

            if ( numElements )
            {
                // define global string array
                pOuter->CompileBuiltinOpcode( OP_DO_STRING_ARRAY );
                for ( i = 0; i < numElements; i++ )
                {
                    pOuter->CompileInt( len );
                    pOuter->CompileInt( 0 );
                    pStr = (char *) (pEngine->GetDP());
                    // a length of 4 means room for 4 chars plus a terminating null
                    pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
                    *pStr = 0;
                }
            }
            else
            {
                // define global string variable
                pOuter->CompileBuiltinOpcode( OP_DO_STRING );
                pOuter->CompileInt( len );
                pOuter->CompileInt( 0 );
                pStr = (char *) (pEngine->GetDP());
                // a length of 4 means room for 4 chars plus a terminating null
                pEngine->AllotLongs( ((len  + 4) & ~3) >> 2 );
                *pStr = 0;
                if (doInitializationVarop)
                {
                    SET_VAR_OPERATION(VarOperation::kVarSet);
                }

                if (GET_VAR_OPERATION == VarOperation::kVarSet)
                {
                    // var definition was preceeded by "->", so initialize var
                    pEngine->ExecuteOp(pCore,  pEntry[0] );
                }
            }
        }
    }
}
