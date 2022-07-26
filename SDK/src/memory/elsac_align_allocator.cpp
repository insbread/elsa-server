/*
 * @Author: insbread
 * @Date: 2022-07-15 20:22:15
 * @LastEditTime: 2022-07-15 20:22:15
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/memory/elsac_align_allocator.cpp
 * @版权声明
 */

#include "memory/elsac_align_allocator.h"

ElsaCAlignAllocator::ElsaCAlignAllocator(const char *name, int max_size, int align) : ElsaCBaseAllocator(name)
{
    m_bMultThread = false;
    m_pHeader = nullptr;
    Init(max_size, align);
}

ElsaCAlignAllocator::~ElsaCAlignAllocator()
{
    for (int i = 0; i < m_iHdrCnt; i++)
    {
        ElsaSHeader &hdr = m_pHeader[i];
        assert(hdr.m_iTotal == hdr.m_iFree);
        ElsaSFreeNode *node = hdr.m_pFirst;
        while (node != nullptr)
        {
#ifdef _MEMORY_TRACE_
            m_allocTotal -= ((i + 1) * m_iAlign);
#endif
            hdr.m_pFirst = node->m_pNext;
            free(node);
            node = hdr.m_pFirst;
        }

        m_pHeader[i].m_iTotal = 0;
    }

    delete[] m_pHeader;
}

void ElsaCAlignAllocator::Init(int max_size, int align)
{
    // 要求最大的内存分配量和对齐量必须是8的倍数
    assert(max_size % 8 == 0 && align % 8 == 0);

    m_iMaxSize = max_size;
    m_iAlign = align;

    m_iHdrCnt = m_iMaxSize / m_iAlign; //  每一个等级i的链表保存的大小是i * m_iAlign
    m_pHeader = new ElsaSHeader[m_iHdrCnt];
    m_iRest = 0;
}

#ifndef _MEMORY_TRACE_
void *ElsaCAlignAllocator::AllocBuffer(size_t count)
#else
void *ElsaCAlignAllocator::_AllocBuffer(size_t count, const char *, int)
#endif
{
    assert(count > 0);

    if (m_bMultThread == true)
        m_bufferLock.Lock();

    ElsaSFreeNode *node = nullptr;
    if ((int)count <= m_iMaxSize)
    {
        // 如果内存大小在范围内，则查看是否存在剩余的空闲节点用作分配
        int idx = GetAlignIdx((int)count);
        ElsaSHeader &hdr = m_pHeader[idx];

        if (hdr.m_pFirst != nullptr)
        {
            //	如果存在空闲节点，就将hdr中的Free结点分配出去
            node = hdr.m_pFirst;
            hdr.m_pFirst = node->m_pNext;
            hdr.m_iFree--;
        }
        else
        {
            // 如果当前hdr中所在的内存级别没有空闲链表，就创建新的结点分配出去，
            int newSize = (idx + 1) * m_iAlign;
#ifdef _MEMORY_TRACE_
            m_allocTotal += newSize;
#endif
            ElsaSHeader &hdr = m_pHeader[idx];
            hdr.m_iTotal++;

            node = static_cast<ElsaSFreeNode *>(malloc(newSize + sizeof(ElsaSFreeNode)));
        }
    }
    else
    {
        // 大内存则直接分配，这里效率会很低，因为没有实现对齐分配。
#ifdef _MEMORY_TRACE_
        m_allocTotal += count;
#endif
        node = static_cast<ElsaSFreeNode *>(malloc(count + sizeof(ElsaSFreeNode)));
    }

    node->m_iSize = int(count);

    if (m_bMultThread)
        m_bufferLock.Unlock();
    return reinterpret_cast<void *>(node + 1);
}

#ifndef _MEMORY_TRACE_
void *ElsaCAlignAllocator::ReAllocBuffer(void *src, size_t count)
#else
void *ElsaCAlignAllocator::_ReAllocBuffer(void *src, size_t newsize, const char *fn, int line)
#endif
{
    if (newsize == 0)
    {
#ifndef _MEMORY_TRACE_
        FreeBuff(src);
#else
        _FreeBuffer(src, fn, line);
#endif
        return nullptr;
    }

    if (src == nullptr)
    {
#ifndef _MEMORY_TRACE_
        return AllocBuffer(newsize);
#else
        return _AllocBuffer(newsize, fn, line);
#endif
    }

    ElsaSFreeNode *node = reinterpret_cast<ElsaSFreeNode *>(static_cast<char *>(src) - sizeof(ElsaSFreeNode)); //  调整指针到正确位置
    int oldsize = node->m_iSize;
    if (oldsize >= (int)newsize)
        return src;

    // 大结点的内存无法实现对齐分配，直接进行分配和回收即可
    if (oldsize > m_iMaxSize)
    {
        if ((int)newsize > m_iMaxSize)
        {
#ifdef _MEMORY_TRACE_
            m_allocTotal += (newsize - oldsize);
#endif
            node = static_cast<ElsaSFreeNode *>(realloc(node, newsize + sizeof(ElsaSFreeNode)));
            node->m_iSize = newsize;
            return reinterpret_cast<void *>(node + 1);
        }
        else
        {
            assert(false);
        }
    }

    // 符合对齐的结点，则借助Alloc和Free以及memcpy进行内存的转移和再分配
    void *retPtr = src;
    assert((int)newsize > oldsize);

    int oldidx = GetAlignIdx(oldsize);
    int newidx = GetAlignIdx((int)newsize);
    if (oldidx == newidx) //  同一等级不需要分配
    {
        node->m_iSize = (int)newsize;
    }
    else // 不同等级则需要分配、拷贝、释放
    {
        void *ret = _AllocBuffer(newsize, fn, line);

        if (src != nullptr)
        {
            memcpy(ret, src, oldsize);
#ifndef _MEMORY_TRACE_
            FreeBuff(src);
#else
            _FreeBuffer(src, fn, line);
#endif
        }
        retPtr = ret;
    }

    return retPtr;
}

#ifndef _MEMORY_TRACE_
void ElsaCAlignAllocator::FreeBuffer(void *ptr)
#else
void ElsaCAlignAllocator::_FreeBuffer(void *ptr, const char *, int)
#endif
{
    if (ptr == nullptr)
        return;
    if (m_bMultThread == true)
        m_bufferLock.Lock();

    ElsaSFreeNode *node = reinterpret_cast<ElsaSFreeNode *>(ptr) - 1;
    int size = node->m_iSize;
    if (size > m_iMaxSize)
    {
#ifdef _MEMORY_TRACE_
        m_allocTotal -= size;
#endif
        free(node);
    }
    else
    {
        int idx = GetAlignIdx(size);
        ElsaSHeader &hdr = m_pHeader[idx];
        node->m_pNext = hdr.m_pFirst;
        hdr.m_pFirst = node->m_pNext;
        hdr.m_iFree++;
    }
    if (m_bMultThread == true)
        m_bufferLock.Unlock();
}

void ElsaCAlignAllocator::GC()
{
    if (m_bMultThread == true)
        m_bufferLock.Lock();

    for (int i = 0; i < m_iHdrCnt; i++)
    {
        ElsaSHeader &hdr = m_pHeader[i];
        int rest = m_iRest != 0 ? m_iRest : (int)(hdr.m_iTotal * 0.01); //  设置保存的空闲量

        if (hdr.m_iFree > rest)
        {
            int freeCnt = hdr.m_iFree - rest;
            ElsaSFreeNode *node = hdr.m_pFirst;
            int cnt = 0;
            while (node != nullptr && cnt < freeCnt) // 注意这里cnt是从0开始，所以是<，如果是从1开始就是<=了
            {
#ifdef _MEMORY_TRACE_
                m_allocTotal -= ((i + 1) * m_iAlign);
#endif
                hdr.m_pFirst = node->m_pNext;
                free(node);
                node = hdr.m_pFirst;
                cnt++;
            }
            // hdr.m_iFree = rest;
            // hdr.m_iTotal -= freeCnt;
            hdr.m_iFree -= cnt; //  这里用cnt会更加合适，因为可能node=nullptr
            hdr.m_iTotal -= cnt;
        }
    }

    if (m_bMultThread == true)
        m_bufferLock.Unlock();
}