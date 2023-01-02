#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthBuiltinClasses.h: builtin classes
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "ForthForgettable.h"
#include "ForthObject.h"

class ForthClassVocabulary;

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
    kBCIShellStack,
	kNumBuiltinClasses		// must be last
} eBuiltinClassIndex;

// ForthShowAlreadyShownObject returns true if object was already shown (or null), does display for those cases
bool ForthShowAlreadyShownObject(ForthObject obj, ForthCoreState* pCore, bool addIfUnshown);
void ForthShowObject(ForthObject& obj, ForthCoreState* pCore);

extern "C"
{
    extern FORTHOP(unimplementedMethodOp);
    extern FORTHOP(illegalMethodOp);
};

#define EXIT_IF_OBJECT_ALREADY_SHOWN if (ForthShowAlreadyShownObject(GET_TP, pCore, true)) { METHOD_RETURN; return; }

void unrefObject(ForthObject& fobj);

#define GET_CLASS_VOCABULARY(BCI_INDEX) ForthTypesManager::GetInstance()->GetClassVocabulary(BCI_INDEX)
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

class ForthForgettableGlobalObject : public ForthForgettable
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
