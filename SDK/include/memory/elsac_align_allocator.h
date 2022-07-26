/***
 * @Author: insbread
 * @Date: 2022-07-15 20:14:27
 * @LastEditTime: 2022-07-15 20:14:27
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/memory/elsac_align_allocator.h
 * @版权声明
 */

#pragma once

#include "elsac_x_lock.h"
#include "elsac_base_allocator.hpp"

using namespace ElsaNLock;

#define GetAlignIdx(t) (t % m_iAlign == 0 ? (t / m_iAlign - 1) : (t / m_iAlign))

/*
    简单分离对齐内存分配器。
    这个分配器以 简单分离算法 和 对齐 为原则指导设计的内存分配器。对齐必须是8的倍数，因为64位操作系统默认是8字节对齐的，8字节对齐能满足各种计算架构和系统。
    简单分离是指保存多个链表，每个链表的结点都是分配相同大小的外嵌结点，对齐是指结点的大小必须是align的倍数。
    整的来说：每一级idx链表，该链表中所有结点物理分配的内存大小是idx * align。
    结点的分配格式：
    mhead[i]->[FreeNode | 真正内存: i * align 字节]   [FreeNode | 真正内存：i * align 字节]
               |                                        |
               -----------------------------------------
    mhead[i + 1]->[FreeNode | 真正内存: (i + 1) * align 字节]   [FreeNode | 真正内存：(i + 1) * align 字节]
                    |                                             |
                    ----------------------------------------------
*/
class ElsaCAlignAllocator : public ElsaCBaseAllocator
{
private:
    struct ElsaSFreeNode
    {
        ElsaSFreeNode *m_pNext; //  指向下一个Free结点
        int m_iSize;
    };
    struct ElsaSHeader //  头结点
    {
        ElsaSFreeNode *m_pFirst; // 指向Free链表的头结点
        int m_iTotal;            // 总的结点个数
        int m_iFree;             // 空闲节点个数
        ElsaSHeader()
        {
            m_iTotal = m_iFree = 0;
            m_pFirst = nullptr;
        }
    };

private:
    int m_iMaxSize;         // 最大分配的内存容量
    int m_iAlign;           // 每一次分配的内存容量
    ElsaSHeader *m_pHeader; // 内存等级链表结点
    int m_iHdrCnt;          // 结点个数
    int m_iRest;            // 保留的空闲节点个数，GC功能每次回收Free节点时，至少保存m_iRest个结点，如果不设置就是保留总数的0.01个结点

    bool m_bMultThread;      // 支持加锁，可以多线程安全
    ElsaCMutex m_bufferLock; //  多线程锁

public:
    ElsaCAlignAllocator(const char *name, int max_size, int align);
    virtual ~ElsaCAlignAllocator();

public:
#ifndef _MEMORY_TRACE_
    virtual void *AllocBuffer(size_t count);
    virtual void *ReAllocBuffer(void *src, size_t count);
    virtual void FreeBuffer(void *ptr);
#else
    // 分配内存，根据count所在的等级进行分配，分配的时候会进行对齐分配，即每一次分配的内存总是align的倍数，
    //	但是如果要求分配的count大于max_size，那么就直接分配不进行对齐操作。
    virtual void *_AllocBuffer(size_t count, const char *, int);
    //	重新为一块内存进行分配。主要思路是按照newsize进行分配，然后进行数据拷贝；注意，如果newsize比src可用内存还小，会导致assert异常的
    //	然后将src的内存进行_FreeBuffer操作
    virtual void *_ReAllocBuffer(void *src, size_t newsize, const char *fn, int line);
    //	对ptr进行释放操作，注意如果是非对齐分配的内存会直接free，否则会放回heads_中
    virtual void _FreeBuffer(void *ptr, const char *, int);
#endif

    void Init(int max_size, int align);
    void GC();
    void SetMultiThread(bool flag) { m_bMultThread = flag; }
    void SetGCRest(int rest) { m_iRest = rest; }
};

#ifdef TEST_DEBUG
inline void ElsaCAlignAllocatorTestFunc()
{
    ElsaCAlignAllocator tmp("ElsaCAlignAllocator", 2048, 8);

    void *t1 = tmp.AllocBuffer(1000);
    void *t2 = tmp.AllocBuffer(300);
    void *t3 = tmp.AllocBuffer(1500);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    t1 = tmp.ReAllocBuffer(t1, 1200);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    t2 = tmp.ReAllocBuffer(t2, 400);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp.FreeBuffer(t1);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp.FreeBuffer(t2);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp.FreeBuffer(t3);
}
#endif