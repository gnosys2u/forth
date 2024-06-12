#pragma once
//////////////////////////////////////////////////////////////////////
//
// MemoryManager.h: interface for the Forth memory manager classes.
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

#include "Forth.h"
#include <vector>
#include <string>

#define USE_POOLED_MEMORY_MANAGER 1

// memory allocation wrappers
#define __MALLOC s_memoryManager->malloc
#define __REALLOC s_memoryManager->realloc
#define __FREE s_memoryManager->free

#if defined(USE_POOLED_MEMORY_MANAGER)
#define ALLOCATE_BYTES(NUM_BYTES)         s_memoryManager->allocate(NUM_BYTES)
#define DEALLOCATE_BYTES(PTR, NUM_BYTES)  s_memoryManager->deallocate(PTR, NUM_BYTES)
#define DEALLOCATE_OBJECT(OBJ)            s_memoryManager->deallocateObject(OBJ)
#else
#define ALLOCATE_BYTES(NUM_BYTES)         s_memoryManager->malloc(NUM_BYTES)
#define DEALLOCATE_BYTES(PTR, NUM_BYTES)  s_memoryManager->free(PTR)
#define DEALLOCATE_OBJECT(OBJ)            s_memoryManager->free(OBJ)
#endif

extern void startMemoryManager();
extern void stopMemoryManager();

class MemoryStats
{
public:
    MemoryStats(const char* name);

    std::string& getName();

    uint64_t getNumAllocations();
    uint64_t getNumDeallocations();
    uint64_t getBytesInUse();

    uint64_t getMaxActiveAllocations();
    uint64_t getMaxBytesInUse();

    void trackAllocation(size_t size);
    void trackDeallocation(size_t size);

private:
    std::string mName;
    uint64_t mNumAllocs;
    uint64_t mNumFrees;
    uint64_t mCurrentInUse;
    uint64_t mMaxAllocs;   // maximum of numAllocs - numFrees over time
    uint64_t mMaxInUse;
};

class MemoryPoolBucket
{
public:
    MemoryPoolBucket(int size);
    virtual ~MemoryPoolBucket();
    virtual char* allocate();
    virtual void deallocate(char*);

    int getSize() const;
    MemoryStats* getMemoryStats();

protected:
    int mSize;
    char* mpFreeChain;
    MemoryStats* mStats;
};


#define NUM_MEMORY_POOLS 16
#define INITIAL_BLOCK_ALLOCATION (1024 * 1024)
class MemoryManager
{
public:
    MemoryManager();
    virtual ~MemoryManager();

    virtual void* allocate(size_t numBytes);
    virtual void deallocate(void* pBlock, size_t numBytes);
    virtual void deallocateObject(void* pObject);

    virtual void* malloc(size_t numBytes) = 0;
    virtual void* realloc(void *pMemory, size_t numBytes) = 0;
    virtual void free(void* pBlock) = 0;

    void getStats(std::vector<MemoryStats*> &statsOut, int& numStorageBlocks, int& totalStorage, int& freeStorage);

protected:
    struct storageBlock
    {
        int mSize;
        char* mStorage;
        storageBlock(int size, char* pStorage)
            : mSize(size)
            , mStorage(pStorage) {}
    };

    // add a new block to mBlocks
    void allocateStorage(int blockSize);

    std::vector<storageBlock *> mBlocks;
    // how many bytes in top entry in mBlocks haven't been used
    int mUnusedInNewestBlock;
    int mTotalStorageSize;
    // position in top entry in mBlocks where next allocation will come from
    char* mpNextAllocation;

    std::vector<MemoryPoolBucket *> mPools;
    MemoryStats* mBigThingStats;
    MemoryStats* mTotalStats;
};

extern MemoryManager* s_memoryManager;

class PassThruMemoryManager : public MemoryManager
{
public:
    void* malloc(size_t numBytes) override;
    void* realloc(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};

class DebugPassThruMemoryManager : public MemoryManager
{
public:
    void* allocate(size_t numBytes) override;
    void deallocate(void* pBlock, size_t numBytes) override;
    void deallocateObject(void* pObject) override;

    void* malloc(size_t numBytes) override;
    void* realloc(void *pMemory, size_t numBytes) override;
    void free(void* pBlock) override;
};
