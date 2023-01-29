//////////////////////////////////////////////////////////////////////
//
// MemoryManager.cpp: implementation of the Forth memory manager classes.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Forth.h"
#include "ForthEngine.h"

#if defined(LINUX) || defined(MACOSX)
#endif

bool __useStandardMemoryAllocation = true;

MemoryManager* s_memoryManager = nullptr;

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
///     MemoryStats
//
//

MemoryStats::MemoryStats(const char* name)
    : mNumAllocs(0)
    , mNumFrees(0)
    , mCurrentInUse(0)
    , mMaxAllocs(0)
    , mMaxInUse(0)
    , mName(name)
{
}

void MemoryStats::trackAllocation(size_t size)
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

void MemoryStats::trackDeallocation(size_t size)
{
    mNumFrees++;
    mCurrentInUse -= size;
}

uint64_t MemoryStats::getNumAllocations()
{
    return mNumAllocs;
}

uint64_t MemoryStats::getNumDeallocations()
{
    return mNumFrees;
}

uint64_t MemoryStats::getBytesInUse()
{
    return mCurrentInUse;
}

uint64_t MemoryStats::getMaxActiveAllocations()
{
    return mMaxAllocs;
}

uint64_t MemoryStats::getMaxBytesInUse()
{
    return mMaxInUse;
}

std::string& MemoryStats::getName()
{
    return mName;
}

//////////////////////////////////////////////////////////////////////
////
///     MemoryPoolBucket
//
//

MemoryPoolBucket::MemoryPoolBucket(int size)
    : mSize(size)
    , mpFreeChain(nullptr)
{
    char buffer[32];
#if defined(LINUX)
    sprintf(buffer, "bucket %d", size);
    mStats = new MemoryStats(buffer);
#else
    std::string name("bucket ");
    name.append(::itoa(size, buffer, 10));
    mStats = new MemoryStats(name.c_str());
#endif
}

MemoryPoolBucket::~MemoryPoolBucket()
{
    delete mStats;
}

char* MemoryPoolBucket::allocate()
{
    mStats->trackAllocation(mSize);

    char* pResult = mpFreeChain;
    if (mpFreeChain != nullptr) {
        mpFreeChain = *(char**)mpFreeChain;
    }

    return pResult;
}

void MemoryPoolBucket::deallocate(char* pToBeFreed)
{
    mStats->trackDeallocation(mSize);

    *(char**)pToBeFreed = mpFreeChain;
    mpFreeChain = pToBeFreed;
}

int MemoryPoolBucket::getSize() const
{
    return mSize;
}

MemoryStats* MemoryPoolBucket::getMemoryStats()
{
    return mStats;
}




//////////////////////////////////////////////////////////////////////
////
///     MemoryManager
//
//

MemoryManager::MemoryManager()
    : mUnusedInNewestBlock(0)
    , mpNextAllocation(nullptr)
    , mTotalStorageSize(0)
{
    for (int i = 0; i < NUM_MEMORY_POOLS; i++) {
        mPools.push_back(new MemoryPoolBucket(8 * (i + 1)));
    }
    mBigThingStats = new MemoryStats("bigThings");
    mTotalStats = new MemoryStats("all");
}

MemoryManager::~MemoryManager()
{
    for (int i = 0; i < mBlocks.size(); i++) {
        ::free(mBlocks[i]->mStorage);
    }
}

void* MemoryManager::allocate(size_t numBytes)
{
    void* result = nullptr;
    if (numBytes != 0)
    {
        int poolNum = (int)((numBytes - 1) >> 3);
        if (poolNum < NUM_MEMORY_POOLS)
        {
            MemoryPoolBucket* pool = mPools[poolNum];
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
            	//printf(">>> allocate @%p %d bytes, pool%02d\n", result, numBytes, poolNum);
            }
            /*
            else
            {
            	printf(">>> allocate @%p %d bytes, pool%02d RECYCLED\n", result, numBytes, poolNum);
            }
            */
        }
        else
        {
            result = ::malloc(numBytes);
            mBigThingStats->trackAllocation(numBytes);
        	//printf(">>> allocateBIG @%p %d bytes\n", result, numBytes);
        }
    }

    return result;
}

void MemoryManager::deallocate(void* pBlock, size_t numBytes)
{
    /*
    if (pBlock < mBlocks[0]->mStorage || pBlock >= (mBlocks[0]->mStorage + mBlocks[0]->mSize))
    {
        printf("### bogus free %d @%p, not in storage!\n", (int)numBytes, pBlock);
    }
    */
    int poolNum = (int)((numBytes - 1) >> 3);
    if (poolNum < NUM_MEMORY_POOLS)
    {
        MemoryPoolBucket* pool = mPools[poolNum];
        pool->deallocate((char *)pBlock);
    	//printf(">>> deallocate @%p %d bytes, pool%02d\n", pBlock, numBytes, poolNum);
    }
    else
    {
    	//printf(">>> deallocateBIG @%p %d bytes\n", pBlock, numBytes);
        ::free(pBlock);
        mBigThingStats->trackDeallocation(numBytes);
    }
}

void MemoryManager::deallocateObject(void* pObject)
{
    ForthObject obj = (ForthObject)pObject;
    ForthClassObject* pClassObject = GET_CLASS_OBJECT(obj);
    deallocate(pObject, pClassObject->pVocab->GetSize());
}

void MemoryManager::allocateStorage(int size)
{
    char *pNewBlock = (char *)::malloc(size);
    mBlocks.push_back(new storageBlock(size, pNewBlock));
    mpNextAllocation = pNewBlock;
    mUnusedInNewestBlock = size;
    mTotalStorageSize += size;
}

void MemoryManager::getStats(std::vector<MemoryStats*>& statsOut, int& numStorageBlocks, int& totalStorage, int& freeStorage)
{
    for (MemoryPoolBucket* pool : mPools)
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
    void * pData = MemoryManager::allocate(numBytes);
    ForthEngine::GetInstance()->TraceOut("allocate %d bytes @ 0x%p\n", numBytes, pData);
    return pData;
}

void DebugPassThruMemoryManager::deallocate(void* pBlock, size_t numBytes)
{
    ForthEngine::GetInstance()->TraceOut("deallocate @ 0x%p\n", pBlock);
    MemoryManager::deallocate(pBlock, numBytes);
}

void DebugPassThruMemoryManager::deallocateObject(void* pObject)
{
    ForthEngine::GetInstance()->TraceOut("deallocateObject @ 0x%p\n", pObject);
    MemoryManager::deallocateObject(pObject);
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

