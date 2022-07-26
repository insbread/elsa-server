/***
 * @Author: insbread
 * @Date: 2022-07-18 21:50:05
 * @LastEditTime: 2022-07-18 21:50:05
 * @LastEditors: insbread
 * @Description: 内存分配器，只分配固定大小的内存，适用于频繁创建实例的类，结构体之类
 * @FilePath: /elsa-server/SDK/include/memory/elsac_mempool.hpp
 * @版权声明
 */
#pragma once

#include "memory/elsa_allocdef.h"
#include "memory/elsac_memory_counter.hpp"
#include "container/elsac_vector.h"

/*
    分配大小固定的内存，适用于频繁创建的类或者结构体。
    内存的格式：
    mdata_block-> [DataBlock块信息 | {AllocBuffHead | BUFF_SIZE} | {AllocBuffHead | BUFF_SIZE} | ... | {AllocBuffHead | BUFF_SIZE}]
               -> [DataBlock块信息 | {AllocBuffHead | BUFF_SIZE} | {AllocBuffHead | BUFF_SIZE} | ... | {AllocBuffHead | BUFF_SIZE}]
    1. 分配新内存的时候以一个DataBlock块为单位进行分配，每个块包含一个DataBlock块信息和多个{AllocBuffHead | BUFF_SIZE}结点。真正分配出去的是BUFF_SIZE部分的内存；
    2. 空闲队列的单位则是以{AllocBuffHead | BUFF_SIZE}为单位进行管理和保存，请求和回收的单位也是以{AllocBuffHead | BUFF_SIZE}为单位进行管理。
        空闲队列：vec = [&{AllocBuffHead | BUFF_SIZE}, &{AllocBuffHead | BUFF_SIZE}, ..., &{AllocBuffHead | BUFF_SIZE}]；
    3. 分配流程：先从空闲队列中分配搜索是否存在可以分配的{AllocBuffHead | BUFF_SIZE}，否则就在以分配一个DataBlock块。
    3. 每个分配的结点大小在类实例化之后就是固定的，因此适用于某些特定频繁创建和回收的实例的类或结构体
*/
template <int BUFF_SIZE, int ONE_TIME_COUNT = 1024>
class ElsaCMemPool : public ElsaCAllocatorCounterItem
{
public:
    typedef ElsaCAllocatorCounterItem Inherited;

private:
    struct ElsaSDataBlock
    {
        ElsaSDataBlock *m_pPrev;
    } * m_pDataPtr; // 记录所有分配出去的数据，以一个block为单位

protected:
    ElsaNContainer::ElsaCVector<void *> m_vecFreeList; // 空闲队列，记录所有空闲可分配空间，以AllocBufferHead为单位
    int m_iAllSize;
#ifdef _MEMORY_TRACE_
    ElsaNContainer::ElsaCVector<void *> m_vecAllList; // 整体队列，记录所有空闲，以AllocBufferHead为单位
#endif
public:
    ElsaCMemPool(const char *namestr) : Inherited(namestr)
    {
        m_pDataPtr = nullptr;
        m_iAllSize = 0;
    }

    ~ElsaCMemPool()
    {
#ifdef _MEMORY_TRACE_
        // 对每一个数据做一个检测，是否已经释放了
        for (int i = 0; i < m_vecAllList.Size(); i++)
        {
            AllocBuffHead *hdr = static_cast<AllocBuffHead *>(m_vecAllList[i]);

            if (hdr->m_bUsingFlag)
            {
                char err[1024];
                SNPRINTFA(err, sizeof(err) - 1, " %scheck memory leaks(size:%d),alloc：%s(%d)\n",
                          __FUNCTION__, (int)hdr->m_buffSize, hdr->m_al.m_cpFn, hdr->m_al.m_iLine);
                OutputWatchFile(err, "err.log");
            }
        }
#endif
        ElsaSDataBlock *block_ptr = m_pDataPtr;
        ElsaSDataBlock *prev = nullptr;
        int block_count = 0;
        while (block_ptr != nullptr)
        {
            prev = block_ptr->m_pPrev;
            free(block_ptr);
            block_ptr = prev;
            block_count++;
        }

        m_pDataPtr = nullptr;

#ifdef _MEMORY_TRACE_
        // 计算空闲区间的数量，检测是否出现内存泄露
        size_t free_size = m_vecFreeList.Count() * (BUFF_SIZE + sizeof(AllocBuffHead)) + block_count * sizeof(ElsaSDataBlock);
        if (free_size != m_allocTotal)
        {
            char err[1024];
            SNPRINTFA(err, sizeof(err) - 1, "%s:%s:%d memory error,free size:%lld, alloc_size:%lld\n",
                      __FUNCTION__, __FILE__, __LINE__, (long long int)free_size, (long long int)m_allocTotal);
            OutputWatchFile(err, "err.log");
        }
#endif
    }

#ifndef _MEMORY_TRACE_
    void *Alloc()
#else
#define Alloc() _Alloc(__FILE__, __LINE__)
#define Free(ptr) _Free(ptr, __FILE__, __LINE__)
    void *_Alloc(const char *file_name, int line)
#endif
    {
        void *result = nullptr;
        int count = m_vecFreeList.Count();

        if (count <= 0)
        {
            count = m_iAllSize > 0 ? 2 * m_iAllSize : ONE_TIME_COUNT;
            m_iAllSize += count;
#ifndef _MEMORY_TRACE_
            size_t allocsize = sizeof(ElsaSDataBlock) + BUFF_SIZE * count;
#else
            size_t allocsize = sizeof(ElsaSDataBlock) + (BUFF_SIZE + sizeof(AllocBuffHead)) * count;
#endif
            ElsaSDataBlock *block_ptr = static_cast<ElsaSDataBlock *>(malloc(allocsize));
#ifdef _MEMORY_TRACE_
            m_allocTotal += allocsize;
#endif
            // 将新分配的数据插入到空闲链表中
            result = reinterpret_cast<void *>(block_ptr + 1); //  跳过数据头
            if (m_vecFreeList.MaxSize() < m_iAllSize)
                m_vecFreeList.reverse(m_iAllSize);
            void **list = m_vecFreeList;
            for (int i = 0; i < count; i++)
            {
                list[i] = result; //  为了保证适合用在类上面，需要采用赋值函数而不是memcpy这种函数
#ifdef _MEMORY_TRACE_
                AllocBuffHead *hdr = static_cast<AllocBuffHead *>(result);
                hdr->m_bUsingFlag = false;
                hdr->m_buffSize = BUFF_SIZE;
                result = reinterpret_cast<void *>(static_cast<char *>(result) + BUFF_SIZE + sizeof(AllocBuffHead)); //  跳到下一个结点
#else
                result = reinterpret_cast<void *>(static_cast<char *>(result) + BUFF_SIZE);
#endif
            }

#ifdef _MEMORY_TRACE_
            // 开启内存追踪的条件下，将所有的分配项记录下来，用作检测使用
            m_vecAllList.AddArray(m_vecFreeList, count);
#endif
            // 空闲链表的尾插法
            block_ptr->m_pPrev = m_pDataPtr;
            m_pDataPtr = block_ptr;
        }

        // 返回空闲链表中的最后一个元素
        count--;
        result = m_vecFreeList[count];
        m_vecFreeList.Trunc(count);
#ifdef _MEMORY_TRACE_
        m_usedCount += BUFF_SIZE;
        AllocBuffHead *hdr = static_cast<AllocBuffHead *>(result);
        hdr->m_bUsingFlag = true;
        hdr->m_al.m_cpFn = file_name;
        hdr->m_al.m_iLine = line;
        result = reinterpret_cast<void *>(static_cast<AllocBuffHead *>(result) + 1);
#endif
        return result;
    }

#ifndef _MEMORY_TRACE_
    void Free(void *ptr)
#else
    void _Free(void *ptr, const char *file_name, int line)
#endif
    {
        if (ptr == nullptr)
            return;

#ifdef _MEMORY_TRACE_
        AllocBuffHead *hdr = static_cast<AllocBuffHead *>(ptr) - 1;
        if (hdr->m_bUsingFlag == false)
        {
            char err[1024];
            SNPRINTFA(err, sizeof(err) - 1, " %s Free the buff that had be release before(size:%d),alloc：%s(%d),free：%s(%d),now:%s(%d)\n",
                      __FUNCTION__, (int)hdr->m_buffSize, hdr->m_al.m_cpFn, hdr->m_al.m_iLine,
                      hdr->m_fl.m_cpFn, hdr->m_fl.m_iLine, file_name, line);
            OutputWatchFile(err, "err.log");
            return;
        }

        hdr->m_bUsingFlag = false;
        hdr->m_fl.m_cpFn = file_name;
        hdr->m_fl.m_iLine = line;
        ptr = hdr;
#endif
        // 放入空闲位置，这里没有回收这个内存块
        m_vecFreeList.Add(ptr);
#ifdef _MEMORY_TRACE_
        m_usedCount -= BUFF_SIZE;
#endif
    }

};

#ifdef TEST_DEBUG
class Student
{
private:
    char *name;
    int stu_no;
    int major_no;

public:
    Student()
    {
        name = new char[100];
        memset(name, 0, sizeof(char) * 100);
        stu_no = 0;
        major_no = 0;
    }
    ~Student()
    {
        delete[] name;
        name = nullptr;
    }

    void Show()
    {
        printf("name:%s -- stuno:%d -- majorno:%d\n", name, stu_no, major_no);
    }
};
inline void ElsaCMemPoolTest()
{
    ElsaCMemPool<sizeof(Student)> tm("ElsaCMemPool");
    Student *ptr = static_cast<Student *>(tm._Alloc(__FILE__, __LINE__));
    ptr->Show();
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tm._Free(ptr, __FILE__, __LINE__);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    return;
}
#endif