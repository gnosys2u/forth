//////////////////////////////////////////////////////////////////////
//
// ForthMemoryManager.cpp: implementation of the Forth memory manager classes.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Forth.h"
#include "ForthEngine.h"

#if defined(LINUX) || defined(MACOSX)
#endif

bool __useStandardMemoryAllocation = true;

ForthMemoryManager* s_memoryManager = nullptr;

void startMemoryManager()
{
    if (s_memoryManager == nullptr)
    {
        if (__useStandardMemoryAllocation)
        {
            s_memoryManager = new PassThruMemoryManager;
        }
        else
        {
            s_memoryManager = new DebugPassThruMemoryManager;
        }
    }
}

void stopMemoryManager()
{
    if (s_memoryManager != nullptr)
    {
        delete s_memoryManager;
        s_memoryManager = nullptr;
    }
}

/*
* The memory manager allocates a large block of memory which is used for
* multiple requests made with the allocatePooled method.  There are N (16 by default)
* 'pools' of freed blocks of the same size, a pool for each of the sizes 0,8,16...112.
* If the requested size is larger than the largest size for which there is a pool, the
* request is just forwarded to the malloc method.
* requests come in, if the requested size is one of
*/

//////////////////////////////////////////////////////////////////////
////
///     ForthMemoryStats
//
//

ForthMemoryStats::ForthMemoryStats(const char* name)
    : mNumAllocs(0)
    , mNumFrees(0)
    , mCurrentInUse(0)
    , mMaxAllocs(0)
    , mMaxInUse(0)
    , mName(name)
{
}

void ForthMemoryStats::trackAllocation(int size)
{
    mNumAllocs++;
    uint64_t activeAllocs = mNumAllocs - mNumFrees;
    if (activeAllocs > mMaxAllocs)
    {
        mMaxAllocs = activeAllocs;
    }

    mCurrentInUse += size;
    if (mCurrentInUse > mMaxInUse)
    {
        mMaxInUse = mCurrentInUse;
    }
}

void ForthMemoryStats::trackDeallocation(int size)
{
    mNumFrees++;
    mCurrentInUse -= size;
}

uint64_t ForthMemoryStats::getNumAllocations()
{
    return mNumAllocs;
}

uint64_t ForthMemoryStats::getNumDeallocations()
{
    return mNumFrees;
}

uint64_t ForthMemoryStats::getBytesInUse()
{
    return mCurrentInUse;
}

uint64_t ForthMemoryStats::getMaxActiveAllocations()
{
    return mMaxAllocs;
}

uint64_t ForthMemoryStats::getMaxBytesInUse()
{
    return mMaxInUse;
}

std::string& ForthMemoryStats::getName()
{
    return mName;
}

//////////////////////////////////////////////////////////////////////
////
///     ForthMemoryPoolBucket
//
//

ForthMemoryPoolBucket::ForthMemoryPoolBucket(int size)
    : mSize(size)
    , mpFreeChain(nullptr)
{
    char buffer[32];
    std::string name("bucket ");
    name.append(::itoa(size, buffer, 10));
    mStats = new ForthMemoryStats(name.c_str());
}

ForthMemoryPoolBucket::~ForthMemoryPoolBucket()
{
    delete mStats;
}

char* ForthMemoryPoolBucket::allocate()
{
    mStats->trackAllocation(mSize);

    char* pResult = mpFreeChain;
    if (mpFreeChain != nullptr) {
        mpFreeChain = *(char**)mpFreeChain;
    }

    return pResult;
}

void ForthMemoryPoolBucket::deallocate(char* pToBeFreed)
{
    mStats->trackDeallocation(mSize);

    *(char**)pToBeFreed = mpFreeChain;
    mpFreeChain = pToBeFreed;
}

int ForthMemoryPoolBucket::getSize() const
{
    return mSize;
}

ForthMemoryStats* ForthMemoryPoolBucket::getMemoryStats()
{
    return mStats;
}




//////////////////////////////////////////////////////////////////////
////
///     ForthMemoryManager
//
//

ForthMemoryManager::ForthMemoryManager()
    : mUnusedInNewestBlock(0)
    , mpNextAllocation(nullptr)
    , mTotalStorageSize(0)
{
    for (int i = 0; i < NUM_MEMORY_POOLS; i++) {
        mPools.push_back(new ForthMemoryPoolBucket(8 * (i + 1)));
    }
    mBigThingStats = new ForthMemoryStats("bigThings");
    mTotalStats = new ForthMemoryStats("all");
}

ForthMemoryManager::~ForthMemoryManager()
{
    for (int i = 0; i < mBlocks.size(); i++) {
        ::free(mBlocks[i]->mStorage);
    }
}

void* ForthMemoryManager::allocate(size_t numBytes)
{
    void* result = nullptr;
    if (numBytes != 0)
    {
        int poolNum = (numBytes - 1) >> 3;
        if (poolNum < NUM_MEMORY_POOLS)
        {
            ForthMemoryPoolBucket* pool = mPools[poolNum];
            result = pool->allocate();
            if (result == nullptr)
            {
                int size = pool->getSize();
                if (mUnusedInNewestBlock < size)
                {
                    allocateStorage(INITIAL_BLOCK_ALLOCATION);
                }
                mUnusedInNewestBlock -= size;
                result = mpNextAllocation;
                mpNextAllocation += size;
            }
        }
        else
        {
            result = ::malloc(numBytes);
            mBigThingStats->trackAllocation(numBytes);
        }
    }

    return result;
}

void ForthMemoryManager::deallocate(void* pBlock, size_t numBytes)
{
    /*
    if (pBlock < mBlocks[0]->mStorage || pBlock >= (mBlocks[0]->mStorage + mBlocks[0]->mSize))
    {
        printf("### bogus free %d @%p, not in storage!\n", (int)numBytes, pBlock);
    }
    */
    int poolNum = (numBytes - 1) >> 3;
    if (poolNum < NUM_MEMORY_POOLS)
    {
        ForthMemoryPoolBucket* pool = mPools[poolNum];
        pool->deallocate((char *)pBlock);
    }
    else
    {
        ::free(pBlock);
        mBigThingStats->trackDeallocation(numBytes);
    }
}

void ForthMemoryManager::deallocateObject(void* pObject)
{
    ForthObject obj = (ForthObject)pObject;
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
    deallocate(pObject, pClassObject->pVocab->GetSize());
}

void ForthMemoryManager::allocateStorage(int size)
{
    char *pNewBlock = (char *)::malloc(size);
    mBlocks.push_back(new storageBlock(size, pNewBlock));
    mpNextAllocation = pNewBlock;
    mUnusedInNewestBlock = size;
    mTotalStorageSize += size;
}

void ForthMemoryManager::getStats(std::vector<ForthMemoryStats*>& statsOut, int& numStorageBlocks, int& totalStorage, int& freeStorage)
{
    for (ForthMemoryPoolBucket* pool : mPools)
    {
        statsOut.push_back(pool->getMemoryStats());
    }
    statsOut.push_back(mBigThingStats);
    statsOut.push_back(mTotalStats);

    numStorageBlocks = (int)mBlocks.size();
    totalStorage = mTotalStorageSize;
    freeStorage = mUnusedInNewestBlock;
}

//////////////////////////////////////////////////////////////////////
////
///     PassThruMemoryManager
//
//

void* PassThruMemoryManager::malloc(size_t numBytes)
{
    return ::malloc(numBytes);
}

void* PassThruMemoryManager::realloc(void *pMemory, size_t numBytes)
{
    return ::realloc(pMemory, numBytes);
}

void PassThruMemoryManager::free(void* pBlock)
{
    ::free(pBlock);
}


//////////////////////////////////////////////////////////////////////
////
///     DebugPassThruMemoryManager
//
//

void* DebugPassThruMemoryManager::allocate(size_t numBytes)
{
    void * pData = ForthMemoryManager::allocate(numBytes);
    ForthEngine::GetInstance()->TraceOut("allocate %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}

void DebugPassThruMemoryManager::deallocate(void* pBlock, size_t numBytes)
{
    ForthEngine::GetInstance()->TraceOut("deallocate @ 0x%p\n", pBlock);
    ForthMemoryManager::deallocate(pBlock, numBytes);
}

void DebugPassThruMemoryManager::deallocateObject(void* pObject)
{
    ForthEngine::GetInstance()->TraceOut("deallocateObject @ 0x%p\n", pObject);
    ForthMemoryManager::deallocateObject(pObject);
}

void* DebugPassThruMemoryManager::malloc(size_t numBytes)
{
    void* pData = ::malloc(numBytes);
    ForthEngine::GetInstance()->TraceOut("malloc %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}

void* DebugPassThruMemoryManager::realloc(void *pMemory, size_t numBytes)
{
    void *pData = ::realloc(pMemory, numBytes);
    ForthEngine::GetInstance()->TraceOut("realloc %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}

void DebugPassThruMemoryManager::free(void* pBlock)
{
    ForthEngine::GetInstance()->TraceOut("free @ 0x%p\n", pBlock);
    ::free(pBlock);
}

