#pragma once
//////////////////////////////////////////////////////////////////////
//
// NativeType.h: support for native types
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

#include "Forth.h"

class NativeType
{
public:
    NativeType( const char* pName, int numBytes, BaseType nativeType );
    virtual ~NativeType();
    virtual void DefineInstance( Engine *pEngine, void *pInitialVal, int32_t flags=0 );

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
