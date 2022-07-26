/***
 * @Author: insbread
 * @Date: 2022-07-18 17:54:07
 * @LastEditTime: 2022-07-18 17:54:07
 * @LastEditors: insbread
 * @Description: 类似std::vector 的动态数组类，接口尽量和vector接近，除了转化为驼峰命名之外；注意：可指定最小的内存数量，默认是16，每次扩展是翻倍
 * @FilePath: /elsa-server/SDK/include/container/elsac_vector.h
 * @版权声明
 */
#pragma once

#include "memory/elsac_base_allocator.hpp"

namespace ElsaNContainer
{
    template <typename T, int ONE_TIME_NUM = 16>
    class ElsaCVector
    {
    protected:
        T *m_pDataPtr;
        int m_iMaxSize; //  最多的元素个数
        int m_iCount;   //  vec中元素的个数
        ElsaCBaseAllocator *m_pAlloc;

    public:
        ElsaCVector(ElsaCBaseAllocator *alloc = nullptr)
        {
            m_pDataPtr = nullptr;
            m_iMaxSize = 0;
            m_iCount = 0;
            m_pAlloc = alloc;
        }
        ~ElsaCVector()
        {
            Empty();
        }

        inline int Count() const
        {
            return m_iCount;
        }
        inline int Size() const
        {
            return m_iCount;
        }
        inline int MaxSize() const
        {
            return m_iMaxSize;
        }

        // 将内存扩充到count，原来的数据不发生改变
        virtual void reverse(int count)
        {
            if (count > m_iCount && count != m_iMaxSize)
            {
                m_iMaxSize = count;
                if (m_pAlloc != nullptr)
                {
                    m_pDataPtr = static_cast<T *>(m_pAlloc->ReAllocBuffer(m_pDataPtr, sizeof(T) * count));
                }
                else
                {
                    m_pDataPtr = static_cast<T *>(realloc(m_pDataPtr, sizeof(T) * count));
                }
            }
        }

        // 在index位置插入data数据
        void Insert(const int index, const T &data)
        {
            assert(index > -1 && index <= m_iCount);
            //  双倍分配内存
            if (m_iCount >= m_iMaxSize)
                reverse((m_iMaxSize > 0) ? m_iMaxSize * 2 : ONE_TIME_NUM);

            if (index < m_iCount)
            {
                memmove(m_pDataPtr + index + 1, m_pDataPtr + index, sizeof(T) * (m_iCount - index));
            }

            m_pDataPtr[index] = data;
            m_iCount++;
        }
        // 在最后一个位置插入data数据，返回插入后的位置索引
        int Add(const T &data)
        {
            if (m_iCount >= m_iMaxSize)
                reverse((m_iMaxSize > 0) ? 2 * m_iMaxSize : ONE_TIME_NUM);

            memcpy(m_pDataPtr + m_iCount, &data, sizeof(T));
            m_iCount++;
            return m_iCount;
        }
        // 在最后一个位置插入data数据，返回插入后的位置索引
        int PushBack(const T &data)
        {
            return Add(data);
        }
        // 在最后一个位置插入data数据，返回插入后的位置索引
        int Push(const T &data)
        {
            return Add(data);
        }
        // 将Vector的所有数据追加到数据末尾
        void AddList(const ElsaCVector<T> &list)
        {
            AddArray(list.m_pDataPtr, list.m_iCount);
        }
        // 将T*数组追加到数据末尾
        void AddArray(T *data, int length)
        {
            if (m_iCount + length > m_iMaxSize)
                reverse(m_iCount + length);
            memcpy(m_pDataPtr + m_iCount, data, sizeof(T) * length);
            m_iCount += length;
        }

        // 从数组的第index位置移除count个元素
        void Remove(const int index, const int count)
        {
            assert(index >= 0 && index < m_iCount);

            if (count > 0)
            {
                memmove(m_pDataPtr + index, m_pDataPtr + index + count, sizeof(T) * (m_iCount - index - count));
                m_iCount -= count;
            }
        }
        // 从数组移除第index所在的元素
        void Remove(const int index)
        {
            assert(index >= 0 && index < m_iCount);
            Remove(index, 1);
        }
        // 从vec尾部弹出一个元素
        T Pop()
        {
            if (m_iCount > 0)
            {
                return m_pDataPtr[--m_iCount];
            }
            throw "stack was empty";
        }
        // 从vec尾部弹出一个元素
        T PopBack()
        {
            return Pop();
        }
        // 截断数组
        void Trunc(const int count)
        {
            assert(count >= 0 && count <= m_iMaxSize);
            m_iCount = count;
        }
        // 逻辑清除
        void Clear()
        {
            m_iCount = 0;
        }
        // 物理清除
        void Empty()
        {
            Clear();
            m_iMaxSize = 0;

            if (m_pDataPtr != nullptr)
            {
                if (m_pAlloc != nullptr)
                    m_pAlloc->FreeBuffer(m_pDataPtr);
                else
                    free(m_pDataPtr);
                m_pDataPtr = nullptr;
            }
        }

        T &Get(const int index) const
        {
            assert(index >= 0 && index < m_iCount);
            return m_pDataPtr[index];
        }
        void Set(const int index, const T &data)
        {
            assert(index >= 0 && index < m_iCount);
            m_pDataPtr[index] = data;
        }
        //  返回data所在的逆序第一个位置
        int Index(const T &data) const
        {
            for (int i = m_iCount - 1; i >= 0; i--)
                if (m_pDataPtr == data)
                    return i;
            return -1;
        }
        //  返回data所在从endIndex逆序的第一个位置索引
        int Index(const T &data, const int endIndex) const
        {
            for (int i = endIndex; i >= 0; i++)
                if (m_pDataPtr[i] == data)
                    return i;
            return -1;
        }

        inline operator T *() const
        {
            return m_pDataPtr;
        }
    };
};

#ifdef TEST_DEBUG
#include <time.h>
#include <cstdlib>
#include "memory/elsac_buffer_allocator.h"
#include "memory/elsac_align_allocator.h"
inline void ElsaCVectorTestFunc()
{
    // 顺便测试分配器是否有问题
    // ElsaCBufferAllocator alloc1("ElsaCBufferAllocator1");
    // ElsaCBufferAllocator alloc2("ElsaCBufferAllocator2");
    ElsaCAlignAllocator alloc1("Align1", 1024, 16);
    ElsaCAlignAllocator alloc2("Align2", 1024, 16);
    ElsaNContainer::ElsaCVector<int> tm1(&alloc1);
    ElsaNContainer::ElsaCVector<int> tm2(&alloc2);
    int tmp = 0;
    srand((unsigned int)time(nullptr));
    int arr1[10], arr2[10];
    for (int i = 0; i <= 10; i++)
    {
        arr1[i] = rand() % 1000;
        arr2[i] = rand() % 4000;
        // arr1[i] = i;
        // arr2[i] = i;
    }

    tm1.AddArray(arr1, 10);
    tm2.AddArray(arr2, 10);
    printf("AddArray:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tmp = rand() % 1000;
    tm1.Insert(0, tmp);
    printf("Insert tmp:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tmp = rand() % 1000;
    tm1.Add(tmp);
    printf("Add tmp:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tm1.AddList(tm2);
    printf("AddList:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tm1.Remove(1, 5);
    printf("Remove:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tmp = tm1.Pop();
    printf("Pop:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    printf("\ntmp: %d", tmp);
    puts("\n");

    printf("Get:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1.Get(i));
    puts("\n");

    printf("Set:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        tm1.Set(i, i + 10);
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tm1.Trunc(5);
    printf("Trunc:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");

    tm1.Clear();
    printf("Clear:\nCount: %d, Size: %d, MaxSize: %d\n", tm1.Count(), tm1.Size(), tm1.MaxSize());
    for (int i = 0; i < tm1.Size(); i++)
        printf("%d ", tm1[i]);
    puts("\n");
}
#endif