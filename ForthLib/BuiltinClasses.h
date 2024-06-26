#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
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

#include <vector>

#include "Forgettable.h"
#include "Object.h"

class ClassVocabulary;

// structtype indices for builtin classes
typedef enum
{
	kBCIInvalid,
    kBCIContainedType,
    kBCIObject,
    kBCIClass,
    kBCIInterface,
	kBCIIter,
	kBCIIterable,
	kBCIArray,
	kBCIArrayIter,
	kBCIList,
	kBCIListIter,
	kBCIMap,
	kBCIMapIter,
	kBCIIntMap,
	kBCIIntMapIter,
	kBCIFloatMap,
	kBCIFloatMapIter,
	kBCILongMap,
	kBCILongMapIter,
	kBCIDoubleMap,
	kBCIDoubleMapIter,
	kBCIStringIntMap,
	kBCIStringIntMapIter,
	kBCIStringFloatMap,
	kBCIStringFloatMapIter,
	kBCIStringLongMap,
	kBCIStringLongMapIter,
	kBCIStringDoubleMap,
	kBCIStringDoubleMapIter,
	kBCIString,
	kBCIStringMap,
	kBCIStringMapIter,
	kBCIPair,
	kBCIPairIter,
	kBCITriple,
	kBCITripleIter,
	kBCIByteArray,
	kBCIByteArrayIter,
	kBCIShortArray,
	kBCIShortArrayIter,
	kBCIIntArray,
	kBCIIntArrayIter,
	kBCIFloatArray,
	kBCIFloatArrayIter,
	kBCIDoubleArray,
	kBCIDoubleArrayIter,
	kBCILongArray,
	kBCILongArrayIter,
    kBCIStructArray,
    kBCIStructArrayIter,
    kBCIDeque,
    //kBCIDequeIter,
    kBCIInt,
	kBCILong,
	kBCIFloat,
	kBCIDouble,
	kBCIThread,
	kBCIFiber,
	kBCIAsyncLock,
	kBCILock,
    kBCIAsyncSemaphore,
    kBCISemaphore,
    kBCISystem,
	kBCIVocabulary,
	kBCIVocabularyIter,
	kBCIInStream,
	kBCIFileInStream,
	kBCIConsoleInStream,
	kBCIOutStream,
	kBCIFileOutStream,
	kBCIStringOutStream,
	kBCIConsoleOutStream,
	kBCIErrorOutStream,
	kBCIFunctionOutStream,
	kBCITraceOutStream,
    kBCISplitOutStream,
    kBCIBlockFile,
    kBCIBag,
    kBCIBagIter,
    kBCISocket,
    kBCIControlStack,
    kBCISearchStack,
	kNumBuiltinClasses		// must be last
} eBuiltinClassIndex;

// ForthShowAlreadyShownObject returns true if object was already shown (or null), does display for those cases
bool ForthShowAlreadyShownObject(ForthObject obj, CoreState* pCore, bool addIfUnshown);
void ForthShowObject(ForthObject& obj, CoreState* pCore);

extern "C"
{
    extern FORTHOP(unimplementedMethodOp);
    extern FORTHOP(illegalMethodOp);
};

#define EXIT_IF_OBJECT_ALREADY_SHOWN if (ForthShowAlreadyShownObject(GET_TP, pCore, true)) { METHOD_RETURN; return; }

void unrefObject(ForthObject& fobj);

#define GET_CLASS_VOCABULARY(BCI_INDEX) TypesManager::GetInstance()->GetClassVocabulary(BCI_INDEX)
#define GET_BUILTIN_INTERFACE(BCI_INDEX, INTERFACE_INDEX) GET_CLASS_VOCABULARY(BCI_INDEX)->GetInterface(INTERFACE_INDEX)
#ifdef FORTH64
#define GET_CLASS_OBJECT(OBJ) *((ForthClassObject **)(((OBJ)->pMethods) - 2))
#else
#define GET_CLASS_OBJECT(OBJ) ((ForthClassObject *)(*(((OBJ)->pMethods) - 1)))
#endif
// oOutStream is an abstract output stream class

struct OutStreamFuncs
{
	streamCharOutRoutine		outChar;
	streamBytesOutRoutine		outBytes;
	streamStringOutRoutine		outString;
};


struct oOutStreamStruct
{
    forthop*            pMethods;
	REFCOUNTER          refCount;
	void*               pUserData;
	OutStreamFuncs*     pOutFuncs;
	char				eolChars[4];
};

struct oStringOutStreamStruct
{
    oOutStreamStruct		ostream;
    ForthObject				outString;
};

struct InStreamFuncs
{
    streamCharInRoutine		    inChar;
    streamBytesInRoutine		inBytes;
    streamLineInRoutine		    inLine;
    streamStringInRoutine		inString;
};


enum
{
	kOutStreamPutCharMethod = kNumBaseMethods,
	kOutStreamPutBytesMethod = kNumBaseMethods + 1,
    kOutStreamPutStringMethod = kNumBaseMethods + 2,
    kOutStreamPutLineMethod = kNumBaseMethods + 3,
    kInStreamGetCharMethod = kNumBaseMethods,
	kInStreamGetBytesMethod = kNumBaseMethods + 1,
	kInStreamGetLineMethod = kNumBaseMethods + 2,
	kInStreamGetStringMethod = kNumBaseMethods + 3,
	kInStreamAtEOFMethod = kNumBaseMethods + 4
};

struct oInStreamStruct
{
    forthop*            pMethods;
	REFCOUNTER          refCount;
	void*               pUserData;
	int					bTrimEOL;
    InStreamFuncs*      pInFuncs;
};

typedef std::vector<ForthObject> oArray;
struct oArrayStruct
{
    forthop*			pMethods;
	REFCOUNTER          refCount;
	oArray*				elements;
};

struct oListElement
{
	oListElement*	prev;
	oListElement*	next;
	ForthObject		obj;
};

struct oListStruct
{
    forthop*			pMethods;
	REFCOUNTER          refCount;
	oListElement*		head;
	oListElement*		tail;
};

struct oArrayIterStruct
{
    forthop*			pMethods;
	REFCOUNTER          refCount;
	ForthObject			parent;
	ucell				cursor;
};

#define DEFAULT_STRING_DATA_BYTES 32

struct oString
{
	int32_t		maxLen;
	int32_t		curLen;
	char		data[DEFAULT_STRING_DATA_BYTES];
};

struct oStringStruct
{
    forthop*			pMethods;
	REFCOUNTER          refCount;
	ucell				hash;
	oString*			str;
};

typedef std::map<int64_t, ForthObject> oLongMap;
struct oLongMapStruct
{
    forthop*			pMethods;
	REFCOUNTER          refCount;
    oLongMap*			elements;
};

struct oLongMapIterStruct
{
    forthop*            pMethods;
	REFCOUNTER          refCount;
	ForthObject			parent;
    oLongMap::iterator*	cursor;
};

class ForthForgettableGlobalObject : public Forgettable
{
public:
    ForthForgettableGlobalObject( const char* pName, void *pOpAddress, forthop op, int numElements = 1 );
    virtual ~ForthForgettableGlobalObject();

    virtual const char* GetTypeName();
    virtual const char* GetName();
protected:
    char* mpName;
    virtual void    ForgetCleanup( void *pForgetLimit, forthop op );

	int		mNumElements;
};
