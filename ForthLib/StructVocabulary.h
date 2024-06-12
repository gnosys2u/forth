#pragma once
//////////////////////////////////////////////////////////////////////
//
// StructVocabulary.h: support for user-defined structures
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

#include "Vocabulary.h"
#include "BuiltinClasses.h"
#include "StructCodeGenerator.h"
#include <vector>

class Engine;
class StructVocabulary;
class ClassVocabulary;
class NativeType;
class TypesManager;
class StructCodeGenerator;
class ObjectReader;

// each new structure type definition is assigned a unique index
// the struct type index is:
// - recorded in the type info field of vocabulary entries for global struct instances
// - recorded in the type info field of struct vocabulary entries for members of this type
// - used to get from a struct field to the struct vocabulary which defines its subfields

struct ForthTypeInfo
{
	ForthTypeInfo()
		: pVocab(NULL)
		, op(OP_ABORT)
		, typeIndex(static_cast<int>(kBCIInvalid))
	{}

	ForthTypeInfo(StructVocabulary* inVocab, forthop inOp, int inTypeIndex)
		: pVocab(inVocab)
		, op(inOp)
		, typeIndex(inTypeIndex)
	{}

    StructVocabulary*  pVocab;
    forthop                 op;
	int                     typeIndex;
};

typedef struct
{
    forthop*                    pMethods;
    REFCOUNTER                  refCount;
	ClassVocabulary*       pVocab;
    forthop                     newOp;
} ForthClassObject;

typedef bool(*CustomObjectReader)(const std::string& elementName, ObjectReader* reader);

///////////////////////////////////////

// these are the types of struct/object fields which need to be set by struct init ops
enum eForthStructInitType
{
	kFSITSuper,
	kFSITString,
	kFSITStruct,
	kFSITStringArray,
	kFSITStructArray,
};

typedef struct
{
	eForthStructInitType fieldType;
	int32_t offset;
	int32_t len;
	int32_t typeIndex;
	int32_t numElements;
} ForthFieldInitInfo;

class StructVocabulary : public Vocabulary
{
public:
    StructVocabulary( const char* pName, int typeIndex );
    virtual ~StructVocabulary();

    // return pointer to symbol entry, NULL if not found
    virtual forthop*    FindSymbol( const char *pSymName, ucell serial=0 );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( forthop op );

    virtual const char* GetTypeName();

    virtual const char* GetDescription( void );

    virtual void        PrintEntry(forthop*   pEntry);
    static void         TypecodeToString( int32_t typeCode, char* outBuff, size_t outBuffSize );

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance( void );

    void                AddField( const char* pName, int32_t fieldType, int numElements );
    int                 GetAlignment( void );
    int                 GetSize( void );
    void                StartUnion( void );
    virtual void        Extends( StructVocabulary *pParentStruct );

    inline StructVocabulary* BaseVocabulary( void ) { return mpSearchNext; }

    inline int32_t         GetTypeIndex( void ) { return mTypeIndex; };

    virtual void        EndDefinition();

    virtual void		ShowData(const void* pData, CoreState* pCore, bool showId);
    // returns number of top-level data items shown
    // pass optional pEndVocab to prevent showing items from that vocab or lower
    virtual int		    ShowDataInner(const void* pData, CoreState* pCore,
        StructVocabulary* pEndVocab = nullptr);

	inline forthop			GetInitOpcode() { return mInitOpcode;  }
	void				SetInitOpcode(forthop op);

protected:
    int                     mNumBytes;
    int                     mMaxNumBytes;
    int                     mTypeIndex;
    int                     mAlignment;
    StructVocabulary   *mpSearchNext;
	forthop					mInitOpcode;
};

