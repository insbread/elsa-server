/***
 * @Author: insbread
 * @Date: 2022-07-17 14:59:14
 * @LastEditTime: 2022-07-17 14:59:14
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/memory/elsa_buffer_allocator.h
 * @版权声明
 */
#pragma once

#include "macro/os_def.h"
#include "memory/elsac_base_allocator.hpp"
#include "elsac_x_lock.h"

using namespace ElsaNLock;

/*
    简单分离内存分配器。
    分配器以简单分离算法为指导进行设计。共有四个大小类，2^6,2^7,2^8,以及>2^8。
    采用空闲链表进行管理，结点的嵌入方式是外部嵌入。链表管理方法是尾插入和尾分配。
    内存链表结构：
    mlast_small_buffer: [ElsaSTagBuffer | 真实内存：2^6字节 | int]   [ElsaSTagBuffer | 真实内存：2^6字节 | int]
                         |                                               |
                         ------------------------------------------------
    mlast_middle_buffer: [ElsaSTagBuffer | 真实内存：2^7字节 | int]   [ElsaSTagBuffer | 真实内存：2^7字节 | int]
                         |                                              |
                         -----------------------------------------------
    注意：对于>2^8的结点分配需要遍历整个空闲链表查找是否由够大的结点进行分配，所以分配的速度是最慢的O(n);
         其他的结点由于大小限定，所以能够在O(1)分配；

    该内存分配器采用定期释放内存空间的功能，每个节点进入空闲队列时都有一个过期时间
*/
class ElsaCBufferAllocator : public ElsaCBaseAllocator
{
public:
    static const unsigned int SmallBufferSize = 64;
    static const unsigned int MiddleBufferSize = 256;
    static const unsigned int LargeBufferSize = 1024;
#pragma pack(push, 1)
    /*
        分配内存的结点，内存分配使用链表管理分配的内存。
        每个分配的内存结点就是采用tagBuffer外嵌入到内存中实现管理。
        一个结点的内存的物理结构：[BUFFER | size of bytes | int];
        此时BUFFER用于连接前面的物理内存结构。
    */
    typedef struct ElsaSTagBuffer
    {
        size_t m_size;                 //内存数据段大小
        ElsaSTagBuffer *m_pPrevBuffer; //指向上一个内存块的指针
        int64_t m_iFreeTick;           //即将被销毁的时间
#ifdef _MEMORY_TRACE_
        // 开启内存最终时候，需要使用的额外参数
        ElsaSTagBuffer *m_pNextBuffer;
        struct
        {
            const char *m_pFn; //申请内存块的代码位置，用于调试
            int m_iLine;
        } m_al, m_fl;
        bool m_bUsedFlag; //是否使用的标记
#endif
    } BUFFER, *PBUFFER;
#pragma pack(pop)

    //  内存块分配的使用情况
    struct ElsaSBufferStatic
    {
        size_t m_allocSize; //  已分配大小
        size_t m_freeSize;  //  空闲块大小
    };
    //  各个内存块使用的情况
    typedef struct ElsaSTagAllocatorMemoryInfo
    {
        ElsaSBufferStatic m_smallBuffer;
        ElsaSBufferStatic m_middleBuffer;
        ElsaSBufferStatic m_largeBuffer;
        ElsaSBufferStatic m_superBuffer;
    } ALLOCATOR_MEMORY_INFO;

private:
    ElsaCMutex m_bufferLock;     //内存块申请锁
    PBUFFER m_pLastSmallBuffer;  //指向最后一个空闲的小型内存块的指针
    PBUFFER m_pLastMiddleBuffer; //指向最后一个空闲的中型内存块的指针
    PBUFFER m_pLastLargeBuffer;  //指向最后一个空闲的大型内存块的指针
    PBUFFER m_pLastSuperBuffer;  //指向最后一个空闲的超级内存块的指针
#ifdef _MEMORY_TRACE_
    //指向最后一个被申请的内存块，这个链表是通过next_ptr这个结点进行链接在一起，采用的是尾结点管理方法
    PBUFFER m_pLastAllocedBuffer;
#endif
    size_t m_szAllocedSmallSize;  //已被申请的小型内存块大小
    size_t m_szAllocedMiddleSize; //已被申请的中型内存块大小
    size_t m_szAllocedLargeSize;  //已被申请的大型内存块大小
    size_t m_szAllocedSuperSize;  //已被申请的超级内存块大小
    size_t m_szFreedSmallSize;    //已被释放的小型内存块大小
    size_t m_szFreedMiddelSize;   //已被释放的中型内存块大小
    size_t m_szFreedLargeSize;    //已被释放的大型内存块大小
    size_t m_szFreedSuperSize;    //已被释放的超级内存块大小

    bool m_bMultThread; // 是否用于多线程中，是的话对于某些操作需要上锁

public:
    ElsaCBufferAllocator(const char *namestr);
    ~ElsaCBufferAllocator();

#ifndef _MEMORY_TRACE_
    void *AllocBuffer(size_t size);
    void *ReAllocBuffer(void *ptr, size_t new_size);
    void FreeBuffer(void *ptr);
#else
    /*** 分配的流程：从空闲链表的末尾找出结点进行分配；没有则请求内存进行分配；内存分配器的分配内存的物理结构:
     * BUFFER | size of bytes | int	;
     * 前者是记录整个内存的信息的结构体，中间是真正返回可以被外部使用的内存块，后面则是记录整个内存的大小；
     * BUFFER也会记录大小，但是只会记录后面可使用内存的大小；
     * BUFFER本身是一个类似链表结点的结构，只能连接前面的结点(连接后面的结点是为了调试)
     * @param {size_t} size
     * @param {char*} fn
     * @param {int} line
     * @return {*}
     * @use:
     */
    void *_AllocBuffer(size_t size, const char *fn, int line);
    /***
     *  整体思路就是：先分配空间，将ptr原来的数据拷贝到新的空间，然后释放原空间，将新空间返回
     * @param {void*} ptr 要重新分配的空间原来的数据
     * @param {size_t} new_size 新空间的大小
     * @param {char*} fn debuf下的调试信息
     * @param {int} line
     * @return {void*} 新分配的空间起始位置
     * @use:
     */
    void *_ReAllocBuffer(void *ptr, size_t new_size, const char *fn, int line);
    /***
     *  将ptr从已分配链表中放入到空闲链表中，并设置该节点的过期时间
     * @param {void*} ptr
     * @param {char*} fn
     * @param {int} line
     * @return {*}
     * @use:
     */
    void _FreeBuffer(void *ptr, const char *fn, int line);
#endif

    void GetMemoryInfo(ElsaCBufferAllocator::ALLOCATOR_MEMORY_INFO *pMemoryInfo);
    // 销毁所有过期的结点。
    // 检测所有的空闲链表，对于每一个空闲结点PBUFFER，如果当前时间超过其成员时间free_tick_，则free销毁
    void CheckFreeBuffers();
    void SetMultiThread(bool flag);
    //	定期回收函数，就是直接调用了CheckFreeBuffers
    void GC();
};

#ifdef TEST_DEBUG
void ElsaCBufferAllocatorTestFunc();
#endif