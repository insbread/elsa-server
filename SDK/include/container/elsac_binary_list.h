/***
 * @Author: insbread
 * @Date: 2022-07-20 19:28:07
 * @LastEditTime: 2022-07-20 19:28:07
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/container/elsac_binary_list.h
 * @版权声明
 */

#pragma once
#include <assert.h>
#include "container/elsac_vector.h"
#include "container/elsac_static_array_list.h"

namespace ElsaNContainer
{
    /*
    
    */
    template <typename T, int ONE_TIME_NUM = 16>
    class ElsaCBinaryList : public ElsaCVector<T, ONE_TIME_NUM>
    {
    public:
        ElsaCBinaryList(ElsaCBaseAllocator *alloc = nullptr) : ElsaCVector<T, ONE_TIME_NUM>(alloc) {}
        virtual ~ElsaCBinaryList() {}

        // 基于二分插入数据，如果有相同的数据则默认插入到最后一个位置
        int AddItem(const T &item)
        {
            if (this->m_iCount == 0)
            {
                this->Insert(0, item);
                return 0;
            }
            int low = 0, high = this->m_iCount - 1;
            while (low < high)
            {
                int mid = (low + high + 1) >> 1;
                if (this->m_pDataPtr[mid] <= item)
                    low = mid;
                else
                    high = mid - 1;
            }
            this->Insert(low + 1, item);
            return low + 1;
        }
        // 移除元素，如果不指定重复则移除相同元素的第一个元素，否则移除所有相同的元素
        void RemoveItem(const T &item, bool bDelRepeat = false)
        {
            int idx = Find(item);
            if (idx < -1)
                return;

            if (bDelRepeat == false)
            {
                this->Remove(idx);
                return;
            }

            int del_len = 0;
            for (int i = idx; i < this->m_iCount; i++)
            {
                if (this->m_pDataPtr[i] == item)
                {
                    del_len++;
                }
                else
                {
                    break;
                }
            }

            this->Remove(idx, del_len);
        }
        // 二分查找item的位置，如果有相同的位置，则找到第一个位置
        int Find(const T &item)
        {
            if (this->m_iCount == 0)
                return -1;
            int low = 0, high = this->m_iCount - 1;
            while (low < high)
            {
                int mid = (low + high) >> 1;
                if (this->m_pDataPtr[mid] >= item)
                    high = mid;
                else
                    low = mid + 1;
            }
            if (this->m_pDataPtr[low] != item)
                return -1;
            return low;
        }
    };

    template <typename T, int ONE_TIME_NUM = 16>
    class ElsaCStaticBinaryList : public ElsaCStaticArrayList<T, ONE_TIME_NUM>
    {
    public:
        ElsaCStaticBinaryList() {}
        virtual ~ElsaCStaticBinaryList() {}
        // 基于二分插入数据，如果有相同的数据则默认插入到最后一个位置
        int AddItem(const T &item)
        {
            if (this->m_iCount == 0)
            {
                this->Insert(0, item);
                return 0;
            }
            int low = 0, high = this->m_iCount - 1;
            while (low < high)
            {
                int mid = (low + high + 1) >> 1;
                if (this->m_pDataPtr[mid] <= item)
                {
                    low = mid;
                }
                else
                {
                    high = mid - 1;
                }
            }

            this->Insert(low + 1, item);
            return low + 1;
        }
        // 二分查找，如果有重复元素则查到第一个元素所在的位置
        int Find(const T &item)
        {
            if (this->m_iCount == 0)
                return -1;
            int low = 0, high = this->m_iCount - 1;
            while (low < high)
            {
                int mid = (low + high) >> 1;
                if (this->m_pDataPtr[mid] >= item)
                {
                    high = mid;
                }
                else
                {
                    low = mid + 1;
                }
            }
            if (this->m_pDataPtr[low] != item)
                return -1;
            return low;
        }
        // 二分查找item的位置，如果有相同的位置，则找到第一个位置
        void RemoveItem(const T &item, bool bDelRepeat = false)
        {
            int idx = Find(item);
            if (idx < 0)
                return;
            if (bDelRepeat == false)
            {
                this->Remove(idx);
                return;
            }

            int del_len = 0;
            for (int i = idx; i < this->m_iCount; i++)
            {
                if (this->m_pDataPtr[i] == item)
                {
                    del_len++;
                }
            }

            this->Remove(idx, del_len);
        }
    };
};

    // #define TEST_DEBUG

#ifdef TEST_DEBUG
using namespace ElsaNContainer;
void ElsaCBinaryListTestFunc()
{
    ElsaCBinaryList<int> tm;

    for (int i = 0; i < 20; i++)
        tm.AddItem(i);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    for (int i = 0; i < 20; i += 2)
        tm.RemoveItem(i);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.Clear();
    for (int i = 1; i < 5; i++)
        tm.AddItem(2);
    for (int i = 1; i < 5; i++)
        tm.AddItem(5);
    for (int i = 1; i < 5; i++)
        tm.AddItem(10);
    for (int i = 1; i < 5; i++)
        tm.AddItem(20);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");
    tm.RemoveItem(5, true);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    printf("find a not exist element 1000 in a not empty list: %d\n", tm.Find(1000));
    printf("find an exist element 2 in an empty list: %d\n", tm.Find(2));
    printf("find an exist element 10 in an empty list: %d\n", tm.Find(10));
    while (tm.Count())
        tm.RemoveItem(tm[0]);
    printf("After Remove: tm size %d\n", tm.Count());
    printf("Find a not exist 2 element in an empty list: %d\n", tm.Find(2));
    return;
}

void ElsaCStaticBinaryListTestFunc()
{
    ElsaCStaticBinaryList<int, 100> tm;

    for (int i = 0; i < 20; i++)
        tm.AddItem(i);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    for (int i = 0; i < 20; i += 2)
        tm.RemoveItem(i);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    tm.Clear();
    for (int i = 1; i < 5; i++)
        tm.AddItem(2);
    for (int i = 1; i < 5; i++)
        tm.AddItem(5);
    for (int i = 1; i < 5; i++)
        tm.AddItem(10);
    for (int i = 1; i < 5; i++)
        tm.AddItem(20);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");
    tm.RemoveItem(5, true);
    for (int i = 0; i < tm.Count(); i++)
        printf("%d ", tm[i]);
    puts("\n");

    printf("find a not exist element 1000 in a not empty list: %d\n", tm.Find(1000));
    printf("find an exist element 2 in an empty list: %d\n", tm.Find(2));
    printf("find an exist element 10 in an empty list: %d\n", tm.Find(10));
    while (tm.Count())
        tm.RemoveItem(tm[0]);
    printf("After Remove: tm size %d\n", tm.Count());
    printf("Find a not exist 2 element in an empty list: %d\n", tm.Find(2));
    return;
}
#endif