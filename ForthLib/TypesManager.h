#pragma once
//////////////////////////////////////////////////////////////////////
//
// TypesManager.h: manager of structs and classes
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

#include "Forgettable.h"

class StructCodeGenerator;
class StructVocabulary;
class ClassVocabulary;
class ForthInterface;

// a struct accessor compound operation must be less than this length in longs
constexpr auto MAX_ACCESSOR_LONGS = 64;

class TypesManager : public Forgettable
{
public:
    TypesManager();
    ~TypesManager();

    virtual void    ForgetCleanup( void *pForgetLimit, forthop op );

    // compile/interpret symbol if it is a valid structure accessor
    virtual bool    ProcessSymbol( ParseInfo *pInfo, OpResult& exitStatus );

    // compile symbol if it is a class member variable or method
    virtual bool    ProcessMemberSymbol( ParseInfo *pInfo, OpResult& exitStatus, VarOperation varop = VarOperation::varDefaultOp );

    void            AddBuiltinClasses(OuterInterpreter* pOuter);
    void            ShutdownBuiltinClasses(Engine* pEngine);

    // add a new structure type
    StructVocabulary*          StartStructDefinition( const char *pName );
    void                            EndStructDefinition( void );
	// default classIndex value means assign next available classIndex
	ClassVocabulary*           StartClassDefinition(const char *pName, int classIndex = kNumBuiltinClasses, bool isInterface = false);
    void                            EndClassDefinition( void );
    static TypesManager*       GetInstance( void );

    // return info structure for struct type specified by typeIndex
    ForthTypeInfo*        GetTypeInfo( int typeIndex );
	ClassVocabulary* GetClassVocabulary(int typeIndex) const;
	ForthInterface* GetClassInterface(int typeIndex, int interfaceIndex) const;

    // return vocabulary for a struct type given its opcode or name
    StructVocabulary*  GetStructVocabulary( forthop op );
	StructVocabulary*	GetStructVocabulary( const char* pName );

    void GetFieldInfo( int32_t fieldType, int32_t& fieldBytes, int32_t& alignment );

    StructVocabulary*  GetNewestStruct( void );
    ClassVocabulary*   GetNewestClass( void );
    BaseType                GetBaseTypeFromName( const char* typeName );
    NativeType*        GetNativeTypeFromName( const char* typeName );
    int32_t                 GetBaseTypeSizeFromName( const char* typeName );
    forthop*                GetClassMethods();

    virtual const char* GetTypeName();
    virtual const char* GetName();

	inline const std::vector<ForthFieldInitInfo>&	GetFieldInitInfos() {  return mFieldInitInfos;  }
	void AddFieldInitInfo(const ForthFieldInitInfo& fieldInitInfo);
	void					DefineInitOpcode();

protected:
    // mStructInfo is an array with an entry for each defined structure type
	std::vector<ForthTypeInfo>      mStructInfo;
    static TypesManager*       mpInstance;
    Vocabulary*                mpSavedDefinitionVocab;
    char                            mToken[ DEFAULT_INPUT_BUFFER_LEN ];
    forthop                         mCode[ MAX_ACCESSOR_LONGS ];
    forthop*                        mpClassMethods;
	StructCodeGenerator*		mpCodeGenerator;
	std::vector<ForthFieldInitInfo>	mFieldInitInfos;
	cell							mNewestTypeIndex;
};

