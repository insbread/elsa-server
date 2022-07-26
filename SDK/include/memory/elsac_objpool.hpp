/***
 * @Author: insbread
 * @Date: 2022-07-19 15:49:19
 * @LastEditTime: 2022-07-19 15:49:20
 * @LastEditors: insbread
 * @Description: 内存分配器，只分配固定大小的类和结构体之类，和mempool的区别是会自动执行构造函数和析构函数
 * @FilePath: /elsa-server/SDK/include/memory/elsac_objpool.hpp
 * @版权声明
 */
#pragma once
#include "macro/os_def.h"
#include "memory/elsac_mempool.hpp"

// 针对结构体或者类的固定大小内存分配器，内存分配器，只分配固定大小的类和结构体之类，和mempool的区别是会自动执行构造函数和析构函数
template <typename T, int ONE_TIME_COUNT = 1024>
class ElsaCObjPool : public ElsaCMemPool<sizeof(T), ONE_TIME_COUNT>
{
    typedef ElsaCMemPool<sizeof(T), ONE_TIME_COUNT> Inherited;

public:
    ElsaCObjPool(const char *namestr) : Inherited(namestr) {}

#ifndef _MEMORY_TRACE_
    T *Alloc()
#else
    T *_Alloc(const char *fn, int line)
#endif
    {
#ifndef _MEMORY_TRACE_
        T *result = static_cast<T *>(Inherited::Alloc());
#else
        T *result = static_cast<T *>(Inherited::_Alloc(fn, line));
#endif
        new (result) T(); //  定位new
        return result;
    }

#ifndef _MEMORY_TRACE_
    void Free(T *ptr)
    {
        if (ptr == nullptr)
            return;
        ptr->~T();
        Inherited::Free(ptr);
    }
#else
    void _Free(T *ptr, const char *fn, int line)
    {
        if (ptr == nullptr)
            return;
        ptr->~T();
        Inherited::_Free(ptr, fn, line);
    }
#endif
};