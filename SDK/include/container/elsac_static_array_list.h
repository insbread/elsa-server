/***
 * @Author: insbread
 * @Date: 2022-07-20 17:16:14
 * @LastEditTime: 2022-07-20 17:16:15
 * @LastEditors: insbread
 * @Description: 静态vector数组，数组大小固定
 * @FilePath: /elsa-server/SDK/include/container/elsac_array_list.h
 * @版权声明
 */

#pragma once
#include <cstring>
#include <assert.h>

#include "container/elsac_vector.h"
namespace ElsaNContainer
{
    template <typename T, int MAX_NUM>
    class ElsaCStaticArrayList
    {
    protected:
        T m_pDataPtr[MAX_NUM];
        int m_iCount;

    public:
        ElsaCStaticArrayList()
        {
            m_iCount = 0;
        };
        virtual ~ElsaCStaticArrayList()
        {
            m_iCount = 0;
        };

        inline operator T *() { return m_pDataPtr; }
        inline operator T *() const { return m_pDataPtr; }

        int Size() const { return m_iCount; }
        int Count() const { return m_iCount; }
        int MaxSize() const { return MAX_NUM; }

        // 在数组末尾追加一个数字，并返回所在的索引
        int Add(const T &data)
        {
            assert(m_iCount + 1 <= MAX_NUM); //  保证还有空位置
            memcpy(m_pDataPtr + m_iCount, &data, sizeof(T));
            m_iCount++;
            return m_iCount - 1;
        }
        // 在数组末尾追加一个数字，并返回所在的索引
        int PushBack(const T &data)
        {
            return Add(data);
        }
        // 在idx处插入数据data
        void Insert(const int idx, const T &data)
        {
            assert(idx >= 0 && idx <= m_iCount); //  确保索引合法
            assert(m_iCount + 1 <= MAX_NUM);     //  确保有空闲位置

            if (idx < m_iCount)
            {
                memmove(m_pDataPtr + idx + 1, m_pDataPtr + idx, sizeof(T) * (m_iCount - idx));
            }
            m_pDataPtr[idx] = data;
            m_iCount++;
        }

        inline const T &Get(const int index) const
        {
            assert(index >= 0 && index < m_iCount);
            return m_pDataPtr[index];
        }
        inline void Set(const int index, const T &data)
        {
            assert(index >= 0 && index < m_iCount);
            m_pDataPtr[index] = data;
        }

        void Remove(const int index, const int num)
        {
            assert(index >= 0 && index + num - 1 <= m_iCount);
            if (num > 0)
            {
                memmove(m_pDataPtr + index, m_pDataPtr + index + num, sizeof(T) * (m_iCount - index - num));
                m_iCount -= num;
            }
        }
        void Remove(const int index)
        {
            assert(index >= 0 && index < m_iCount); //  这里会顺便检测是否为空，因为为空的话assert永远为false
            Remove(index, 1);
        }

        inline void Clear()
        {
            m_iCount = 0;
        }
        inline void Trunc(const int num)
        {
            assert(num >= 0 && num <= MAX_NUM);
            m_iCount = num;
        }
        inline void AddArray(const T *data, int length)
        {
            assert(m_iCount + length <= MAX_NUM);
            if (length > 0)
            {
                memcpy(m_pDataPtr + m_iCount, data, sizeof(T) * length);
                m_iCount += length;
            }
        }
        inline void AddList(const ElsaCVector<T> &list)
        {
            AddArray((T *)list, list.Count());
        }
        // 从后往前搜索数组，返回逆序第一个找到data的下标
        int Index(const T &data) const
        {
            for (int i = m_iCount - 1; i >= 0; i--)
            {
                if (m_pDataPtr[i] == data)
                {
                    return i;
                }
            }
            return -1;
        }
    };
};
// #define TEST_DEBUG
#ifdef TEST_DEBUG
using namespace ElsaNContainer;
inline void ElsaCStaticArrayListTestFuc()
{
    ElsaCVector<int> vec;
    ElsaCStaticArrayList<int, 200> tm;
    for (int i = 0; i < 10; i++)
    {
        tm.PushBack(i * 2);
        vec.Push(i + 21);
    }

    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.Insert(1, 1);
    tm.Insert(3, 3);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.Set(0, 100);
    tm.Set(1, 200);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.Remove(0, 2);
    tm.Remove(tm.Count() - 1, 1);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.AddList(vec);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    printf("idx of 10 : %d\n", tm.Index(10));
    return;
}
#endif