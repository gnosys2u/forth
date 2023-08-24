//////////////////////////////////////////////////////////////////////
//
// NativeType.cpp: support for native types
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Engine.h"
#include "NativeType.h"
#include "OuterInterpreter.h"
#include "TypesManager.h"
#include "LocalVocabulary.h"
#include "VocabularyStack.h"

NativeType gNativeTypeByte( "byte", 1, BaseType::kByte );

NativeType gNativeTypeUByte( "ubyte", 1, BaseType::kUByte );

NativeType gNativeTypeShort( "short", 2, BaseType::kShort );

NativeType gNativeTypeUShort( "ushort", 2, BaseType::kUShort );

NativeType gNativeTypeInt( "int", 4, BaseType::kInt );

NativeType gNativeTypeUInt( "uint", 4, BaseType::kUInt );

NativeType gNativeTypeLong( "long", 8, BaseType::kLong );

NativeType gNativeTypeULong( "ulong", 8, BaseType::kULong );

NativeType gNativeTypeFloat( "sfloat", 4, BaseType::kFloat );

NativeType gNativeTypeDouble( "dfloat", 8, BaseType::kDouble );

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

void NativeType::DefineInstance( Engine *pEngine, void *pInitialVal, int32_t flags )
{
    OuterInterpreter* pOuter = pEngine->GetOuterInterpreter();
    char *pToken = pOuter->GetNextSimpleToken();
    int nBytes = mNumBytes;
    char *pHere;
    int i;
    int32_t val = 0;
    BaseType baseType = mBaseType;
    Vocabulary *pVocab;
    forthop* pEntry;
    int32_t typeCode, len, varOffset, storageLen;
    char *pStr;
    CoreState *pCore = pEngine->GetCoreState();        // so we can SPOP maxbytes
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
                SET_ERROR( ForthError::missingSize );
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
                    pOuter->CompileOpcode(pEntry[0] | ((forthop) VarOperation::varSet) << 20);
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
                    SET_VAR_OPERATION(VarOperation::varSet);
                }

                if (GET_VAR_OPERATION == VarOperation::varSet )
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
                    pOuter->CompileOpcode(pEntry[0] | ((forthop)VarOperation::varSet) << 20);
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
                    SET_VAR_OPERATION(VarOperation::varSet);
                }

                if (GET_VAR_OPERATION == VarOperation::varSet)
                {
                    // var definition was preceeded by "->", so initialize var
                    pEngine->ExecuteOp(pCore,  pEntry[0] );
                }
            }
        }
    }
}
