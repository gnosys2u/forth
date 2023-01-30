//////////////////////////////////////////////////////////////////////
//
// ClassVocabulary.cpp: support for user-defined classes
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Engine.h"
#include "OuterInterpreter.h"
#include "ClassVocabulary.h"
#include "TypesManager.h"
#include "NativeType.h"
#include "LocalVocabulary.h"

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


void ClassVocabulary::DefineInstance( void )
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


void ClassVocabulary::DefineInstance(char* pInstanceName, const char* pContainedClassName)
{
    // do one of the following:
    // - define a global instance of this class type
    // - define a local instance of this class type
    // - define a field of this class type
    int nBytes = sizeof(ForthObject *);
    ForthObject* pHere;
    Vocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode;
    bool isPtr = false;
    TypesManager* pManager = TypesManager::GetInstance();
    CoreState *pCore = mpEngine->GetCoreState();
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
        Vocabulary* pFoundVocab;
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

int ClassVocabulary::AddMethod( const char*    pName,
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

int ClassVocabulary::FindMethod( const char* pName )
{
	ForthInterface* pCurInterface = mInterfaces[ mCurrentInterface ];
	// see if method name is already defined - if so, just overwrite the method longword with op
	// if name is not already defined, add the method name and op
    return pCurInterface->GetMethodIndex( pName );
}


// TODO: find a better way to do this
extern forthop gObjectDeleteOpcode;
extern forthop gObjectShowInnerOpcode;


void ClassVocabulary::Extends( ClassVocabulary *pParentClass )
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


void ClassVocabulary::Implements( const char* pName )
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


void ClassVocabulary::EndImplements()
{
	// TBD: report error if not all methods implemented
    mCurrentInterface = 0;
}

ForthInterface* ClassVocabulary::GetInterface( int32_t index )
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

int32_t ClassVocabulary::FindInterfaceIndex( int32_t classId )
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


int32_t ClassVocabulary::GetNumInterfaces( void )
{
	return (int32_t) mInterfaces.size();
}


ForthClassObject* ClassVocabulary::GetClassObject( void )
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


void ClassVocabulary::PrintEntry(forthop*   pEntry )
{
#define BUFF_SIZE 512
    char buff[BUFF_SIZE];
    char nameBuff[128];
    int32_t methodNum = *pEntry;
    int32_t typeCode = pEntry[1];
    BaseType baseType = CODE_TO_BASE_TYPE(typeCode);
    
    if (baseType == BaseType::kUserDefinition)
    {
        Vocabulary::PrintEntry(pEntry);
        return;
    }

    bool isMethod = CODE_IS_METHOD( typeCode );
    if ( !isMethod )
    {
        StructVocabulary::PrintEntry( pEntry );
        return;
    }

    CoreState* pCore = mpEngine->GetCoreState();
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

ClassVocabulary* ClassVocabulary::ParentClass( void )
{
	return ((mpSearchNext != nullptr) && mpSearchNext->IsClass()) ? (ClassVocabulary *) mpSearchNext : NULL;
}

const char * ClassVocabulary::GetTypeName( void )
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
        Engine::GetInstance()->SetError(ForthError::kBadArrayIndex, "attempt to set interface method with out-of-bounds index");
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


