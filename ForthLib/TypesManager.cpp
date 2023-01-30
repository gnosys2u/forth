//////////////////////////////////////////////////////////////////////
//
// TypesManager.cpp: manager of structs and classes
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthEngine.h"
#include "TypesManager.h"
#include "NativeType.h"
#include "OuterInterpreter.h"
#include "ForthParseInfo.h"
#include "ClassVocabulary.h"

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
	mpCodeGenerator = new StructCodeGenerator(this);

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

void TypesManager::ForgetCleanup( void *pForgetLimit, forthop op )
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


StructVocabulary* TypesManager::StartStructDefinition( const char *pName )
{
    Engine *pEngine = Engine::GetInstance();
    Vocabulary* pDefinitionsVocab = pEngine->GetOuterInterpreter()->GetDefinitionVocabulary();

    forthop *pEntry = pEngine->GetOuterInterpreter()->StartOpDefinition( pName, true, kOpUserDefImmediate );
	mNewestTypeIndex = static_cast<int>(mStructInfo.size());
	StructVocabulary* pVocab = new StructVocabulary(pName, (int)mNewestTypeIndex);
	mStructInfo.emplace_back(ForthTypeInfo(pVocab, *pEntry, (int)mNewestTypeIndex));
	SPEW_STRUCTS("StartStructDefinition %s struct index %d\n", pName, mNewestTypeIndex);
    return pVocab;
}

void TypesManager::EndStructDefinition()
{
    SPEW_STRUCTS( "EndStructDefinition\n" );
    Engine *pEngine = Engine::GetInstance();
    pEngine->GetOuterInterpreter()->EndOpDefinition( true );
    GetNewestStruct()->EndDefinition();
	DefineInitOpcode();
}

void TypesManager::DefineInitOpcode()
{
	ForthTypeInfo *pInfo = &(mStructInfo[mNewestTypeIndex]);
	StructVocabulary *pVocab = pInfo->pVocab;

	// TODO: define new init opcode
	if (mFieldInitInfos.size() > 0)
	{
		Engine *pEngine = Engine::GetInstance();
        OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();

		Vocabulary* pOldDefinitionsVocab = pOuter->GetDefinitionVocabulary();
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

ClassVocabulary* TypesManager::StartClassDefinition(const char *pName, int classIndex, bool isInterface)
{
    Engine *pEngine = Engine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    Vocabulary* pDefinitionsVocab = pOuter->GetDefinitionVocabulary();
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

void TypesManager::EndClassDefinition()
{
    SPEW_STRUCTS( "EndClassDefinition\n" );
    Engine *pEngine = Engine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    pOuter->EndOpDefinition( false );
	DefineInitOpcode();
	pOuter->SetDefinitionVocabulary(mpSavedDefinitionVocab);
    mpSavedDefinitionVocab = NULL;
}

StructVocabulary* TypesManager::GetStructVocabulary( forthop op )
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

StructVocabulary* TypesManager::GetStructVocabulary( const char* pName )
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

ForthTypeInfo* TypesManager::GetTypeInfo( int typeIndex )
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

ClassVocabulary* TypesManager::GetClassVocabulary(int typeIndex) const
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

ForthInterface* TypesManager::GetClassInterface(int typeIndex, int interfaceIndex) const
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

TypesManager* TypesManager::GetInstance( void )
{
    ASSERT( mpInstance != NULL );
    return mpInstance;
}

void TypesManager::GetFieldInfo( int32_t fieldType, int32_t& fieldBytes, int32_t& alignment )
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

StructVocabulary * TypesManager::GetNewestStruct( void )
{
    TypesManager* pThis = GetInstance();
	return pThis->mStructInfo[mNewestTypeIndex].pVocab;
}

ClassVocabulary * TypesManager::GetNewestClass( void )
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
bool TypesManager::ProcessSymbol( ParseInfo *pInfo, OpResult& exitStatus )
{
    Engine *pEngine = Engine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    CoreState* pCore = pEngine->GetCoreState();
    Vocabulary *pFoundVocab = NULL;
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
bool TypesManager::ProcessMemberSymbol( ParseInfo *pInfo, OpResult& exitStatus, VarOperation varop)
{
    Engine *pEngine = Engine::GetInstance();
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    forthop *pDst = &(mCode[0]);
    Vocabulary *pFoundVocab = NULL;
    CoreState* pCore = pEngine->GetCoreState();

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


NativeType* TypesManager::GetNativeTypeFromName( const char* typeName )
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


BaseType TypesManager::GetBaseTypeFromName( const char* typeName )
{
	NativeType* pNative = GetNativeTypeFromName( typeName );
	return (pNative != NULL) ? pNative->GetBaseType() : BaseType::kUnknown;
}


int32_t TypesManager::GetBaseTypeSizeFromName( const char* typeName )
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

forthop* TypesManager::GetClassMethods()
{
    return mpClassMethods;
}

void TypesManager::AddFieldInitInfo(const ForthFieldInitInfo& fieldInitInfo)
{
	mFieldInitInfos.push_back(fieldInitInfo);
}

