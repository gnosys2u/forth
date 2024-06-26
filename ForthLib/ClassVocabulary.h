#pragma once
//////////////////////////////////////////////////////////////////////
//
// ClassVocabulary.h: support for user-defined classes
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the �Software�), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#include "Vocabulary.h"
#include "StructVocabulary.h"

class ClassVocabulary;

class ForthInterface
{
public:
    ForthInterface(ClassVocabulary* pDefiningClass = NULL);
    virtual ~ForthInterface();

    void					Copy(ForthInterface* pInterface, bool isPrimaryInterface);
    void					Implements(ClassVocabulary* pClass);
    ClassVocabulary* GetDefiningClass();
    forthop* GetMethods();
    forthop					GetMethod(int index);
    void					SetMethod(int index, forthop method);
    int					    AddMethod(forthop method);
    int                     GetMethodIndex(const char* pName);
    int					    GetNumMethods();
    int					    GetNumAbstractMethods();
protected:
    ClassVocabulary* mpDefiningClass;
    std::vector<forthop>    mMethods;
    int                     mNumAbstractMethods;
};


class ClassVocabulary : public StructVocabulary
{
public:
    ClassVocabulary( const char* pName, int typeIndex );
    virtual ~ClassVocabulary();

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance(void);
    virtual void	    DefineInstance(char* pInstanceName, const char* pContainedClassName = nullptr);

    virtual const char* GetTypeName();

	int                 AddMethod( const char* pName, int methodIndex, forthop op );
	int 				FindMethod( const char* pName );
	void				Implements( const char* pName );
	void				EndImplements( void );
	int32_t				GetClassId( void )		{ return mTypeIndex; }

	ForthInterface*		GetInterface( int32_t index );
    ForthInterface*     GetCurrentInterface();

    forthop*            GetMethods();
    int32_t                FindInterfaceIndex( int32_t classId );
	int32_t				GetNumInterfaces( void );
    virtual void        Extends( ClassVocabulary *pParentClass );
    ForthClassObject*   GetClassObject(void);
    void                FixClassObjectMethods(void);
    ClassVocabulary* ParentClass( void );

    virtual void        PrintEntry(forthop*   pEntry);
    void                SetCustomObjectReader(CustomObjectReader reader);
    CustomObjectReader  GetCustomObjectReader();

protected:
    int32_t                        mCurrentInterface;
	ClassVocabulary*       mpParentClass;
	std::vector<ForthInterface *>	mInterfaces;
    ForthClassObject*           mpClassObject;
    CustomObjectReader          mCustomReader;
	static ClassVocabulary* smpObjectClass;
};

class InterfaceVocabulary : public ClassVocabulary
{
public:
    InterfaceVocabulary(const char* pName, int typeIndex);
};

