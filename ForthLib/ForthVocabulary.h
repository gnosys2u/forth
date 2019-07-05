#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.h: interface for the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthForgettable.h"
#include "ForthObject.h"

class ForthParseInfo;
class ForthEngine;

// default initial vocab size in bytes
#define DEFAULT_VOCAB_STORAGE 512

// by default, the value field of a vocabulary entry is 2 longwords
//   but this can be overridden in the constructor
#define DEFAULT_VALUE_FIELD_LONGS 2

// maximum length of a symbol in longwords
#define SYM_MAX_LONGS 64

class ForthVocabulary;

// vocabulary object defs
struct oVocabularyStruct
{
    long*               pMethods;
    ulong				refCount;
	ForthVocabulary*	vocabulary;
};

struct oVocabularyIterStruct
{
    long*               pMethods;
    ulong				refCount;
	ForthObject			parent;
	long*				cursor;
	ForthVocabulary*	vocabulary;
};

class ForthVocabulary : public ForthForgettable
{
public:
    ForthVocabulary( const char *pName = NULL,
                     int valueLongs = DEFAULT_VALUE_FIELD_LONGS,
                     int storageBytes = DEFAULT_VOCAB_STORAGE,
                     void* pForgetLimit = NULL,
                     long op = 0 );
    virtual ~ForthVocabulary();

    virtual void        ForgetCleanup( void *pForgetLimit, long op );

    virtual void        DoOp( ForthCoreState *pCore );

    void                SetName( const char *pVocabName );
    virtual const char *GetName( void );
    virtual const char *GetTypeName( void );

    void                Empty( void );

    // add symbol to symbol table, return ptr to new symbol entry
    virtual long*       AddSymbol( const char *pSymName, long symType, long symValue, bool addToEngineOps );

    // copy a symbol table entry, presumably from another vocabulary
    void                CopyEntry( long *pEntry );

    // delete single symbol table entry
    void                DeleteEntry( long *pEntry );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );

    // the FindSymbol* methods take a serial number which is used to avoid doing
    //  redundant searches, since a vocabulary can appear multiple times in the
    //  vocabulary stack - the vocabulary stack keeps a serial which it increments
    //  before each search.  If the serial passed to FindSymbol is the same as the
    //  serial of the last failed search, FindSymbol immediately returns NULL.
    // The default serial value of 0 will force FindSymbol to do the search

    // return pointer to symbol entry, NULL if not found
    virtual long *      FindSymbol( const char *pSymName, ulong serial=0 );
	// continue searching a vocabulary 
    virtual long *      FindNextSymbol( const char *pSymName, long* pStartEntry, ulong serial=0 );
    // return pointer to symbol entry, NULL if not found, given its value
    virtual long *      FindSymbolByValue( long val, ulong serial=0 );
    // return pointer to symbol entry, NULL if not found, given its value
    virtual long *      FindNextSymbolByValue( long val, long* pStartEntry, ulong serial=0 );

    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    virtual long *      FindSymbol( ForthParseInfo *pInfo, ulong serial=0 );
	// continue searching a vocabulary 
    virtual long *      FindNextSymbol( ForthParseInfo *pInfo, long* pStartEntry, ulong serial=0 );

    // compile/interpret entry returned by FindSymbol
    virtual eForthResult ProcessEntry( long *pEntry );

    // return a string telling the type of library
    virtual const char* GetType( void );

    virtual void        PrintEntry( long*   pEntry );

    // the symbol for the word which is currently under construction is "smudged"
    // so that if you try to reference that symbol in its own definition, the match
    // will fail, and an earlier symbol with the same name will be compiled instead
    // this is to allow you to re-define a symbol by layering on top of an earlier
    // symbol of the same name
    void                SmudgeNewestSymbol( void );
    // unsmudge a symbol when its definition is complete
    void                UnSmudgeNewestSymbol( void );

	ForthObject& GetVocabularyObject(void);

	inline long *       GetFirstEntry(void) {
        return mpStorageBottom;
    };

    inline int          GetNumEntries( void )
    {
        return mNumSymbols;
    };

    // find next entry in vocabulary
    // invoker is responsible for not going past end of vocabulary
    inline long *       NextEntry( long *pEntry ) {
        long *pTmp = pEntry + mValueLongs;
        // add in 1 for length, and 3 for longword rounding
        return (pTmp + ( (( ((char *) pTmp)[0] ) + 4) >> 2));
    };

	// return next entry in vocabulary, return NULL if this is last entry
	inline long *       NextEntrySafe(long *pEntry) {
		long *pTmp = pEntry + mValueLongs;
		// add in 1 for length, and 3 for longword rounding
		pTmp += (((((char *)pTmp)[0]) + 4) >> 2);
		return (pTmp < mpStorageTop) ? pTmp : NULL;
	};

	static inline forthOpType   GetEntryType(const long *pEntry) {
        return FORTH_OP_TYPE( *pEntry );

    };

    static inline void          SetEntryType( long *pEntry, forthOpType opType ) {
        *pEntry = COMPILED_OP( opType, FORTH_OP_VALUE( *pEntry ) );
    };

    static inline long          GetEntryValue( const long *pEntry ) {
        return FORTH_OP_VALUE( *pEntry );
    };

	inline long *               GetNewestEntry(void)
	{
		return mpNewestSymbol;
	};

	inline long *               GetEntriesEnd(void)
	{
		return mpStorageTop;
	};

	inline char *               GetEntryName(const long *pEntry) {
        return ((char *) pEntry) + (mValueLongs << 2) + 1;
    };

    inline int                  GetEntryNameLength( const long *pEntry ) {
        return (int) *(((char *) pEntry) + (mValueLongs << 2));
    };

    inline int                  GetValueLength()
    {
        return mValueLongs << 2;
    }

    // returns number of chars in name
    virtual int                 GetEntryName( const long *pEntry, char *pDstBuff, int buffSize );

    static ForthVocabulary *FindVocabulary( const char* pName );

    static inline ForthVocabulary *GetVocabularyChainHead( void ) {
        return mpChainHead;
    };

    inline ForthVocabulary *GetNextChainVocabulary( void ) {
        return mpChainNext;
    };

    inline char *NewestSymbol( void ) {
        return &(mNewestSymbol[0]);
    };

	virtual bool IsStruct();
	virtual bool IsClass();

    virtual void AfterStart();
    virtual int Save( FILE* pOutFile );
    virtual bool Restore( const char* pBuffer, unsigned int numBytes );

#ifdef MAP_LOOKUP
    void                        InitLookupMap();
#endif

protected:

    static ForthVocabulary *mpChainHead;
    ForthEngine         *mpEngine;
    ForthVocabulary     *mpChainNext;
    int                 mNumSymbols;
    int                 mStorageLongs;
    long                *mpStorageBase;
    long                *mpStorageTop;
    long                *mpStorageBottom;
    long                *mpNewestSymbol;
    char                *mpName;
    int                 mValueLongs;
    ulong               mLastSerial;
    char                mNewestSymbol[ 256 ];
    // these are set right after forth is started, before any user definitions are loaded
    // they are used when saving/restoring vocabularies
    int                 mStartNumSymbols;
    int                 mStartStorageLongs;
	oVocabularyStruct	mVocabStruct;
	ForthObject			mVocabObject;
#ifdef MAP_LOOKUP
    CMapStringToPtr     mLookupMap;
#endif
};

#define MAX_LOCAL_DEPTH 16
#define LOCAL_STACK_STRIDE 3
class ForthLocalVocabulary : public ForthVocabulary
{
public:
    ForthLocalVocabulary( const char *pName=NULL,
                               int valueLongs=NUM_LOCALS_VOCAB_VALUE_LONGS, int storageBytes=DEFAULT_VOCAB_STORAGE );
    virtual ~ForthLocalVocabulary();

    // return a string telling the type of library
    virtual const char* GetType( void );

	void				Push();
	void				Pop();

	int					GetFrameLongs();
	long*				GetFrameAllocOpPointer();
	long*				AddVariable( const char* pVarName, long fieldType, long varValue, int nLongs );
	void				ClearFrame();

protected:
	int					mDepth;
	int					mStack[ MAX_LOCAL_DEPTH * LOCAL_STACK_STRIDE ];
	long*				mpAllocOp;
	int					mFrameLongs;
};

class ForthDLLVocabulary : public ForthVocabulary
{
public:
    ForthDLLVocabulary( const char *pVocabName,
                        const char *pDLLName,
                        int valueLongs = DEFAULT_VALUE_FIELD_LONGS,
                        int storageBytes = DEFAULT_VOCAB_STORAGE,
                        void* pForgetLimit = NULL,
                        long op = 0 );
    virtual ~ForthDLLVocabulary();

    // return a string telling the type of library
    virtual const char* GetType( void );

    long                LoadDLL( void );
    void                UnloadDLL( void );
	long *              AddEntry(const char* pFuncName, const char* pEntryName, long numArgs);
	void				SetFlag( unsigned long flag );
protected:
    char *              mpDLLName;
	unsigned long		mDLLFlags;
#ifdef WIN32
    HINSTANCE           mhDLL;
#endif
#if defined(LINUX) || defined(MACOSX)
    void*				mLibHandle;
#endif
};

class ForthVocabularyStack
{
public:
    ForthVocabularyStack( int maxDepth=256 );
    ~ForthVocabularyStack();
    void                Initialize( void );
    void                DupTop( void );
    bool                DropTop( void );
    ForthVocabulary*    GetTop( void );
    void                SetTop( ForthVocabulary* pVocab );
    void                Clear();
    // GetElement(0) is the same as GetTop
    ForthVocabulary*    GetElement( int depth );
	inline int			GetDepth() { return mTop + 1; }

    // return pointer to symbol entry, NULL if not found
    // ppFoundVocab will be set to the vocabulary the symbol was actually found in
    // set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
    long *      FindSymbol( const char *pSymName, ForthVocabulary** ppFoundVocab=NULL );

    // return pointer to symbol entry, NULL if not found, given its value
    long *      FindSymbolByValue( long val, ForthVocabulary** ppFoundVocab=NULL );

    // return pointer to symbol entry, NULL if not found
    // pSymName is required to be a longword aligned address, and to be padded with 0's
    // to the next longword boundary
    long *      FindSymbol( ForthParseInfo *pInfo, ForthVocabulary** ppFoundVocab=NULL );

private:
    ForthVocabulary**   mStack;
    ForthEngine*        mpEngine;
    int                 mMaxDepth;
    int                 mTop;
    ulong               mSerial;
};

namespace OVocabulary
{
	void AddClasses(ForthEngine* pEngine);
}
