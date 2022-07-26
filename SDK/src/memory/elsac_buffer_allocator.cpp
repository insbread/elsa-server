/***
 * @Author: insbread
 * @Date: 2022-07-17 15:30:35
 * @LastEditTime: 2022-07-17 15:30:35
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/memory/elsac_buffer_allocator.cpp
 * @版权声明
 */
#include "elsa_x_tick.h"
#include "memory/elsac_buffer_allocator.h"

#ifdef _MEMORY_TRACE_
#define _Free(x) /*alloc_total_ -= x->size_; */ free(x);
#else
#define _Free(x) free(x)
#endif
static const int BufferFreeTick = 1 * 1000; //标记为释放的的内存块的延时销毁时间

ElsaCBufferAllocator::ElsaCBufferAllocator(const char *name) : ElsaCBaseAllocator(name)
{
    m_pLastSmallBuffer = nullptr;
    m_pLastMiddleBuffer = nullptr;
    m_pLastLargeBuffer = nullptr;
    m_pLastSuperBuffer = nullptr;
#ifdef _MEMORY_TRACE_
    m_pLastAllocedBuffer = nullptr;
#endif
    m_szAllocedSmallSize = 0;
    m_szAllocedMiddleSize = 0;
    m_szAllocedLargeSize = 0;
    m_szAllocedSuperSize = 0;
    m_szFreedSmallSize = 0;
    m_szFreedMiddelSize = 0;
    m_szFreedLargeSize = 0;
    m_szFreedSuperSize = 0;

    m_bMultThread = false;
}

ElsaCBufferAllocator::~ElsaCBufferAllocator()
{
    PBUFFER pBuffer, pPrevBuffer;

    pBuffer = m_pLastSmallBuffer;
    while (pBuffer != nullptr)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        _Free(pBuffer);
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastMiddleBuffer;
    while (pBuffer != nullptr)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        _Free(pBuffer);
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastLargeBuffer;
    while (pBuffer != nullptr)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        _Free(pBuffer);
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastSuperBuffer;
    while (pBuffer != nullptr)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        _Free(pBuffer);
        pBuffer = pPrevBuffer;
    }

#ifdef _MEMORY_TRACE_
    pBuffer = m_pLastAllocedBuffer;

    while (pBuffer)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        OutputMsg(rmError, ("%s(%d) : memory has not free！pointer Addr=%d\n"),
                  pBuffer->m_al.m_pFn, pBuffer->m_al.m_iLine, (size_t)pBuffer);

        char err[1024];
        SNPRINTFA(err, sizeof(err) - 1, ("%s(%d) : memory has not free！pointer Addr=%d\n"),
                  pBuffer->m_al.m_pFn, pBuffer->m_al.m_iLine, (size_t)pBuffer);
        OutputWatchFile(err, "err.log");

        assert(false); //  这里是出现了内存泄露了，按道理来说pBuffer最后应该是nullptr的
        _Free(pBuffer);
        pBuffer = pPrevBuffer;
    }

#endif
}

#ifndef _MEMORY_TRACE_
void *ElsaCBufferAllocator::AllocBuffer(size_t size)
#else
void *ElsaCBufferAllocator::_AllocBuffer(size_t size, const char *fn, int line)
#endif
{
    PBUFFER pBuffer, pPrev, pResult = nullptr;
    if (m_bMultThread == true)
        m_bufferLock.Lock();

    if (size <= SmallBufferSize)
    {
        if (m_pLastSmallBuffer != nullptr)
        {
            pResult = m_pLastSmallBuffer;
            m_pLastSmallBuffer = m_pLastSmallBuffer->m_pPrevBuffer;
            pResult->m_pPrevBuffer = nullptr;
            m_szFreedSmallSize -= SmallBufferSize;
        }
        else
        {
#ifdef _MEMORY_TRACE_
            // 分配数据的一个结点内存格局 BUFFER | size of bytes | int;
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + SmallBufferSize + sizeof(int)));
            pResult->m_bUsedFlag = false;
            pResult->m_pNextBuffer = nullptr;
#else
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + SmallBufferSize));
#endif
            pResult->m_size = SmallBufferSize;
            pResult->m_pPrevBuffer = nullptr;
        }
        m_szAllocedSmallSize += SmallBufferSize;
    }
    else if (size <= MiddleBufferSize)
    {
        if (m_pLastMiddleBuffer != nullptr)
        {
            pResult = m_pLastMiddleBuffer;
            m_pLastMiddleBuffer = m_pLastMiddleBuffer->m_pPrevBuffer;
            pResult->m_pPrevBuffer = nullptr;
            m_szFreedMiddelSize -= MiddleBufferSize;
        }
        else
        {
#ifdef _MEMORY_TRACE_
            // 分配数据的一个结点内存格局 BUFFER | size of bytes | int;
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + MiddleBufferSize + sizeof(int)));
            pResult->m_bUsedFlag = false;
            pResult->m_pNextBuffer = nullptr;
#else
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + MiddleBufferSize));
#endif
            pResult->m_size = MiddleBufferSize;
            pResult->m_pPrevBuffer = nullptr;
        }
        m_szAllocedMiddleSize += MiddleBufferSize;
    }
    else if (size <= LargeBufferSize)
    {
        if (m_pLastLargeBuffer != nullptr)
        {
            pResult = m_pLastLargeBuffer;
            m_pLastLargeBuffer = m_pLastLargeBuffer->m_pPrevBuffer;
            pResult->m_pPrevBuffer = nullptr;
            m_szFreedLargeSize -= LargeBufferSize;
        }
        else
        {
#ifdef _MEMORY_TRACE_
            // 分配数据的一个结点内存格局 BUFFER | size of bytes | int;
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + LargeBufferSize + sizeof(int)));
            pResult->m_bUsedFlag = false;
            pResult->m_pNextBuffer = nullptr;
#else
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + LargeBufferSize));
#endif
            pResult->m_size = LargeBufferSize;
            pResult->m_pPrevBuffer = nullptr;
        }
        m_szAllocedLargeSize += LargeBufferSize;
    }
    else
    {
        pPrev = nullptr;
        pBuffer = m_pLastSuperBuffer;
        // 注意遍历过程中的变量位置 pBuffer <- pPrev
        while (pBuffer != nullptr)
        {
            if (pBuffer->m_size >= size)
            {
                pResult = pBuffer;
#ifdef _MEMORY_TRACE_
                assert(pResult->m_bUsedFlag);
#endif
                if (pResult == m_pLastSuperBuffer)
                {
                    // 删除头结点
                    m_pLastSuperBuffer = m_pLastSuperBuffer->m_pPrevBuffer;
                    pResult->m_pPrevBuffer = nullptr;
                }
                else if (pPrev != nullptr) //  按道理来说非头结点pPrev不应该是空的
                {
                    //  删除非头结点
                    pPrev->m_pPrevBuffer = pResult->m_pPrevBuffer;
                    pResult->m_pPrevBuffer = nullptr;
                }

                m_szFreedSuperSize -= size;
                break;
            }

            pPrev = pBuffer;
            pBuffer = pBuffer->m_pPrevBuffer;
        }

        // 找不到合适的位置，就需要再次分配了
        if (pResult == nullptr)
        {
            size = (size + 511) & (~511);
#ifdef _MEMORY_TRACE_
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + size + sizeof(int)));
            pResult->m_bUsedFlag = false;
            pResult->m_pNextBuffer = nullptr;
#else
            pResult = static_cast<PBUFFER>(malloc(sizeof(BUFFER) + size));
#endif
            pResult->m_pPrevBuffer = nullptr;
            pResult->m_size = size;
        }

        m_szAllocedSuperSize += size;
    }

#ifdef _MEMORY_TRACE_
    // 这里需要填写内存追踪的信息数据：填充fn和line、设置最后int位为标识位、将分配的结点尾插入到m_pLastAllocedBuffer
    if (pResult != nullptr)
    {
        assert(pResult->m_bUsedFlag == false);
        pResult->m_bUsedFlag = true;
        pResult->m_al.m_pFn = fn;
        pResult->m_al.m_iLine = line;
        pResult->m_fl.m_pFn = nullptr;
        pResult->m_fl.m_iLine = 0;
        pResult->m_iFreeTick = 0;
        // 取最后一个的int位置设置标识位
        *reinterpret_cast<int *>((reinterpret_cast<char *>(pResult + 1) + pResult->m_size)) = 0xCC55EE33;

        if (m_pLastAllocedBuffer != nullptr)
            m_pLastAllocedBuffer->m_pNextBuffer = pResult;

        // 尾插入到m_pLastAllocedBuffer所在的链表中
        pResult->m_pPrevBuffer = m_pLastAllocedBuffer;
        pResult->m_pNextBuffer = nullptr;
        m_pLastAllocedBuffer = pResult;

        m_allocTotal += pResult->m_size;
    }
#endif

    if (m_bMultThread == true)
        m_bufferLock.Unlock();
    // 返回的时候注意调整真正分配出去的内存指针位置
    return pResult != nullptr ? reinterpret_cast<void *>(pResult + 1) : nullptr;
}

#ifndef _MEMORY_TRACE_
void *ElsaCBufferAllocator::ReAllocBuffer(void *ptr, size_t new_size)
#else
void *ElsaCBufferAllocator::_ReAllocBuffer(void *ptr, size_t new_size, const char *fn, int)
#endif
{
    if (new_size == 0)
    {
        FreeBuffer(ptr);
        return nullptr;
    }

    PBUFFER pBuffer = nullptr;

    if (ptr)
    {
        pBuffer = static_cast<PBUFFER>(ptr) - 1; //  调整位置
        if (new_size <= pBuffer->m_size)
            return ptr;
    }

    void *pResult = nullptr;
#ifndef _MEMORY_TRACE_
    pResult = AllocBuffer(new_size);

    if (ptr)
    {
        memcpy(pResult, ptr, pBuffer->m_size);
        FreeBuffer(ptr);
    }
#else
    // 采用的方式是：先拷贝再删除
    pResult = _AllocBuffer(new_size, fn, 0);
    if (ptr)
    {
        memcpy(pResult, ptr, pBuffer->m_size);
        _FreeBuffer(ptr, fn, 0);
    }
#endif

    return pResult;
}

#ifndef _MEMORY_TRACE_
void ElsaCBufferAllocator::FreeBuffer(void *ptr)
#else
void ElsaCBufferAllocator::_FreeBuffer(void *ptr, const char *fn, int line)
#endif
{
    PBUFFER pBuffer = nullptr;
    pBuffer = static_cast<PBUFFER>(ptr) - 1; //  调整位置

    if (m_bMultThread == true)
        m_bufferLock.Lock();

#ifdef _MEMORY_TRACE_
    if (!pBuffer->m_bUsedFlag)
    {
        OutputMsg(rmError, (" %s try to free the memory that had freed before(%X,size:%d)，alloc file：%s(%d)，free file：%s(%d)\n"),
                  __FUNCTION__, pBuffer, pBuffer->m_size,
                  pBuffer->m_al.m_pFn, pBuffer->m_al.m_iLine,
                  pBuffer->m_fl.m_pFn, pBuffer->m_fl.m_iLine);
    }

    m_allocTotal -= pBuffer->m_size;
    assert(pBuffer->m_bUsedFlag);
    pBuffer->m_bUsedFlag = false;
    assert(*reinterpret_cast<int *>(reinterpret_cast<char *>(pBuffer + 1) + pBuffer->m_size) == (int)0xCC55EE33);

    // 将pbuffer从m_pLastAllocedBuffer所在的链表中移除，注意是双向链表哦
    if (pBuffer->m_pPrevBuffer)
        pBuffer->m_pPrevBuffer->m_pNextBuffer = pBuffer->m_pNextBuffer;
    if (pBuffer->m_pNextBuffer)
        pBuffer->m_pNextBuffer->m_pPrevBuffer = pBuffer->m_pPrevBuffer;
    // 如果删除的尾结点就是最后的结点就需要移动尾结点
    if (pBuffer == m_pLastAllocedBuffer)
        m_pLastAllocedBuffer = pBuffer->m_pPrevBuffer;

    pBuffer->m_pPrevBuffer = nullptr;
    pBuffer->m_pNextBuffer = nullptr;
    pBuffer->m_fl.m_pFn = fn;
    pBuffer->m_fl.m_iLine = line;
#endif

    // 设置结点过期时间
    pBuffer->m_iFreeTick = _getTickCount() + BufferFreeTick;

    // 将结点尾插入到链表中
    if (pBuffer->m_size == SmallBufferSize)
    {
        pBuffer->m_pPrevBuffer = m_pLastSmallBuffer;
        m_pLastSmallBuffer = pBuffer;
        m_szAllocedSmallSize -= SmallBufferSize;
        m_szFreedSmallSize += SmallBufferSize;
    }
    else if (pBuffer->m_size == MiddleBufferSize)
    {
        pBuffer->m_pPrevBuffer = m_pLastMiddleBuffer;
        m_pLastMiddleBuffer = pBuffer;
        m_szAllocedMiddleSize -= MiddleBufferSize;
        m_szFreedMiddelSize += MiddleBufferSize;
    }
    else if (pBuffer->m_size == LargeBufferSize)
    {
        pBuffer->m_pPrevBuffer = m_pLastLargeBuffer;
        m_pLastLargeBuffer = pBuffer;
        m_szAllocedLargeSize -= LargeBufferSize;
        m_szFreedLargeSize += LargeBufferSize;
    }
    else
    {
        pBuffer->m_pPrevBuffer = m_pLastSuperBuffer;
        m_pLastSuperBuffer = pBuffer;
        m_szAllocedSuperSize -= pBuffer->m_size;
        m_szFreedSuperSize += pBuffer->m_size;
    }

    if (m_bMultThread == true)
        m_bufferLock.Unlock();
}

void ElsaCBufferAllocator::GetMemoryInfo(ElsaCBufferAllocator::ALLOCATOR_MEMORY_INFO *pMemoryInfo)
{
    pMemoryInfo->m_smallBuffer.m_allocSize = m_szAllocedSmallSize;
    pMemoryInfo->m_smallBuffer.m_freeSize = m_szFreedSmallSize;
    pMemoryInfo->m_middleBuffer.m_allocSize = m_szAllocedMiddleSize;
    pMemoryInfo->m_middleBuffer.m_freeSize = m_szFreedMiddelSize;
    pMemoryInfo->m_largeBuffer.m_allocSize = m_szAllocedLargeSize;
    pMemoryInfo->m_largeBuffer.m_freeSize = m_szFreedLargeSize;
    pMemoryInfo->m_superBuffer.m_allocSize = m_szAllocedSuperSize;
    pMemoryInfo->m_superBuffer.m_freeSize = m_szFreedSuperSize;
}

void ElsaCBufferAllocator::CheckFreeBuffers()
{
    PBUFFER pBuffer = nullptr, pPrevBuffer = nullptr, pLastBuffer = nullptr;
    long long dwTick = _getTickCount();

    if (m_bMultThread == true)
        m_bufferLock.Lock();

    // 删除的时候结点的顺序分配 pPrevBuffer <- pBuffer <- pLastBuffer

    pBuffer = m_pLastSmallBuffer;
    pLastBuffer = nullptr;
    while (pBuffer)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        if (dwTick >= pBuffer->m_iFreeTick) //  超时释放
        {
            if (pBuffer == m_pLastSmallBuffer)
                m_pLastSmallBuffer = pPrevBuffer;
            if (pLastBuffer)
                pLastBuffer->m_pPrevBuffer = pPrevBuffer;

            m_szFreedSmallSize -= pBuffer->m_size;
            _Free(pBuffer);
        }
        else
            pLastBuffer = pBuffer;
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastMiddleBuffer;
    pLastBuffer = nullptr;
    while (pBuffer)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        if (dwTick >= pBuffer->m_iFreeTick) //  超时释放
        {
            if (pBuffer == m_pLastMiddleBuffer)
                m_pLastMiddleBuffer = pPrevBuffer;
            if (pLastBuffer)
                pLastBuffer->m_pPrevBuffer = pPrevBuffer;

            m_szFreedMiddelSize -= pBuffer->m_size;
            _Free(pBuffer);
        }
        else
            pLastBuffer = pBuffer;
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastLargeBuffer;
    pLastBuffer = nullptr;
    while (pBuffer)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        if (dwTick >= pBuffer->m_iFreeTick) //  超时释放
        {
            if (pBuffer == m_pLastLargeBuffer)
                m_pLastLargeBuffer = pPrevBuffer;
            if (pLastBuffer)
                pLastBuffer->m_pPrevBuffer = pPrevBuffer;

            m_szFreedLargeSize -= pBuffer->m_size;
            _Free(pBuffer);
        }
        else
            pLastBuffer = pBuffer;
        pBuffer = pPrevBuffer;
    }

    pBuffer = m_pLastSuperBuffer;
    pLastBuffer = nullptr;
    while (pBuffer)
    {
        pPrevBuffer = pBuffer->m_pPrevBuffer;
        if (dwTick >= pBuffer->m_iFreeTick) //  超时释放
        {
            if (pBuffer == m_pLastSuperBuffer)
                m_pLastSuperBuffer = pPrevBuffer;
            if (pLastBuffer)
                pLastBuffer->m_pPrevBuffer = pPrevBuffer;

            m_szFreedSuperSize -= pBuffer->m_size;
            _Free(pBuffer);
        }
        else
            pLastBuffer = pBuffer;
        pBuffer = pPrevBuffer;
    }

    if (m_bMultThread == true)
        m_bufferLock.Unlock();
}

void ElsaCBufferAllocator::SetMultiThread(bool flag)
{
    m_bMultThread = flag;
}

void ElsaCBufferAllocator::GC()
{
    CheckFreeBuffers();
}

#ifdef TEST_DEBUG
void ElsaCBufferAllocatorTestFunc()
{
    ElsaCBufferAllocator tm("ElsaCBufferAllocator1");
    ElsaCBufferAllocator::ALLOCATOR_MEMORY_INFO info;
    void *small_ptr1 = tm._AllocBuffer(64, __FUNCTION__, __LINE__);
    void *small_ptr2 = tm._AllocBuffer(64, __FUNCTION__, __LINE__);
    void *middle_ptr1 = tm._AllocBuffer(256, __FUNCTION__, __LINE__);
    void *middle_ptr2 = tm._AllocBuffer(256, __FUNCTION__, __LINE__);
    void *large_ptr1 = tm._AllocBuffer(1024, __FUNCTION__, __LINE__);
    void *large_ptr2 = tm._AllocBuffer(1024, __FUNCTION__, __LINE__);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tm.GetMemoryInfo(&info);
    printf("small: alloced: %d, free: %d\n",
           info.m_smallBuffer.m_allocSize, info.m_smallBuffer.m_freeSize);
    printf("middle: alloced: %d, free: %d\n",
           info.m_middleBuffer.m_allocSize, info.m_middleBuffer.m_freeSize);
    printf("large: alloced: %d, free: %d\n",
           info.m_largeBuffer.m_allocSize, info.m_largeBuffer.m_freeSize);
    printf("super: alloced: %d, free: %d\n",
           info.m_superBuffer.m_allocSize, info.m_superBuffer.m_freeSize);

    printf("\nAfter Free:\n");
    tm._FreeBuffer(small_ptr1, __FUNCTION__, __LINE__);
    tm._FreeBuffer(middle_ptr1, __FUNCTION__, __LINE__);
    tm._FreeBuffer(large_ptr1, __FUNCTION__, __LINE__);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tm.GetMemoryInfo(&info);
    printf("small: alloced: %d, free: %d\n",
           info.m_smallBuffer.m_allocSize, info.m_smallBuffer.m_freeSize);
    printf("middle: alloced: %d, free: %d\n",
           info.m_middleBuffer.m_allocSize, info.m_middleBuffer.m_freeSize);
    printf("large: alloced: %d, free: %d\n",
           info.m_largeBuffer.m_allocSize, info.m_largeBuffer.m_freeSize);
    printf("super: alloced: %d, free: %d\n",
           info.m_superBuffer.m_allocSize, info.m_superBuffer.m_freeSize);

    printf("\nAfter Reallocator:\n");
    large_ptr1 = tm._ReAllocBuffer(middle_ptr2, 1024, __FUNCTION__, __LINE__);
    middle_ptr1 = tm._ReAllocBuffer(small_ptr2, 256, __FUNCTION__, __LINE__);
    small_ptr1 = tm._ReAllocBuffer(large_ptr2, 64, __FUNCTION__, __LINE__);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tm.GetMemoryInfo(&info);
    printf("small: alloced: %d, free: %d\n",
           info.m_smallBuffer.m_allocSize, info.m_smallBuffer.m_freeSize);
    printf("middle: alloced: %d, free: %d\n",
           info.m_middleBuffer.m_allocSize, info.m_middleBuffer.m_freeSize);
    printf("large: alloced: %d, free: %d\n",
           info.m_largeBuffer.m_allocSize, info.m_largeBuffer.m_freeSize);
    printf("super: alloced: %d, free: %d\n",
           info.m_superBuffer.m_allocSize, info.m_superBuffer.m_freeSize);

    sleep(BufferFreeTick / 1000 + 4);
    tm.GC();
    printf("\nAfter GC:\n");
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tm.GetMemoryInfo(&info);
    printf("small: alloced: %d, free: %d\n",
           info.m_smallBuffer.m_allocSize, info.m_smallBuffer.m_freeSize);
    printf("middle: alloced: %d, free: %d\n",
           info.m_middleBuffer.m_allocSize, info.m_middleBuffer.m_freeSize);
    printf("large: alloced: %d, free: %d\n",
           info.m_largeBuffer.m_allocSize, info.m_largeBuffer.m_freeSize);
    printf("super: alloced: %d, free: %d\n",
           info.m_superBuffer.m_allocSize, info.m_superBuffer.m_freeSize);

    tm._FreeBuffer(small_ptr1, __FUNCTION__, __LINE__);
    tm._FreeBuffer(middle_ptr1, __FUNCTION__, __LINE__);
    tm._FreeBuffer(large_ptr1, __FUNCTION__, __LINE__);
}
#endif