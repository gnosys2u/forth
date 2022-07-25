#pragma once

#include "Forth.h"
#include <vector>
#include <string>

//#define USE_POOLED_MEMORY_MANAGER 0

// memory allocation wrappers
#define __MALLOC s_memoryManager->malloc
#define __REALLOC s_memoryManager->realloc
#define __FREE s_memoryManager->free

#if defined(USE_POOLED_MEMORY_MANAGER)
#define __ALLOCATE_BYTES(NUM_BYTES)         s_memoryManager->allocate(NUM_BYTES)
#define __DEALLOCATE_BYTES(PTR, NUM_BYTES)  s_memoryManager->deallocate(PTR, NUM_BYTES)
#define __DEALLOCATE_OBJECT(OBJ)            s_memoryManager->deallocateObject(OBJ)
#else
#define __ALLOCATE_BYTES(NUM_BYTES)         s_memoryManager->malloc(NUM_BYTES)
#define __DEALLOCATE_BYTES(PTR, NUM_BYTES)  s_memoryManager->free(PTR)
#define __DEALLOCATE_OBJECT(OBJ)            s_memoryManager->free(OBJ)
#endif

extern void startMemoryManager();
extern void stopMemoryManager();

class ForthMemoryStats
{
public:
    ForthMemoryStats(const char* name);

    std::string& getName();

    uint64_t getNumAllocations();
    uint64_t getNumDeallocations();
    uint64_t getBytesInUse();

    uint64_t getMaxActiveAllocations();
    uint64_t getMaxBytesInUse();

    void trackAllocation(int size);
    void trackDeallocation(int size);

private:
    std::string mName;
    uint64_t mNumAllocs;
    uint64_t mNumFrees;
    uint64_t mCurrentInUse;
    uint64_t mMaxAllocs;   // maximum of numAllocs - numFrees over time
    uint64_t mMaxInUse;
};

class ForthMemoryPoolBucket
{
public:
    ForthMemoryPoolBucket(int size);
    virtual ~ForthMemoryPoolBucket();
    virtual char* allocate();
    virtual void deallocate(char*);

    int getSize() const;
    ForthMemoryStats* getMemoryStats();

protected:
    int mSize;
    char* mpFreeChain;
    ForthMemoryStats* mStats;
};


#define NUM_MEMORY_POOLS 16
#define INITIAL_BLOCK_ALLOCATION (1024 * 1024)
class ForthMemoryManager
{
public:
    ForthMemoryManager();
    virtual ~ForthMemoryManager();

    virtual void* allocate(size_t numBytes);
    virtual void deallocate(void* pBlock, size_t numBytes);
    virtual void deallocateObject(void* pObject);

    virtual void* malloc(size_t numBytes) = 0;
    virtual void* realloc(void *pMemory, size_t numBytes) = 0;
    virtual void free(void* pBlock) = 0;

    void getStats(std::vector<ForthMemoryStats*> &statsOut);

protected:
    // add a new block to mBlocks
    void allocateBlock(int blockSize);

    std::vector<char*> mBlocks;
    // how many bytes in top entry in mBlocks haven't been used
    long mUnusedInNewestBlock;
    // position in top entry in mBlocks where next allocation will come from
    char* mpNextAllocation;

    std::vector<ForthMemoryPoolBucket *> mPools;
    ForthMemoryStats* mBigThingStats;
    ForthMemoryStats* mTotalStats;
};

extern ForthMemoryManager* s_memoryManager;

class PassThruMemoryManager : public ForthMemoryManager
{
public:
    void* malloc(size_t numBytes) override;
    void* realloc(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};

class DebugPassThruMemoryManager : public ForthMemoryManager
{
public:
    void* allocate(size_t numBytes) override;
    void deallocate(void* pBlock, size_t numBytes) override;
    void deallocateObject(void* pObject) override;

    void* malloc(size_t numBytes) override;
    void* realloc(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};
