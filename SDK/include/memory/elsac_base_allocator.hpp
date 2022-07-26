/***
 * @Author: insbread
 * @Date: 2022-07-15 16:26:39
 * @LastEditTime: 2022-07-15 16:26:39
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/memory/elsac_base_allocator.hpp
 * @版权声明
 */

#pragma once

#include "macro/os_def.h"
#include "memory/elsac_memory_counter.hpp"

#ifdef _MEMORY_TRACE_
#define AllocBuffer(count_) _AllocBuffer(count_, __FILE__, __LINE__)
#define ReAllocBuffer(src, count_) _ReAllocBuffer(src, count_, __FILE__, __LINE__)
#define FreeBuffer(ptr) _FreeBuffer(ptr, __FILE__, __LINE__)
#endif

/*
    基本分配器，采用crt内存分配方式，内存分配器的基类，其他分配器可继承其接口，实现不同的内存分配算法。
    1. 分配格式：[size_t 内存大小 | 真实内存数据]；多个内存块之间没有任何关系，逻辑和物理上都不一定会相邻
    2. 分配出去的是真实内存数据部分，前面一部分在他人看来是不可见的。
    3. 由于继承自JieCAllocatorCounterItem，创建的会是会自动注册到内存管理器JieCMemory中
    内部处理的时候需要调整指针的真实位置获取开始位置和内存大小。
*/
class ElsaCBaseAllocator : public ElsaCAllocatorCounterItem
{
public:
    ElsaCBaseAllocator(const char *namestr) : ElsaCAllocatorCounterItem(namestr) {}
    virtual ~ElsaCBaseAllocator() {}

    void SetMultiThread(bool) {}
    void GC() {}

public:
#ifndef _MEMORY_TRACE_
    virtual void *AllocBuffer(size_t count)
#else
    // 分配格式：[size_t 内存大小 | 真实内存数据]，返回指向真实内存数据的指针
    virtual void *_AllocBuffer(size_t count, const char *, int)
#endif
    {
        size_t *result = static_cast<size_t *>(malloc(count + sizeof(size_t)));
#ifdef _MEMORY_TRACE_
        if (result != nullptr)
        {
            m_allocTotal += count;
            *result = count;
        }
#endif
        return reinterpret_cast<void *>(result + 1);
    }

#ifndef _MEMORY_TRACE_
    virtual void *ReAllocBuffer(void *src, size_t count)
#else
    virtual void *_ReAllocBuffer(void *src, size_t count, const char *, int)
#endif
    {
#ifdef _MEMORY_TRACE_
        if (src)
        {
            size_t old = *(static_cast<size_t *>(src) - 1);
            m_allocTotal -= old;
            src = reinterpret_cast<void *>(static_cast<size_t *>(src) - 1); //  调整指针
        }
        m_allocTotal += count;
#endif

        size_t *result = static_cast<size_t *>(realloc(src, count + sizeof(size_t)));
        *result = count;

        return reinterpret_cast<void *>(result + 1);
    }

#ifndef _MEMORY_TRACE_
    virtual void FreeBuffer(void *ptr)
#else
    virtual void _FreeBuffer(void *ptr, const char *, int)
#endif
    {
        if (ptr == nullptr)
            return;
#ifdef _MEMORY_TRACE_
        size_t old = *(static_cast<size_t *>(ptr) - 1);
        m_allocTotal -= old;
#endif
        free(reinterpret_cast<void *>(static_cast<size_t *>(ptr) - 1));
    }
};

#ifdef TEST_DEBUG
inline void ElsaCBaseAllocatorTestFunc()
{
    ElsaCBaseAllocator tmp1("test1");
    ElsaCBaseAllocator tmp2("test2");

    void *tmp_ptr1_1 = tmp1.AllocBuffer(1000);
    void *tmp_ptr1_2 = tmp1.AllocBuffer(1300);
    void *tmp_ptr2_1 = tmp2.AllocBuffer(100);
    void *tmp_ptr2_2 = tmp2.AllocBuffer(140);

    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp1.FreeBuffer(tmp_ptr1_1);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp2.FreeBuffer(tmp_ptr2_2);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
    tmp1.FreeBuffer(tmp_ptr1_2);
    tmp2.FreeBuffer(tmp_ptr2_1);
    ElsaCMemoryCounter::GetSingleton().LogToFile();
}
#endif