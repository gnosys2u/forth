//////////////////////////////////////////////////////////////////////
//
// ForthVocabulary.cpp: implementation of the ForthVocabulary class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ForthVocabulary.h"
#include "ForthEngine.h"
#include "ForthShell.h"
#include "ForthForgettable.h"
#include "ForthParseInfo.h"
#include "ForthBuiltinClasses.h"
#if defined(LINUX) || defined(MACOSX)
#include <dlfcn.h>
#endif

//############################################################################
//
//   vocabulary/symbol manipulation
//
//############################################################################


// symbol entry layout:
// offset   contents
//  0..3    4-byte symbol value - high byte is symbol type - usually this is opcode for symbol
//  ...     0 or more extra symbol value fields
//  N       1-byte symbol length (not including padding)
//  N+1     symbol characters
//
//  The total symbol entry length is always a multiple of 4, padded with 0s
//  if necessary.  This allows faster dictionary searches.

// vocabulary symbol storage grows downward, and is searched from the
// bottom upward (try to match newest symbols first)

// this many longs are added to vocab storage whenever it runs out of room
#define VOCAB_EXPANSION_INCREMENT   128

// this is head of a chain which links all vocabs
ForthVocabulary *ForthVocabulary::mpChainHead = NULL;

//////////////////////////////////////////////////////////////////////
////
///     ForthVocabulary
//
//

ForthVocabulary::ForthVocabulary( const char    *pName,
                                  int           valueLongs,
                                  int           storageBytes,
                                  void*         pForgetLimit,
                                  forthop       op )
: ForthForgettable( pForgetLimit, op )
, mpName( NULL )
, mValueLongs( valueLongs )
, mLastSerial( 0 )
{
    mStorageLongs = ((storageBytes + 3) & ~3) >> 2;
    mpStorageBase = new forthop[mStorageLongs];

    // add this vocab to linked list of all vocabs
    mpChainNext = mpChainHead;
    mpChainHead = this;

    if ( pName != NULL )
    {
        SetName( pName );
    }
    mpEngine = ForthEngine::GetInstance();
    Empty();

	mVocabStruct.refCount = 10000;
	mVocabStruct.vocabulary = this;
	// initialize vocab object when it is first requested to avoid an order-of-creation problem with OVocabulary class object
	mVocabObject = nullptr;
}

ForthVocabulary::~ForthVocabulary()
{
    delete [] mpStorageBase;

    ForthVocabulary **ppNext = &mpChainHead;
    while ( ppNext != NULL )
    {
        if ( *ppNext == this )
        {
            *ppNext = mpChainNext;
            break;
        }
        ppNext = &((*ppNext)->mpChainNext);
    }

    delete [] mpName;
}


void
ForthVocabulary::ForgetCleanup( void *pForgetLimit, forthop op )
{
   // this is invoked from the ForthForgettable chain to propagate a "forget"
   ForgetOp( op );
}


void
ForthVocabulary::SetName( const char *pVocabName )
{
    if ( pVocabName != NULL )
    {
        int len = strlen( pVocabName ) + 1;
		if (mpName != NULL)
		{
			delete [] mpName;
		}
        mpName = new char[len];
        strcpy( mpName, pVocabName );
    }
}


const char *
ForthVocabulary::GetName( void )
{
    return (mpName == NULL) ? "Unknown" : mpName;
}

const char *
ForthVocabulary::GetTypeName( void )
{
    return "vocabulary";
}

int ForthVocabulary::GetEntryName( const forthop* pEntry, char *pDstBuff, int buffSize )
{
    int symLen = GetEntryNameLength( pEntry );
    int len = (symLen < buffSize) ? symLen : buffSize - 1;

    memcpy( pDstBuff, (void *) GetEntryName( pEntry ), len );
    pDstBuff[len] = '\0';
    return len;
}

void
ForthVocabulary::Empty( void )
{
    mNumSymbols = 0;
    mpStorageTop = mpStorageBase + mStorageLongs;
    mpStorageBottom = mpStorageTop;
    mpNewestSymbol = NULL;
}


#ifdef MAP_LOOKUP
void
ForthVocabulary::InitLookupMap( void )
{
    mLookupMap.RemoveAll();
    forthop* pEntry = mpStorageBottom;
    char buff[ 256 ];
    for ( int i = 0; i < mNumSymbols; i++ )
    {
        GetEntryName( pEntry, buff, sizeof(buff) );
        mLookupMap.SetAt( buff, pEntry );
        pEntry = NextEntry( pEntry );
    }
}
#endif

forthop* ForthVocabulary::AddSymbol( const char *pSymName, forthop symValue)
{
    char *pVC;
    forthop *pBase, *pSrc, *pDst;
    int i, nameLen, symSize, newLen;

    nameLen = (pSymName == NULL) ? 0 : strlen( pSymName );
#define SYMBOL_LEN_MAX 255
    if ( nameLen > SYMBOL_LEN_MAX )
    {
        nameLen = SYMBOL_LEN_MAX;
    }
    else
    {
        strcpy( mNewestSymbol, pSymName );
    }

    symSize = mValueLongs + ( ((nameLen + 4) & ~3) >> 2 );
    pBase = mpStorageBottom - symSize;
    if ( pBase < mpStorageBase )
    {
        //
        // new symbol wont fit, increase storage size
        //
        newLen = mStorageLongs + VOCAB_EXPANSION_INCREMENT;
        SPEW_VOCABULARY( "Increasing %s vocabulary size to %d longs\n", GetName(), newLen );
        pBase = new forthop[newLen];
        pSrc = mpStorageBase + mStorageLongs;
        pDst = pBase + newLen;
        // copy existing entries into new storage
        while ( pSrc > mpStorageBottom )
        {
            *--pDst = *--pSrc;
        }

        delete [] mpStorageBase;
        mpStorageBottom = pDst;
        mpStorageBase = pBase;
        mStorageLongs = newLen;
        pBase = mpStorageBottom - symSize;
		mpStorageTop = mpStorageBase + mStorageLongs;
#ifdef MAP_LOOKUP
        InitLookupMap();
#endif
    }

    SPEW_VOCABULARY( "Adding symbol %s value 0x%x to %s\n",
        pSymName, symValue, GetName() );

    mpStorageBottom = pBase;
#ifdef MAP_LOOKUP
    mLookupMap.SetAt( pSymName, mpStorageBottom );
#endif
    mpNewestSymbol = mpStorageBottom;
    // TBD: check for storage overflow
    *pBase++ = symValue;
    for ( i = 1; i < mValueLongs; i++ )
    {
        *pBase++ = 0;
    }
    pVC = (char *) pBase;
    *pVC++ = nameLen;
    for ( i = 0; i < nameLen; i++ )
    {
        *pVC++ = *pSymName++;
    }
    // pad with 0s to make the total symbol entry a multiple of longwords int32_t
    nameLen++;
    while ( (nameLen & 3) != 0 )
    {
        *pVC++ = 0;
        nameLen++;
    }

    ASSERT( (((uint32_t) pVC) & 3) == 0 );
    mNumSymbols++;
    
    return mpStorageBottom;
}


// copy a symbol table entry, presumably from another vocabulary
void
ForthVocabulary::CopyEntry(forthop* pEntry )
{
    int numLongs;

    numLongs = NextEntry( pEntry ) - pEntry;
    mpStorageBottom -= numLongs;
    memcpy( mpStorageBottom, pEntry, numLongs * sizeof(int32_t) );
    mNumSymbols++;
#ifdef MAP_LOOKUP
    InitLookupMap();
#endif
}


// delete single symbol entry
void
ForthVocabulary::DeleteEntry(forthop* pEntry)
{
    forthop* pNextEntry;
    int numLongs, entryLongs;
    forthop *pSrc, *pDst;

    pNextEntry = NextEntry( pEntry );
    entryLongs = pNextEntry - pEntry;
    if ( pEntry != mpStorageBottom )
    {
        // there are newer symbols than entry, need to move newer symbols up
        pSrc = pEntry - 1;
        pDst = pNextEntry - 1;
        numLongs = pEntry - mpStorageBottom;
        while ( numLongs-- > 0 )
        {
            *pDst-- = *pSrc--;
        }
    }
    mpStorageBottom += entryLongs;
    mNumSymbols--;
#ifdef MAP_LOOKUP
    InitLookupMap();
#endif
}


// delete symbol entry and all newer entries
// return true IFF symbol was forgotten
bool
ForthVocabulary::ForgetSymbol( const char *pSymName )
{
    int j, symLen, symbolsLeft;
    forthop *pEntry, *pTmp, *pNewBottom;
    bool done;
    int32_t tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

    parseInfo.SetToken( pSymName );

    symLen = parseInfo.GetNumLongs();

    // go through the vocabulary looking for match with name
    pEntry = mpStorageBottom;

    // new bottom of vocabulary after forget
    pNewBottom = NULL;
    // how many symbols are left after forget
    symbolsLeft = mNumSymbols;

    done = false;
    while ( !done && (symbolsLeft > 0) )
    {
		uint32_t opIndex = FORTH_OP_VALUE( *pEntry );
		if ( opIndex < mpEngine->GetCoreState()->numBuiltinOps )
        {
            // sym is unknown, or in built-in ops - no way
            SPEW_VOCABULARY( "Error - attempt to forget builtin op %s from %s\n", pSymName, GetName() );
            done = true;
        }
        else
        {
            // skip value field
            pTmp = pEntry + 1;
            for ( j = 0; j < symLen; j++ )
            {
                if ( pTmp[j] != tmpSym[j] )
                {
                    // not a match
                    break;
                }
            }
            symbolsLeft--;
            if ( j == symLen )
            {
                // found it
                done = true;
                pNewBottom = NextEntry( pEntry );
            }
            else
            {
                pEntry = NextEntry( pEntry );
            }
        }
    }
    if ( pNewBottom != NULL )
    {
        //
        // if we get here, really do the forget operation
        //
        mpStorageBottom = (forthop*) pNewBottom;
        mNumSymbols = symbolsLeft;
#ifdef MAP_LOOKUP
        InitLookupMap();
#endif
        return true;
    }

    return false;
}


// forget all ops with a greater op#
void
ForthVocabulary::ForgetOp( forthop op )
{
    int symbolsLeft;
    forthop *pEntry;
    forthOpType opType;
    forthop opVal;

    // go through the vocabulary looking for symbols which are greater than op#
    pEntry = mpStorageBottom;

    // how many symbols are left after forget
    symbolsLeft = mNumSymbols;
	uint32_t opIndex;

    while ( symbolsLeft > 0 )
    {
        symbolsLeft--;
        opType = GetEntryType( pEntry );
        switch ( opType )
        {
            case kOpNative:
            case kOpNativeImmediate:
            case kOpUserDef:
            case kOpUserDefImmediate:
            case kOpCCode:
            case kOpCCodeImmediate:
				opIndex = FORTH_OP_VALUE( *pEntry );
				if ( opIndex >= mpEngine->GetCoreState()->numBuiltinOps )
				{
					opVal = GetEntryValue( pEntry );
					if ( opVal >= op )
					{
						pEntry = NextEntry( pEntry );
						mpStorageBottom = pEntry;
						mNumSymbols = symbolsLeft;
					}
					else
					{
						// this symbol was defined before the forgotten op, so we are done with this vocab
						symbolsLeft = 0;
					}
				}
				else
				{
					// can't forget builtin ops
					symbolsLeft = 0;
				}
                break;

#if 0
             // I can't figure out what the hell I was thinking here - a constant has
             //  no associated code to be forgotten, and what the hell was I getting
             //  the structure index for (from the wrong place also)?
             case kOpConstant:
                opVal = CODE_TO_STRUCT_INDEX( pEntry[1] );
                if ( opVal >= op )
                {
                    pEntry = NextEntry( pEntry );
                    mpStorageBottom = pEntry;
                    mNumSymbols = symbolsLeft;
                }
                else
                {
                    // this symbol was defined before the forgotten op, so we are done with this vocab
                    symbolsLeft = 0;
                }
                break;
#endif

            case kOpDLLEntryPoint:
                opVal = CODE_TO_DLL_ENTRY_INDEX( GetEntryValue( pEntry ) );
                if ( opVal >= op )
                {
                    pEntry = NextEntry( pEntry );
                    mpStorageBottom = pEntry;
                    mNumSymbols = symbolsLeft;
                }
                else
                {
                    // this symbol was defined before the forgotten op, so we are done with this vocab
                    symbolsLeft = 0;
                }
                break;

             default:
				pEntry = NextEntry( pEntry );
                break;
        }
    }
}


// return ptr to vocabulary entry for symbol
forthop*
ForthVocabulary::FindSymbol( const char *pSymName, ucell serial )
{
    return FindNextSymbol( pSymName, NULL, serial );
}


forthop*
ForthVocabulary::FindNextSymbol( const char *pSymName, forthop* pEntry, ucell serial )
{
    int32_t tmpSym[SYM_MAX_LONGS];
    ForthParseInfo parseInfo( tmpSym, SYM_MAX_LONGS );

#ifdef MAP_LOOKUP
    void *pEntryVoid;
    if ( mLookupMap.Lookup( pSymName, pEntryVoid ) )
    {
        return (forthop *) pEntryVoid;
    }
#endif
    parseInfo.SetToken( pSymName );

    return FindNextSymbol( &parseInfo, pEntry, serial );
}


// return ptr to vocabulary entry for symbol
forthop *
ForthVocabulary::FindSymbol( ForthParseInfo *pInfo, ucell serial )
{
	return FindNextSymbol( pInfo, NULL, serial );
}

forthop *
ForthVocabulary::FindNextSymbol( ForthParseInfo *pInfo, forthop* pStartEntry, ucell serial )
{
    int j, symLen;
    forthop *pEntry, *pTmp;
    forthop *pToken;

    if ( (serial != 0) && (serial == mLastSerial) )
    {
        // this vocabulary was already searched
        return NULL;
    }

    pToken = (forthop*)pInfo->GetTokenAsLong();
    symLen = pInfo->GetNumLongs();
    
	if ( pStartEntry != NULL )
	{
		// start search after entry pointed to by pStartEntry
		pEntry = NextEntry( pStartEntry );
	}
	else
	{
		// start search at newest symbol in vocabulary
		pEntry = mpStorageBottom;
	}

    // go through the vocabulary looking for match with name
    //for ( i = 0; i < mNumSymbols; i++ )
	while ( pEntry < mpStorageTop )
    {
        // skip value field
        pTmp = pEntry + mValueLongs;
        for ( j = 0; j < symLen; j++ )
        {
            if ( pTmp[j] != pToken[j] )
            {
                // not a match
                break;
            }
        }
        if ( j == symLen )
        {
            // found it
            return pEntry;
        }
        pEntry = NextEntry( pEntry );
    }

    // symbol isn't in vocabulary
    mLastSerial = serial;
    return NULL;
}

// return ptr to vocabulary entry given its value
forthop *
ForthVocabulary::FindSymbolByValue(forthop val, ucell serial)

{
	return FindNextSymbolByValue( val, mpStorageBottom, serial );
}

// return pointer to symbol entry, NULL if not found, given its value
forthop *
ForthVocabulary::FindNextSymbolByValue(forthop val, forthop* pStartEntry, ucell serial)
{
    int i;

	// for convenience, allow callers to pass pStartEntry==NULL on first search
	if (pStartEntry == NULL)
	{
		pStartEntry = mpStorageBottom;
	}

    forthop *pEntry = pStartEntry;

    if ( (serial != 0) && (serial == mLastSerial) )
    {
        // this vocabulary was already searched
        return NULL;
    }

    // go through the vocabulary looking for match with value
    for ( i = 0; i < mNumSymbols; i++ )
    {
        if ( *pEntry == val )
        {
            // found it
            return pEntry;
        }
        pEntry = NextEntry( pEntry );
    }
    
    // symbol isn't in vocabulary
    mLastSerial = serial;
    return NULL;
}

// process symbol entry
eForthResult
ForthVocabulary::ProcessEntry( forthop* pEntry )
{
    eForthResult exitStatus = kResultOk;
    bool compileIt = false;
    if ( mpEngine->IsCompiling() )
    {
        switch ( FORTH_OP_TYPE( *pEntry ) )
        {
            case kOpNativeImmediate:
            case kOpUserDefImmediate:
            case kOpCCodeImmediate:
                break;
            default:
                compileIt = true;
        }
    }
    if ( compileIt )
    {
        mpEngine->CompileOpcode( *pEntry );
    }
    else
    {
        // execute the opcode
        exitStatus = mpEngine->FullyExecuteOp(mpEngine->GetCoreState(), *pEntry );
    }

    return exitStatus;
}


void
ForthVocabulary::SmudgeNewestSymbol( void )
{
    // smudge by setting highest bit of 1st character of name
    ASSERT( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[1 + (mValueLongs << 2)] |= 0x80;
}


void
ForthVocabulary::UnSmudgeNewestSymbol( void )
{
    // unsmudge by clearing highest bit of 1st character of name
    ASSERT( mpNewestSymbol != NULL );
    ((char *) mpNewestSymbol)[1 + (mValueLongs << 2)] &= 0x7F;
}

ForthObject&
ForthVocabulary::GetVocabularyObject(void)
{
	if (mVocabObject == nullptr)
	{
		// vocabulary object is lazy initialized to fix an order-of-creation problem between vocabularies and the OVocabulary class object
        ForthClassVocabulary *pClassVocab = GET_CLASS_VOCABULARY(kBCIVocabulary);
        mVocabObject = reinterpret_cast<ForthObject>(&mVocabStruct);
        mVocabObject->pMethods = pClassVocab->GetMethods();
	}
	return mVocabObject;
}

const char*
ForthVocabulary::GetType( void )
{
    return "userOp";
}


void
ForthVocabulary::DoOp( ForthCoreState *pCore )
{
    ForthVocabularyStack* pVocabStack;

	pVocabStack = mpEngine->GetVocabularyStack();
	pVocabStack->SetTop(this);
}


void
ForthVocabulary::PrintEntry( forthop*   pEntry )
{
#define BUFF_SIZE 512
    char buff[BUFF_SIZE];
    ForthCoreState* pCore = mpEngine->GetCoreState();
    forthOpType entryType = GetEntryType( pEntry );
    uint32_t entryValue = GetEntryValue( pEntry );
    sprintf( buff, "  %02x:%06x    ", entryType, entryValue );
    CONSOLE_STRING_OUT( buff );

    bool showCodeAddress = false;
    char immediateChar = ' ';
    switch ( entryType )
    {
    case kOpUserDefImmediate:
        immediateChar = 'I';
    case kOpUserDef:
        showCodeAddress = CODE_IS_USER_DEFINITION( pEntry[1] );
        break;

    case kOpNativeImmediate:
        immediateChar = 'I';
    case kOpNative:
		if ( ((uint32_t) FORTH_OP_VALUE( *pEntry )) >= mpEngine->GetCoreState()->numBuiltinOps )
		{
			showCodeAddress = true;
		}
        break;
    default:
        break;
    }
    if ( showCodeAddress )
    {
        // for user defined ops the second entry field is meaningless, just show code address
        if ( entryValue < GET_NUM_OPS )
        {
#if defined(FORTH64)
            sprintf(buff, "%016llx *%c  ", OP_TABLE[entryValue], immediateChar);
#else
            sprintf(buff, "%08x *%c  ", OP_TABLE[entryValue], immediateChar);
#endif
            CONSOLE_STRING_OUT( buff );
        }
        else
        {
            sprintf( buff, "%08x - out of range", entryValue );
            CONSOLE_STRING_OUT( buff );
        }
     }
    else
    {
        for ( int j = 1; j < mValueLongs; j++ )
        {
            sprintf(buff, "%08x  %c ", pEntry[j], immediateChar);
            CONSOLE_STRING_OUT( buff );
        }
    }

    GetEntryName( pEntry, buff, BUFF_SIZE );
    CONSOLE_STRING_OUT( buff );
}

ForthVocabulary* ForthVocabulary::FindVocabulary( const char* pName )
{
    ForthVocabulary *pVocab = mpChainHead;
    while ( pVocab != NULL )
    {
        if ( strcmp( pName, pVocab->mpName ) == 0 )
        {
            break;
        }
        pVocab = pVocab->mpChainNext;
    }
    return pVocab;
}

bool
ForthVocabulary::IsStruct()
{
	return false;
}

bool
ForthVocabulary::IsClass()
{
	return false;
}

void
ForthVocabulary::AfterStart()
{
}

int
ForthVocabulary::Save( FILE* pOutFile )
{
    return 0;
}

bool
ForthVocabulary::Restore( const char* pBuffer, uint32_t numBytes )
{
    return true;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthLocalVocabulary
//
//

ForthLocalVocabulary::ForthLocalVocabulary( const char    *pName,
                                            int           valueLongs,
                                            int           storageBytes )
: ForthVocabulary( pName, valueLongs, storageBytes )
, mDepth( 0 )
, mFrameCells( 0 )
, mpAllocOp( NULL )
{
}

ForthLocalVocabulary::~ForthLocalVocabulary()
{
}

const char*
ForthLocalVocabulary::GetType( void )
{
    return "local";
}

int
ForthLocalVocabulary::GetFrameCells()
{
	return mFrameCells;
}

forthop*
ForthLocalVocabulary::GetFrameAllocOpPointer()
{
	return mpAllocOp;
}

forthop*
ForthLocalVocabulary::AddVariable( const char* pVarName, int32_t fieldType, int32_t varValue, int nCells)
{
    forthop op = COMPILED_OP(fieldType, varValue);
    forthop* pEntry = AddSymbol(pVarName, op);
    if (mFrameCells == 0)
    {
		mpAllocOp = mpEngine->GetDP();
	}
    mFrameCells += nCells;
	return pEntry;
}

void
ForthLocalVocabulary::ClearFrame()
{
    mFrameCells = 0;
	mpAllocOp = NULL;
}

// 'hide' current local variables - used at start of anonymous function declaration
void
ForthLocalVocabulary::Push()
{
	if ( mDepth < MAX_LOCAL_DEPTH )
	{
		mStack[mDepth++] = mNumSymbols;
		mStack[mDepth++] = mFrameCells;
		mStack[mDepth++] = (cell) mpAllocOp;
		mNumSymbols = 0;
        mFrameCells = 0;
		mpAllocOp = NULL;
	}
	else
	{
		// TODO: ERROR!
	}
}

void
ForthLocalVocabulary::Pop()
{
	if ( mDepth > 0 )
	{
		while ( mNumSymbols > 0 )
		{
			mpStorageBottom = NextEntry( mpStorageBottom );
			mNumSymbols--;
		}
		mpAllocOp = (forthop *) (mStack[--mDepth]);
        mFrameCells = mStack[--mDepth];
		mNumSymbols = mStack[--mDepth];
	}
	else
	{
		// TBD: ERROR!
	}
}

//////////////////////////////////////////////////////////////////////
////
///     ForthDLLVocabulary
//
//

ForthDLLVocabulary::ForthDLLVocabulary(const char      *pName,
    const char      *pDLLName,
    int             valueLongs,
    int             storageBytes,
    void*           pForgetLimit,
    forthop         op)
    : ForthVocabulary(pName, valueLongs, storageBytes, pForgetLimit, op)
    , mDLLFlags(0)
{
    int len = strlen(pDLLName) + 1;
    mpDLLName = new char[len];
    strcpy(mpDLLName, pDLLName);
    LoadDLL();
}

ForthDLLVocabulary::~ForthDLLVocabulary()
{
    UnloadDLL();
    delete [] mpDLLName;
}

void* ForthDLLVocabulary::LoadDLL( void )
{

    ForthEngine* pEngine = ForthEngine::GetInstance();
    ForthShell* pShell = pEngine->GetShell();
    char* pDLLPath = nullptr;
    const char *pDLLSrc = mpDLLName;
    int len = strlen(mpDLLName) + 1;
    if (!pEngine->GetCoreState()->pFileFuncs->fileExists(mpDLLName))
    {
        const char* pDLLDir = pShell->GetDLLDir();
        pDLLPath = new char[strlen(pDLLDir) + len];
        strcpy(pDLLPath, pDLLDir);
        strcat(pDLLPath, mpDLLName);
        pDLLSrc = pDLLPath;
    }
#if defined(WINDOWS_BUILD)
    mhDLL = LoadLibrary(pDLLSrc);
    if (mhDLL == 0)
    {
        pEngine->SetError(kForthErrorFileOpen, "failed to open DLL ");
        pEngine->AddErrorText(pDLLSrc);
    }
    delete[] pDLLPath;
    return mhDLL;
#elif defined(LINUX) || defined(MACOSX)
    mLibHandle = dlopen(pDLLSrc, RTLD_LAZY);
    if (mLibHandle == nullptr)
    {
        pEngine->SetError(kForthErrorFileOpen, "failed to open DLL ");
        pEngine->AddErrorText(pDLLSrc);
    }
    delete[] pDLLPath;
    return mLibHandle;
#endif
}

void ForthDLLVocabulary::UnloadDLL( void )
{
#if defined(WINDOWS_BUILD)
    if ( mhDLL != 0 )
    {
    	FreeLibrary( mhDLL );
    	mhDLL = 0;
    }
#elif defined(LINUX) || defined(MACOSX)
    if ( mLibHandle != NULL )
    {
        dlclose( mLibHandle  );
        mLibHandle = NULL;
    }
#endif
}

forthop * ForthDLLVocabulary::AddEntry( const char *pFuncName, const char* pEntryName, int32_t numArgs )
{
    forthop *pEntry = NULL;
#if defined(WINDOWS_BUILD)
    void* pFunc = GetProcAddress( mhDLL, pFuncName );
#elif defined(LINUX) || defined(MACOSX)
    void* pFunc = dlsym( mLibHandle, pFuncName );
#endif
    if (pFunc )
    {
        forthop dllOp = mpEngine->AddOp(pFunc);
        dllOp = COMPILED_OP(kOpDLLEntryPoint, dllOp) | ((numArgs << 19) | mDLLFlags);
        pEntry = AddSymbol(pEntryName, dllOp);
    }
    else
    {
        mpEngine->SetError( kForthErrorUnknownSymbol, " unknown entry point" );
    }
	// void and 64-bit flags only apply to one vocabulary entry
	mDLLFlags &= ~(DLL_ENTRY_FLAG_RETURN_VOID | DLL_ENTRY_FLAG_RETURN_64BIT);

    return pEntry;
}

const char*
ForthDLLVocabulary::GetType( void )
{
    return "dllOp";
}

void
ForthDLLVocabulary::SetFlag( uint32_t flag )
{
	mDLLFlags |= flag;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthVocabularyStack
//
//

ForthVocabularyStack::ForthVocabularyStack( int maxDepth )
: mStack( NULL )
, mMaxDepth( maxDepth )
, mTop( 0 )
, mSerial( 0 )
{
    mpEngine = ForthEngine::GetInstance();
}

ForthVocabularyStack::~ForthVocabularyStack()
{
    delete mStack;
}

void ForthVocabularyStack::Initialize( void )
{
    delete mStack;
    mTop = 0;
    mStack = new ForthVocabulary* [ mMaxDepth ];
}

void ForthVocabularyStack::DupTop( void )
{
    if ( mTop < (mMaxDepth - 1) )
    {
        mTop++;
        mStack[ mTop ] = mStack[ mTop - 1 ];
    }
    else
    {
        // TBD: report overflow
    }
}

bool ForthVocabularyStack::DropTop( void )
{
    if ( mTop )
    {
        mTop--;
    }
    else
    {
        return false;
    }
    return true;
}

void ForthVocabularyStack::Clear( void )
{
    mTop = 0;
    mStack[0] = mpEngine->GetForthVocabulary();
//    mStack[1] = mpEngine->GetPrecedenceVocabulary();
}

void ForthVocabularyStack::SetTop( ForthVocabulary* pVocab )
{
    mStack[mTop] = pVocab;
}

ForthVocabulary* ForthVocabularyStack::GetTop( void )
{
    return mStack[mTop];
}

ForthVocabulary* ForthVocabularyStack::GetElement( int depth )
{
    return (depth > mTop) ? NULL : mStack[mTop - depth];
}

// return pointer to symbol entry, NULL if not found
// ppFoundVocab will be set to the vocabulary the symbol was actually found in
// set ppFoundVocab to NULL to search just this vocabulary (not the search chain)
forthop* ForthVocabularyStack::FindSymbol( const char *pSymName, ForthVocabulary** ppFoundVocab )
{
    forthop* pEntry = NULL;
    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pSymName, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    if ( (pEntry == NULL) && mpEngine->CheckFeature( kFFIgnoreCase ) )
    {
        // if symbol wasn't found, convert it to lower case and try again
        char buffer[128];
        strncpy( buffer, pSymName, sizeof(buffer) );
        for ( int i = 0; i < sizeof(buffer); i++ )
        {
            char ch = buffer[i];
            if ( ch == '\0' )
            {
                break;
            }
            buffer[i] = tolower( ch );
        }
        // try to find the lower cased version
        mSerial++;
        for ( int i = mTop; i >= 0; i-- )
        {
            pEntry = mStack[i]->FindSymbol( buffer, mSerial );
            if ( pEntry )
            {
                if ( ppFoundVocab != NULL )
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    return pEntry;
}

// return pointer to symbol entry, NULL if not found, given its value
forthop * ForthVocabularyStack::FindSymbolByValue( int32_t val, ForthVocabulary** ppFoundVocab )
{
    forthop *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbolByValue( val, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }
    return pEntry;
}

// return pointer to symbol entry, NULL if not found
// pSymName is required to be a longword aligned address, and to be padded with 0's
// to the next longword boundary
forthop * ForthVocabularyStack::FindSymbol( ForthParseInfo *pInfo, ForthVocabulary** ppFoundVocab )
{
    forthop *pEntry = NULL;

    mSerial++;
    for ( int i = mTop; i >= 0; i-- )
    {
        pEntry = mStack[i]->FindSymbol( pInfo, mSerial );
        if ( pEntry )
        {
            if ( ppFoundVocab != NULL )
            {
                *ppFoundVocab = mStack[i];
            }
            break;
        }
    }

    if ( (pEntry == NULL) && mpEngine->CheckFeature( kFFIgnoreCase ) )
    {
        // if symbol wasn't found, convert it to lower case and try again
        char buffer[128];
        strncpy( buffer, pInfo->GetToken(), sizeof(buffer) );
        for ( int i = 0; i < sizeof(buffer); i++ )
        {
            char ch = buffer[i];
            if ( ch == '\0' )
            {
                break;
            }
            buffer[i] = tolower( ch );
        }
        // try to find the lower cased version
        mSerial++;
        for ( int i = mTop; i >= 0; i-- )
        {
            pEntry = mStack[i]->FindSymbol( buffer, mSerial );
            if ( pEntry )
            {
                if ( ppFoundVocab != NULL )
                {
                    *ppFoundVocab = mStack[i];
                }
                break;
            }
        }
    }

    return pEntry;
}

namespace OVocabulary
{
	//////////////////////////////////////////////////////////////////////
	///
	//                 Vocabulary
	//
	struct oVocabularyStruct
	{
        forthop*            pMethods;
        ucell               refCount;
		ForthVocabulary*	vocabulary;
	};

	struct oVocabularyIterStruct
	{
        forthop*            pMethods;
        ucell               refCount;
        ForthObject			parent;
		forthop*            cursor;
		ForthVocabulary*	vocabulary;
	};

	FORTHOP(oVocabularyNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Vocabulary object");
	}

	FORTHOP(oVocabularyDeleteMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		pVocabulary->refCount = 1;
		// TODO: warn about deleting a vocabulary object
        METHOD_RETURN;
    }

	FORTHOP(oVocabularyGetNameMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		SPUSH((pVocab != NULL) ? (cell)(pVocab->GetName()) : NULL);
		METHOD_RETURN;
	}
	/*
	FORTHOP(oVocabularySetDefinitionsVocabMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
			pEngine->SetDefinitionVocabulary(pVocabStack->GetTop());
		}
		METHOD_RETURN;
	}

	FORTHOP(oVocabularySetSearchVocabMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyPushSearchVocabMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		if (pVocab != NULL)
		{
			ForthEngine *pEngine = GET_ENGINE;
			ForthVocabularyStack* pVocabStack = pEngine->GetVocabularyStack();
			pVocabStack->DupTop();
			pVocabStack->SetTop(pVocab);
		}
		METHOD_RETURN;
	}
	*/
	FORTHOP(oVocabularyHeadIterMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		pVocabulary->refCount++;
		TRACK_KEEP;
		ForthClassVocabulary *pIterVocab = ForthTypesManager::GetInstance()->GetClassVocabulary(kBCIVocabularyIter);
		MALLOCATE_ITER(oVocabularyIterStruct, pIter, pIterVocab);
        pIter->pMethods = pIterVocab->GetMethods();
        pIter->refCount = 0;
		pIter->parent = reinterpret_cast<ForthObject>(pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		pIter->vocabulary = pVocab;
		pIter->cursor = pVocab->GetNewestEntry();
		PUSH_OBJECT(pIter);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyFindEntryByNameMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		forthop* pEntry = NULL;
		const char* pName = (const char *)(SPOP);
		if (pVocab != NULL)
		{
			pEntry = pVocab->FindSymbol(pName);
		}
        SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyGetNewestEntryMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		SPUSH((pVocab != NULL) ? (cell)(pVocab->GetNewestEntry()) : 0);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyGetNumEntriesMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		SPUSH((pVocab != NULL) ? (pVocab->GetNumEntries()) : 0);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyGetValueLengthMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
		SPUSH((pVocab != NULL) ? (pVocab->GetValueLength()) : 0);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyAddSymbolMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		ForthVocabulary* pVocab = pVocabulary->vocabulary;
        cell addToEngineFlags = SPOP;
		forthop opVal = SPOP;
        cell opType = SPOP;
		char* pSymbol = (char *)(SPOP);
		if (pVocab != NULL)
		{
            bool addToEngineOps = (addToEngineFlags < 0) ?
                (opType <= kOpDLLEntryPoint) : (addToEngineFlags > 0);

            if (addToEngineOps)
            {
                ForthEngine *pEngine = ForthEngine::GetInstance();
                opVal = pEngine->AddOp(reinterpret_cast<void *>(opVal));
            }
            opVal = COMPILED_OP(opType, opVal);

            pVocab->AddSymbol(pSymbol, opVal);
		}
		METHOD_RETURN;
	}

    FORTHOP(oVocabularyChainNextMethod)
    {
        GET_THIS(oVocabularyStruct, pVocabulary);
        ForthVocabulary* nextVocabInChain = pVocabulary->vocabulary->GetNextChainVocabulary();
        cell result = 0;
        if (nextVocabInChain != nullptr)
        {
            ForthObject obj = nextVocabInChain->GetVocabularyObject();
            if (obj)
            {
                PUSH_OBJECT(obj);
                result--;
            }
        }
        SPUSH(result);
        METHOD_RETURN;
    }

    
	baseMethodEntry oVocabularyMembers[] =
	{
		METHOD("__newOp", oVocabularyNew),
		METHOD("delete", oVocabularyDeleteMethod),
		METHOD("getName", oVocabularyGetNameMethod),
		METHOD_RET("headIter", oVocabularyHeadIterMethod, RETURNS_OBJECT(kBCIVocabularyIter)),
		METHOD("findEntryByName", oVocabularyFindEntryByNameMethod),
		METHOD("getNewestEntry", oVocabularyGetNewestEntryMethod),
		METHOD("getNumEntries", oVocabularyGetNumEntriesMethod),
		METHOD("getValueLength", oVocabularyGetValueLengthMethod),
		METHOD("addSymbol", oVocabularyAddSymbolMethod),
        METHOD_RET("chainNext", oVocabularyChainNextMethod, RETURNS_NATIVE(kBCIInt)),

		MEMBER_VAR("vocabulary", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

		// following must be last in table
		END_MEMBERS
	};


	//////////////////////////////////////////////////////////////////////
	///
	//                 oVocabularyIter
	//

	FORTHOP(oVocabularyIterNew)
	{
		ForthEngine *pEngine = ForthEngine::GetInstance();
		pEngine->SetError(kForthErrorIllegalOperation, " cannot explicitly create a Vocabulary object");
	}

	FORTHOP(oVocabularyIterDeleteMethod)
	{
		GET_THIS(oVocabularyStruct, pVocabulary);
		// TODO: warn about deleting a vocabulary object
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyIterSeekHeadMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = NULL;
		if (pVocab != NULL)
		{
			pIter->cursor = pVocab->GetNewestEntry();
			pEntry = pIter->cursor;
		}
		SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

    FORTHOP(oVocabularyIterNextMethod)
    {
        GET_THIS(oVocabularyIterStruct, pIter);
        ForthVocabulary* pVocab = pIter->vocabulary;
        int found = 0;
        if ((pVocab != NULL) && (pIter->cursor != NULL))
        {
            SPUSH((cell)(pIter->cursor));
            pIter->cursor = pVocab->NextEntrySafe(pIter->cursor);
            found--;
        }
        SPUSH(found);
        METHOD_RETURN;
    }

    FORTHOP(oVocabularyIterFindEntryByNameMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = NULL;
		const char* pName = (const char *)(SPOP);
		if (pVocab != NULL)
		{
			pIter->cursor = pVocab->FindSymbol(pName);
			pEntry = pIter->cursor;
		}
		SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyIterFindNextEntryByNameMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = NULL;
		const char* pName = (const char *)(SPOP);
		if (pVocab != NULL)
		{
			pIter->cursor = pVocab->FindNextSymbol(pName, pEntry);
			pEntry = pIter->cursor;
		}
		SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyIterFindEntryByValueMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = NULL;
		int32_t val = SPOP;
		if (pVocab != NULL)
		{
			pIter->cursor = pVocab->FindSymbolByValue(val);
			pEntry = pIter->cursor;
		}
		SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

	FORTHOP(oVocabularyIterFindNextEntryByValueMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = NULL;
		int32_t val = SPOP;
		if (pVocab != NULL)
		{
			pIter->cursor = pVocab->FindNextSymbolByValue(val, pEntry);
			pEntry = pIter->cursor;
		}
		SPUSH((cell)pEntry);
		METHOD_RETURN;
	}

	// TODO: add oVocabularyIterInsertEntryMethod?

	FORTHOP(oVocabularyIterRemoveEntryMethod)
	{
		GET_THIS(oVocabularyIterStruct, pIter);
		ForthVocabulary* pVocab = pIter->vocabulary;
		forthop* pEntry = (forthop *)(SPOP);
		if ((pEntry != NULL) && (pVocab != NULL) && (pIter->cursor != NULL))
		{
			pVocab->DeleteEntry(pEntry);
			pIter->cursor = pVocab->GetNewestEntry();
		}
		METHOD_RETURN;
	}
	
	baseMethodEntry oVocabularyIterMembers[] =
	{
		METHOD("__newOp", oVocabularyIterNew),
		METHOD("delete", oVocabularyIterDeleteMethod),
		//METHOD("show", oVocabularyIterShowMethod),
		METHOD("seekHead", oVocabularyIterSeekHeadMethod),
		METHOD("next", oVocabularyIterNextMethod),
		METHOD("findEntryByName", oVocabularyIterFindEntryByNameMethod),
		METHOD("findNextEntryByName", oVocabularyIterFindNextEntryByNameMethod),
		METHOD("findEntryByValue", oVocabularyIterFindEntryByValueMethod),
		METHOD("findNextEntryByValue", oVocabularyIterFindNextEntryByValueMethod),
		METHOD("removeEntry", oVocabularyIterRemoveEntryMethod),

		MEMBER_VAR("parent", OBJECT_TYPE_TO_CODE(0, kBCIVocabulary)),
		MEMBER_VAR("cursor", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),
		//MEMBER_VAR("vocabulary", NATIVE_TYPE_TO_CODE(kDTIsPtr, kBaseTypeUCell)),

		// following must be last in table
		END_MEMBERS
	};

	void AddClasses(ForthEngine* pEngine)
	{
		pEngine->AddBuiltinClass("Vocabulary", kBCIVocabulary, kBCIObject, oVocabularyMembers);
		pEngine->AddBuiltinClass("VocabularyIter", kBCIVocabularyIter, kBCIObject, oVocabularyIterMembers);
	}

} // namespace OVocabulary 
