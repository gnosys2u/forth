#pragma once
//////////////////////////////////////////////////////////////////////
//
// NativeType.h: support for native types
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"

class NativeType
{
public:
    NativeType( const char* pName, int numBytes, BaseType nativeType );
    virtual ~NativeType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal, int32_t flags=0 );

    inline int32_t GetGlobalOp( void ) { return (int32_t)mBaseType + gCompiledOps[OP_DO_BYTE]; };
    inline int32_t GetGlobalArrayOp( void ) { return (int32_t)mBaseType + gCompiledOps[OP_DO_BYTE_ARRAY]; };
    inline int32_t GetLocalOp( void ) { return (int32_t)mBaseType + kOpLocalByte; };
    inline int32_t GetFieldOp( void ) { return (int32_t)mBaseType + kOpFieldByte; };
    inline int32_t GetAlignment( void ) { return (mNumBytes > 4) ? 4 : mNumBytes; };
    inline int32_t GetSize( void ) { return mNumBytes; };
    inline const char* GetName( void ) { return mpName; };
    inline BaseType GetBaseType( void ) { return mBaseType; };

protected:
    const char*         mpName;
    int                 mNumBytes;
    BaseType       mBaseType;
};

extern NativeType gNativeTypeByte, gNativeTypeUByte, gNativeTypeShort, gNativeTypeUShort,
		gNativeTypeInt, gNativeTypeUInt, gNativeTypeLong, gNativeTypeULong, gNativeTypeFloat,
        gNativeTypeDouble, gNativeTypeString, gNativeTypeOp, gNativeTypeObject;

extern NativeType* gpNativeTypes[];
