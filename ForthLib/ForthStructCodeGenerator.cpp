//////////////////////////////////////////////////////////////////////
//
// ForthStructCodeGenerator.cpp: code generator for structures and objects
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ForthEngine.h"
#include "ForthVocabulary.h"
#include "ForthStructs.h"
#include "ForthStructCodeGenerator.h"
#include "ForthBuiltinClasses.h"
#include "ForthParseInfo.h"

// symbol entry layout for struct vocabulary (fields and method symbols
// offset   contents
//  0..3    field offset or method index
//  4..7    field data type or method return type
//  8..12   element size for arrays
//  13      1-byte symbol length (not including padding)
//  14..    symbol characters
//
// see forth.h for most current description of struct symbol entry fields

//////////////////////////////////////////////////////////////////////
////
///     ForthStructCodeGenerator
//
//

ForthStructCodeGenerator::ForthStructCodeGenerator( ForthTypesManager* pTypeManager )
    : mpParseInfo(nullptr)
    , mpTypeManager( pTypeManager )
    , mpStructVocab(nullptr)
    , mpContainedClassVocab(nullptr)
    , mpDst(nullptr)
    , mpDstBase(nullptr)
    , mDstLongs( 0 )
    , mpBuffer(nullptr)
    , mpToken(nullptr)
    , mpNextToken(nullptr)
    , mCompileVarop( 0 )
    , mOffset( 0 )
    , mTypeCode( BASE_TYPE_TO_CODE(kBaseTypeVoid) )
    , mTOSTypeCode( BASE_TYPE_TO_CODE(kBaseTypeVoid) )
    , mUsesSuper(false)
{
	mBufferBytes = 512;
	mpBuffer = (char *)__MALLOC(mBufferBytes);
}

ForthStructCodeGenerator::~ForthStructCodeGenerator()
{
	__FREE(mpBuffer);
}

bool ForthStructCodeGenerator::Generate( ForthParseInfo *pInfo, long*& pDst, int dstLongs )
{
	mpParseInfo = pInfo;
	mpStructVocab = nullptr;
    mpContainedClassVocab = nullptr;
	const char* pSource = mpParseInfo->GetToken();
	int srcBytes = strlen( pSource ) + 1;
	if ( srcBytes > mBufferBytes )
	{
		mBufferBytes = srcBytes + 64;
		mpBuffer = (char*)__REALLOC(mpBuffer, mBufferBytes);
	}
    // get first token

    strcpy( mpBuffer, pSource );
	mpToken = mpBuffer;
	mpNextToken = strchr( mpToken, '.' );
	if ( mpNextToken == NULL )
	{
		//SPEW ERROR!
		return false;
	}
	*mpNextToken++ = '\0';
	
	mpDst = pDst;
	mDstLongs = dstLongs;
	
	bool success = HandleFirst();
	if ( success )
	{
		mpToken = mpNextToken;
		while ( success && (mpToken != NULL) )
		{
			mpNextToken = strchr( mpToken, '.' );
		    mTOSTypeCode = mTypeCode;
			if ( mpNextToken == NULL )
			{
				success = HandleLast();
			}
			else
			{
				*mpNextToken++ = '\0';
				success = HandleMiddle();
			}
			mpToken = mpNextToken;
		}
	}
	pDst = mpDst;
	return success;
}

void ForthStructCodeGenerator::HandlePreceedingVarop()
{
    // handle case where previous opcode was varAction setting op (one of [ref -> ->+ ->- oclear])
    // we need to execute the varAction setting op after the first op, since if the first op is
    // a pointer type, it will use the varAction and clear it, when the varAction is meant to be
    // used by the final field op
    ForthEngine *pEngine = ForthEngine::GetInstance();
    ForthCoreState *pCore = pEngine->GetCoreState();

	mCompileVarop = 0;
    if ( pEngine->IsCompiling() )
    {
        long *pLastOp = pEngine->GetLastCompiledOpcodePtr();
        if ( pLastOp && ((pLastOp + 1) == GET_DP)
            && (*pLastOp >= gCompiledOps[OP_FETCH]) && (*pLastOp <= gCompiledOps[OP_OCLEAR]) )
        {
            // overwrite the varAction setting op with first accessor op
            mCompileVarop = *pLastOp;
        }
    }
    else
    {
        // we are interpreting, clear any existing varAction, but compile an op to set it after first op
        ulong varMode = GET_VAR_OPERATION;
        if ( varMode )
        {
            CLEAR_VAR_OPERATION;
            mCompileVarop = gCompiledOps[OP_FETCH] + (varMode - kVarFetch);
        }
    }
}

#if defined(LINUX) || defined(MACOSX)
#define COMPILE_OP( _caption, _opType, _opData ) *mpDst++ = COMPILED_OP(_opType, _opData)
#define COMPILE_SIMPLE_OP( _caption, _op ) *mpDst++ = _op
#else
#define COMPILE_OP( _caption, _opType, _opData ) SPEW_STRUCTS( " " _caption " 0x%x", COMPILED_OP(_opType, _opData) ); *mpDst++ = COMPILED_OP(_opType, _opData)
#define COMPILE_SIMPLE_OP( _caption, _op ) SPEW_STRUCTS( " " _caption " 0x%x", _op ); *mpDst++ = _op
#endif

bool ForthStructCodeGenerator::HandleFirst()
{
	bool success = true;
    ForthEngine *pEngine = ForthEngine::GetInstance();
	ForthVocabulary* pFoundVocab = NULL;
    mpStructVocab = nullptr;
    mpContainedClassVocab = nullptr;
    mUsesSuper = false;

    HandlePreceedingVarop();
    
	mTypeCode = BASE_TYPE_TO_CODE(kBaseTypeVoid);
	mTOSTypeCode = BASE_TYPE_TO_CODE(kBaseTypeVoid);

    // see if first token is local or global struct
    long* pEntry = NULL;

    if ( mpParseInfo->GetFlags() & PARSE_FLAG_HAS_COLON )
    {
        int tokenLength = mpParseInfo->GetTokenLength();
        char* pColon = strchr( mpToken, ':' );
        if ( pColon != NULL )
        {
            int colonPos = pColon - mpToken;
            if ( (tokenLength > 4) && (colonPos > 0) && (colonPos < (tokenLength - 2)) )
            {
                ////////////////////////////////////
                //
                // symbol may be of form VOCABULARY:SYMBOL.STUFF
                //
                ////////////////////////////////////
                *pColon = '\0';
                ForthVocabulary* pVocab = ForthVocabulary::FindVocabulary( mpToken );
                if ( pVocab != NULL )
                {
                    pEntry = pVocab->FindSymbol( pColon + 1 );
                    if ( pEntry != NULL )
                    {
                        pFoundVocab = pVocab;
                    }
                }
                *pColon = ':';
            }
        }
    }   // endif token contains colon

    bool explicitTOSCast = false;
    bool isClassReference = false;
    char* pLastChar = mpNextToken - 2;
	if ( pEntry == NULL )
	{
		if ( strcmp( mpToken, "super") == 0 )
		{
			ForthStructVocabulary* pSuperVocab = mpTypeManager->GetNewestClass()->ParentClass();
			if ( pSuperVocab != NULL )
			{
				explicitTOSCast = true;
                mUsesSuper = true;
				mTypeCode = STRUCT_TYPE_TO_CODE( kDTIsPtr, pSuperVocab->GetTypeIndex() );
			}
			else
			{
                SPEW_STRUCTS( "Invalid use of super\n" );
                return false;
			}
		}
		else if ( (*mpToken == '<') && (*pLastChar == '>') )
		{
			////////////////////////////////////
			//
			// symbol may be of form <STRUCTTYPE>.STUFF
			//
			////////////////////////////////////
			*pLastChar = '\0';
			ForthStructVocabulary* pCastVocab = mpTypeManager->GetStructVocabulary( mpToken + 1 );
			if ( pCastVocab != NULL )
			{
				explicitTOSCast = true;
				mTypeCode = pCastVocab->IsClass() ? OBJECT_TYPE_TO_CODE( kDTIsPtr, pCastVocab->GetTypeIndex() )
												  : STRUCT_TYPE_TO_CODE( kDTIsPtr, pCastVocab->GetTypeIndex() );
			}
			*pLastChar = '>';
		}
    }

	bool isMember = false;
    if ( !explicitTOSCast )
    {
        //
        // this isn't an explicit cast with <TYPE>, so try to determine the
        //  structure type of the first token with a vocabulary search
        //
        if ( pEntry == NULL && pEngine->IsCompiling())
        {
            pEntry = pEngine->GetLocalVocabulary()->FindSymbol( mpToken );
        }

        if ( pEntry == NULL )
        {
            pEntry = pEngine->GetVocabularyStack()->FindSymbol( mpToken, &pFoundVocab );
            if ( pEntry != NULL )
            {
                // see if mToken is a class name
                ForthStructVocabulary* pClassVocab = mpTypeManager->GetStructVocabulary( mpToken );
                if ( (pClassVocab != NULL) && pClassVocab->IsClass() )
                {
					////////////////////////////////////
					//
					// symbol is of the form CLASSNAME.STUFF
					//
					////////////////////////////////////
					// TODO! this doesn't work currently, need to compile the vocabulary op, at least
                    // this is invoking a class method on a class object (IE CLASSNAME.setNew)
                    // the first compiled opcode is a constantOp whose value is the class type index,
                    // followed by the getClassByIndex opcode
                    isClassReference = true;
					COMPILE_OP("class type index", kOpConstant, pClassVocab->GetTypeIndex());

                    *mpDst++ = gCompiledOps[OP_GET_CLASS_BY_INDEX];
                    mTypeCode = OBJECT_TYPE_TO_CODE( 0, kBCIClass );
                }

				if ( pFoundVocab != NULL )
				{
					isMember = pFoundVocab->IsClass();
				}
            }
        }

        if ( pEntry == NULL )
        {
            // token not found in search or local vocabs
            return false;
        }

        // see if token is a struct
        if ( !isClassReference )
        {
            mTypeCode = pEntry[1];
            if ( CODE_IS_NATIVE( mTypeCode ) )
            {
                SPEW_STRUCTS( "Native type cant have fields\n" );
                return false;
            }
        }
    }   // endif ( !explicitTOSCast )

    bool isPtr = CODE_IS_PTR( mTypeCode );
    bool isArray = CODE_IS_ARRAY( mTypeCode );
    long baseType = CODE_TO_BASE_TYPE( mTypeCode );
    bool isObject = (baseType == kBaseTypeObject);
    bool isMethod = CODE_IS_METHOD( mTypeCode );
    //long compileVarop = 0;

    if (!explicitTOSCast)
    {
        SPEW_STRUCTS( "First field %s op 0x%x\n", mpToken, pEntry[0] );
		if ( isMember )
		{
			// first symbol is either a member variable or method
			switch ( baseType )
			{
				case kBaseTypeObject:
					break;

				case kBaseTypeStruct:
					break;

				default:
				{
					// ERROR! must return object or struct
					sprintf( mErrorMsg, "Method %s return value is not an object or struct", mpToken );
					pEngine->SetError( kForthErrorStruct, mErrorMsg );
					return false;
				}
			}

			if ( isMethod )
			{
				// this method must return either a struct or an object
                COMPILE_OP("method with this", kOpMethodWithThis, pEntry[0]);
                long typeIndex = isObject ? CODE_TO_CONTAINED_CLASS_INDEX(mTypeCode) : CODE_TO_STRUCT_INDEX(mTypeCode);
                ForthTypeInfo* pStruct = mpTypeManager->GetTypeInfo(typeIndex);
				if ( pStruct == NULL )
				{
					pEngine->SetError( kForthErrorStruct, "Method return type not found by type manager" );
					return false;
				}
			}
			else
			{
				// first symbol is non-final member variable
				// struct: do nothing (offset already added in)
				// ptr to struct: compile offset, compile @
				// array of structs: compile offset, compile array offset
				// array of ptrs to structs: compile offset, compile array offset, compile at
				// object: compile offset, compile d@
				// ptr to object: compile offset, compile @, compile d@
				// array of objects: compile offset, compile array offset, compile d@
				// array of ptrs to objects: compile offset, compile array offset, compile at, compile d@
				if ( isArray )
				{
					if ( isObject )
					{
						if ( isPtr )
						{
							COMPILE_OP( "object ptr array", kOpMemberIntArray, pEntry[0] );
							COMPILE_SIMPLE_OP( "ifetch", gCompiledOps[OP_IFETCH] );
						}
						else
						{
							COMPILE_OP( "object array", kOpMemberObjectArray, pEntry[0] );
						}
					}
					else
					{
						// member struct
						if ( isPtr )
						{
							COMPILE_OP( "member struct ptr array", kOpMemberIntArray, pEntry[0] );
							COMPILE_SIMPLE_OP( "fetch", gCompiledOps[OP_IFETCH] );
						}
						else
						{
							COMPILE_OP( "member struct array", kOpMemberRef, pEntry[0] );
							COMPILE_OP( "arrayOffsetOp", kOpArrayOffset, pEntry[2] );
						}
					}
				}
				else
				{
					if ( isObject )
					{
						if ( isPtr )
						{
							COMPILE_OP( "object ptr", kOpMemberInt, pEntry[0] );
							COMPILE_SIMPLE_OP( "ifetch", gCompiledOps[OP_IFETCH] );
						}
						else
						{
							COMPILE_OP( "object", kOpMemberObject, pEntry[0] );
						}
					}
					else
					{
						if ( isPtr )
						{
							// just a struct
							COMPILE_OP( "member struct ptr", kOpMemberRef, pEntry[0] );
							COMPILE_SIMPLE_OP( "fetch", gCompiledOps[OP_IFETCH] );
						}
						else
						{
							// just a struct
							COMPILE_OP( "member struct", kOpMemberRef, pEntry[0] );
						}
					}
				}
			}
		}
		else
		{
			if (!isClassReference)
			{
				// first symbol is a global or local variable
				*mpDst++ = pEntry[0];
			}
		}
        if ( isObject && isPtr )
        {
            *mpDst++ = gCompiledOps[OP_IFETCH];
        }
    }

    if (isObject)
    {
        long containedTypeIndex = CODE_TO_CONTAINED_CLASS_INDEX(mTypeCode);
        long containerTypeIndex = CODE_TO_CONTAINER_CLASS_INDEX(mTypeCode);

        if (containerTypeIndex == kBCIInvalid)
        {
            ForthTypeInfo* pClassInfo = mpTypeManager->GetTypeInfo(containedTypeIndex);
            if (pClassInfo == NULL)
            {
                SPEW_STRUCTS("First field not found by types manager\n");
                return false;
            }
            else
            {
                // no container type
                mpStructVocab = pClassInfo->pVocab;
                SPEW_STRUCTS("First field of type %s\n", mpStructVocab->GetName());
            }
        }
        else
        {
            ForthTypeInfo* pContainerInfo = mpTypeManager->GetTypeInfo(containerTypeIndex);
            if (pContainerInfo == NULL)
            {
                SPEW_STRUCTS("First field container type not found by types manager\n");
                return false;
            }
            else
            {
                ForthTypeInfo* pContainedClassInfo = mpTypeManager->GetTypeInfo(containedTypeIndex);
                if (pContainedClassInfo == NULL)
                {
                    SPEW_STRUCTS("First field contained type not found by types manager\n");
                    return false;
                }
                else
                {
                    mpStructVocab = pContainerInfo->pVocab;
                    mpContainedClassVocab = pContainedClassInfo->pVocab;
                    SPEW_STRUCTS("First field of type %s of %s\n", mpStructVocab->GetName(), mpContainedClassVocab->GetName());
                }
            }
        }
    }
    else
    {
        ForthTypeInfo* pStructInfo = mpTypeManager->GetTypeInfo(CODE_TO_STRUCT_INDEX(mTypeCode));
        if (pStructInfo == NULL)
        {
            SPEW_STRUCTS("First field not found by types manager\n");
            return false;
        }
        else
        {
            mpStructVocab = pStructInfo->pVocab;
            SPEW_STRUCTS("First field of type %s\n", mpStructVocab->GetName());
        }
    }
    
    mOffset = 0;
    
	return success;
}
	
bool ForthStructCodeGenerator::HandleMiddle()
{
    bool bUsesSuper = mUsesSuper;
    mUsesSuper = false;
	if ( mpStructVocab == NULL )
	{
		return false;
	}
	bool success = true;
    ForthEngine *pEngine = ForthEngine::GetInstance();
    
    long* pEntry = mpStructVocab->FindSymbol( mpToken );
			
    SPEW_STRUCTS( "field %s", mpToken );
    if ( pEntry == NULL )
    {
        SPEW_STRUCTS( " not found!\n" );
        return false;
    }
    mTypeCode = pEntry[1];
    if ( CODE_IS_USER_DEFINITION( mTypeCode ) )
    {
		// we bail out her if the entry found is for an internal colonOp definition.
		//  class colonOps cannot be applied to an arbitrary object instance since they
		//  do not set the this pointer, class colonOps can only be used inside a class method
		//  which has already set the this pointer
        SPEW_STRUCTS( " not found!\n" );
        return false;
    }
    bool isNative = CODE_IS_NATIVE( mTypeCode );
    bool isPtr = CODE_IS_PTR( mTypeCode );
    bool isArray = CODE_IS_ARRAY( mTypeCode );
    long baseType = CODE_TO_BASE_TYPE( mTypeCode );
    bool isObject = (baseType == kBaseTypeObject);
    bool isMethod = CODE_IS_METHOD( mTypeCode );
    long opType;
    mOffset += pEntry[0];
    SPEW_STRUCTS( " offset %d", mOffset );
    if ( isArray )
    {
        SPEW_STRUCTS( (isPtr) ? " array of pointers" : " array" );
    }
    else if ( isPtr )
    {
        SPEW_STRUCTS( " pointer" );
    }

    bool bSetStructVocab = false;
    if ( isMethod )
    {
        // This is a method which is a non-final accessor field
        // this method must return either a struct or an object
        opType = (bUsesSuper) ? kOpMethodWithSuper : kOpMethodWithTOS;
        mOffset = pEntry[0];
        SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, mOffset ) );
        *mpDst++ = COMPILED_OP( opType, mOffset );
        mOffset = 0;
        switch (baseType)
        {
            case kBaseTypeObject:
            case kBaseTypeStruct:
                bSetStructVocab = true;
                break;

            default:
            {
                // ERROR! method must return object or struct
                sprintf( mErrorMsg, "Method %s return value is not an object or struct", mpToken );
                pEngine->SetError( kForthErrorStruct, mErrorMsg );
                return false;
            }
        }
    }
    else
    {
        //
        // this is not the final accessor field
        //
        if ( isNative )
        {
            // ERROR! a native field must be a final accessor
            sprintf( mErrorMsg, "Native %s used for non-final accessor", mpToken );
            pEngine->SetError( kForthErrorStruct, mErrorMsg );
            return false;
        }
        // struct: do nothing (offset already added in)
        // ptr to struct: compile offset, compile @
        // array of structs: compile offset, compile array offset
        // array of ptrs to structs: compile offset, compile array offset, compile at
        // object: compile offset, compile d@
        // ptr to object: compile offset, compile @, compile d@
        // array of objects: compile offset, compile array offset, compile d@
        // array of ptrs to objects: compile offset, compile array offset, compile at, compile d@
        if ( isArray || isPtr )
        {
            if ( mOffset )
            {
                SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, mOffset ) );
                *mpDst++ = COMPILED_OP( kOpOffset, mOffset );
                mOffset = 0;
            }
            if ( isArray )
            {
                // TBD: verify the element size for arrays of ptrs to structs is 4
                SPEW_STRUCTS( " arrayOffsetOp 0x%x", COMPILED_OP( kOpArrayOffset, pEntry[2] ) );
                *mpDst++ = COMPILED_OP( kOpArrayOffset, pEntry[2] );
            }
            if ( isPtr )
            {
                SPEW_STRUCTS( " ifetchOp 0x%x", gCompiledOps[OP_IFETCH] );
                *mpDst++ = gCompiledOps[OP_IFETCH];
            }
            if ( isObject )
            {
                SPEW_STRUCTS( " ifetchOp 0x%x", gCompiledOps[OP_IFETCH] );
                *mpDst++ = gCompiledOps[OP_IFETCH];
            }
        }
        else if ( isObject )
        {
            bSetStructVocab = true;
            if (mOffset)
            {
                SPEW_STRUCTS( " offsetOp 0x%x", COMPILED_OP( kOpOffset, mOffset ) );
                *mpDst++ = COMPILED_OP( kOpOffset, mOffset );
                mOffset = 0;
            }
            SPEW_STRUCTS( " ifetchOp 0x%x", gCompiledOps[OP_IFETCH] );
            *mpDst++ = gCompiledOps[OP_IFETCH];
        }
        if (baseType == kBaseTypeStruct)
        {
            bSetStructVocab = true;
        }

    }
    if (bSetStructVocab)
    {
        long containedTypeIndex = CODE_TO_CONTAINED_CLASS_INDEX(mTypeCode);
        long containerTypeIndex = CODE_TO_CONTAINER_CLASS_INDEX(mTypeCode);

        if (containerTypeIndex == kBCIInvalid)
        {
            if (containedTypeIndex == kBCIContainedType)
            {
                if (mpContainedClassVocab != nullptr)
                {
                    mpStructVocab = mpContainedClassVocab;
                }
            }
            else
            {
                ForthTypeInfo* pClassInfo = mpTypeManager->GetTypeInfo(containedTypeIndex);
                if (pClassInfo == NULL)
                {
                    SPEW_STRUCTS("Return type not found by types manager\n");
                    return false;
                }
                else
                {
                    // no container type
                    mpStructVocab = pClassInfo->pVocab;
                    SPEW_STRUCTS("Return type %s\n", mpStructVocab->GetName());
                }
            }
        }
        else
        {
            ForthTypeInfo* pContainerInfo = mpTypeManager->GetTypeInfo(containerTypeIndex);
            if (pContainerInfo == NULL)
            {
                SPEW_STRUCTS("Return container type not found by types manager\n");
                return false;
            }
            else
            {
                ForthTypeInfo* pContainedClassInfo = mpTypeManager->GetTypeInfo(containedTypeIndex);
                if (pContainedClassInfo == NULL)
                {
                    SPEW_STRUCTS("Return type not found by types manager\n");
                    return false;
                }
                else
                {
                    mpStructVocab = pContainerInfo->pVocab;
                    mpContainedClassVocab = pContainedClassInfo->pVocab;
                    SPEW_STRUCTS("Return type %s of %s\n", mpStructVocab->GetName(), mpContainedClassVocab->GetName());
                }
            }
        }
    }
    return success;
}
	
bool ForthStructCodeGenerator::HandleLast()
{
	if ( mpStructVocab == NULL )
	{
		return false;
	}
	bool success = true;
    
    long* pEntry = mpStructVocab->FindSymbol( mpToken );
		
    SPEW_STRUCTS( "field %s", mpToken );
    if ( pEntry == NULL )
    {
        SPEW_STRUCTS( " not found!\n" );
        return false;
    }
    mTypeCode = pEntry[1];
    if ( CODE_IS_USER_DEFINITION( mTypeCode ) )
    {
		// we bail out her if the entry found is for an internal colonOp definition.
		//  class colonOps cannot be applied to an arbitrary object instance since they
		//  do not set the this pointer, class colonOps can only be used inside a class method
		//  which has already set the this pointer
        SPEW_STRUCTS( " not found!\n" );
        return false;
    }
    
    bool isNative = CODE_IS_NATIVE( mTypeCode );
    bool isPtr = CODE_IS_PTR( mTypeCode );
    bool isArray = CODE_IS_ARRAY( mTypeCode );
    long baseType = CODE_TO_BASE_TYPE( mTypeCode );
    bool isObject = (baseType == kBaseTypeObject);
    bool isMethod = CODE_IS_METHOD( mTypeCode );
    long opType;
    mOffset += pEntry[0];
    SPEW_STRUCTS( " offset %d", mOffset );
    
    if ( isArray )
    {
        SPEW_STRUCTS( (isPtr) ? " array of pointers" : " array" );
    }
    else if ( isPtr )
    {
        SPEW_STRUCTS( " pointer" );
    }

    //
    // this is final accessor field
    //
    if ( mCompileVarop != 0 )
    {
        // compile variable-mode setting op just before final field
        *mpDst++ = mCompileVarop;
    }
    SPEW_STRUCTS( " FINAL" );
    if ( isMethod )
    {
        opType = (mUsesSuper) ? kOpMethodWithSuper : kOpMethodWithTOS;
        mOffset = pEntry[0];     // method#
    }
    else if ( isPtr )
    {
        SPEW_STRUCTS( (isArray) ? " array of pointers\n" : " pointer\n" );
        opType = (isArray) ? kOpFieldIntArray : kOpFieldInt;
    }
    else
    {
        if ( isNative || isObject )
        {
            opType = ((isArray) ? kOpFieldByteArray : kOpFieldByte) + CODE_TO_BASE_TYPE( mTypeCode );
        }
        else
        {
            if ( isArray )
            {
                *mpDst++ = COMPILED_OP( kOpArrayOffset, pEntry[2] );
            }
            opType = kOpOffset;
        }
    }
    SPEW_STRUCTS( " opcode 0x%x\n", COMPILED_OP( opType, mOffset ) );
    *mpDst++ = COMPILED_OP( opType, mOffset );

    if (isMethod && mUsesSuper)
    {
        // compile op that will pop old pMethods pointer and stuff it
        //  back in this.pMethods after a methodWithSuper methods opcode
        // for delete, which is method 0, compile an rdrop instead, since we don't
        //   want to save the old pMethods into memory which we have just freed
        long unSuperOp = gCompiledOps[(mOffset == 0) ? OP_RDROP : OP_UNSUPER];
        SPEW_STRUCTS(" unsuperOp 0x%x", unSuperOp);
        *mpDst++ = unSuperOp;
    }
	return success;
}
	
