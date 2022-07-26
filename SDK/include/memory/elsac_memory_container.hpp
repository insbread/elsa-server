/***
 * @Author: insbread
 * @Date: 2022-07-15 17:03:13
 * @LastEditTime: 2022-07-15 17:03:13
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/memory/elsac_memory_container.hpp
 * @版权声明
 */

#include "macro/os_def.h"
#include "memory/elsac_memory_counter.hpp"

/*
    一次性的内存分配器。适用于一次分配内存后，直到程序结束才释放内存的地方，如配置文件的读取。
    好处是：不提供释放内存的函数，因此不需要一个个内存手工释放，而是由分配器一次释放。采用尾链表管理方法。

    内存分配格式：
    [DataBlock | T | T | T | T | ... | T ] <- ... <- [DataBlock | T | T | T | T | ... | T] <- mdata_block;
    注意每一块[DataBlock | T | T | T | T | ... | T]是不定长度的，最小的长度是ONE_TIME_COUNT，每次可以从这个单位中分配若干个T出去。
    会有一个free_ptr记录当前单位分配的偏移量，每次请求的时候，都会倒序遍历这个mdata_block，
    找到一个单元的空闲足够容纳count个请求元素为止，如果找不到就申请新的单位。

    不提供回收的机制，尽管有Free函数，但是不建议使用。
*/
template <typename T, int ONE_TIME_COUNT = 1024>
class ElsaCMemoryContainer : public ElsaCAllocatorCounterItem
{
public:
    typedef ElsaCAllocatorCounterItem Inherited;

    //内存块描述结构，用于查询内存块信息
    struct ElsaSDataBlockDesc
    {
        const char *m_cpAddr; // 内存块指针地址
        size_t m_blockSize;   // 内存块大小
    };

private:
    //内存块链表节点
    struct ElsaSDataBlock
    {
        T *m_pFreePtr; //指向空闲的对象
        T *m_pEndPtr;  //指向对象内存块的结束
        ElsaSDataBlock *m_pPrev;
    } * m_pDataBlock;

public:
    ElsaCMemoryContainer(const char *name) : Inherited(name)
    {
        m_pDataBlock = nullptr;
    }

    virtual ~ElsaCMemoryContainer()
    {
        ElsaSDataBlock *block_ptr, *prev;
        block_ptr = m_pDataBlock;
        while (block_ptr)
        {
            prev = block_ptr->m_pPrev;
            free(block_ptr);
            block_ptr = prev;
        }

        m_pDataBlock = nullptr;
        ElsaCMemoryCounter::GetSingleton().Unregist(this); //  按道理来说这里父类的析构函数是会被执行的，所以这一句话并没有什么意义
    }

    inline ElsaCMemoryContainer &operator=(const ElsaCMemoryContainer &rhs)
    {
        Inherited::operator=(rhs);
        m_pDataBlock = rhs.m_pDataBlock;
        return *this;
    }

private:
    ElsaCMemoryContainer(const ElsaCMemoryContainer &rhs); // 禁止拷贝

public:
#ifndef _MEMORY_TRACE_
    T *AllocBuffer(size_t count)
#else
    T *_AllocBuffer(size_t count, const char *, int)
#endif
    {
        T *result = nullptr;
        ElsaSDataBlock *block_ptr = m_pDataBlock;

        if (block_ptr == nullptr || static_cast<size_t>(block_ptr->m_pEndPtr - block_ptr->m_pFreePtr) < count)
        {
            // 遍历内存链表查找第一块可以分配的内存空间
            while (block_ptr != nullptr && (block_ptr = block_ptr->m_pPrev))
            {
                if (static_cast<size_t>(block_ptr->m_pEndPtr - block_ptr->m_pFreePtr) >= count)
                    break;
            }
            // 内存不足则分配内存，并插入到链表尾部中
            if (block_ptr == nullptr)
            {
                size_t alloc_count = __max(count, ONE_TIME_COUNT); //  一次至少需要分配ONE_TIME_COUNT内存
                block_ptr = static_cast<ElsaSDataBlock *>(malloc(sizeof(ElsaSDataBlock) + alloc_count * sizeof(T)));
                memset(block_ptr, 0, sizeof(ElsaSDataBlock) + alloc_count * sizeof(T));
#ifdef _MEMORY_TRACE_
                m_allocTotal += alloc_count;
#endif
                block_ptr->m_pFreePtr = reinterpret_cast<T *>(block_ptr + 1);
                block_ptr->m_pEndPtr = block_ptr->m_pFreePtr + alloc_count;
                block_ptr->m_pPrev = m_pDataBlock;
                m_pDataBlock = block_ptr;
            }
        }
        result = block_ptr->m_pFreePtr;
        block_ptr->m_pFreePtr += count;
#ifdef _MEMORY_TRACE_
        m_usedCount += count;
#endif
        return result;
    }

    // 一次性释放所有内存
    void _FreeBuffer()
    {
        ElsaSDataBlock *block_ptr, *prev;
        block_ptr = m_pDataBlock;
        while (block_ptr)
        {
            prev = block_ptr->m_pPrev;
            free(block_ptr);
            block_ptr = prev;
        }
        m_pDataBlock = nullptr;
    }

    // 返回链表结点个数(块数)
    inline int BlockCount() const
    {
        int result = 0;
        ElsaSDataBlock *block_ptr = m_pDataBlock;

        while (block_ptr)
        {
            result++;
            block_ptr = block_ptr->m_pPrev;
        }
        return result;
    }

    /*** 如果enum_key为null会自动枚举类自己的内存情况，由于内存管理的链表是逆序(采用尾插法和记录尾结点)，所以枚举顺序和链表逻辑顺序是相反的
     * @param {void*} enum_key 上一次枚举的ElsaSDataBlock类型开头的内存数据
     * @param {ElsaSDataBlockDesc& } desc 统计消息会存放到这个参数中
     * @return {void *} 返回这次枚举的ElsaSDataBlock指针
     * @use
     *  -------------------------------------------------
     *  LPCVOID lpEnumKey = NULL;
     *  BuffAllocator<T>::DataBlockDesc desc;
     *  while (lpEnumKey = allocator.enumBlockDesc(lpEnumKey, desc))
     *  {
     *     printf("Memory Block Info { Address = %X, Size = %u }",
     *	    desc.m_cpAddr, desc.m_blockSize);
     *  }
     * --------------------------------------------------------------
     */
    inline void *EnumBlockDesc(void *enum_key, ElsaSDataBlockDesc &desc) const
    {
        ElsaSDataBlock *block_ptr = static_cast<ElsaSDataBlock *>(enum_key);
        if (block_ptr == nullptr)
            block_ptr = m_pDataBlock;
        else
            block_ptr = block_ptr->m_pPrev;

        if (block_ptr)
        {
            desc.m_cpAddr = reinterpret_cast<const char *>(block_ptr + 1);
            desc.m_blockSize = block_ptr->m_pEndPtr - reinterpret_cast<T *>(const_cast<char*>(desc.m_cpAddr));
            return reinterpret_cast<void *>(block_ptr);
        }
        return nullptr;
    }
};

#ifdef TEST_DEBUG
inline void ElsaCMemoryContainerTestFunc()
{
    ElsaCMemoryContainer<int> tmp("ElsaCMemoryContainer");
    tmp._AllocBuffer(100, nullptr, 0);
    tmp._AllocBuffer(2020, nullptr, 0);
    tmp._AllocBuffer(3000, nullptr, 0);
    tmp._AllocBuffer(400, nullptr, 0);

    void *lpEnumKey = nullptr;
    ElsaCMemoryContainer<int>::ElsaSDataBlockDesc desc;
    while (lpEnumKey = tmp.EnumBlockDesc(lpEnumKey, desc))
    {
        printf("Memory Block Info { Address = %X, Size = %u }\n",
               desc.m_cpAddr, desc.m_blockSize);
    }

    ElsaCMemoryCounter::GetSingleton().LogToFile();
}
#endif