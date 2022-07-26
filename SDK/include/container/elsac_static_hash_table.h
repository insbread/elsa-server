/***
 * @Author: insbread
 * @Date: 2022-07-20 20:45:10
 * @LastEditTime: 2022-07-20 20:45:10
 * @LastEditors: insbread
 * @Description: 一个开放地址的线性搜索hashtable，表长度是固定的，不能动态增长，同时不提供删除的接口为提供更快的性能
 * @FilePath: /elsa-server/SDK/include/container/elsac_static_hash_table.h
 * @版权声明
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace ElsaNContainer
{
    template <typename T, int MAX_NODE_NUM>
    class ElsaCStaticHashTable
    {
    protected:
        class ElsaCHashNode
        {
        public:
            bool m_bExists;    //  当前桶是否保存了数据
            uint64_t m_u64Key; //  当前关键字
            T m_value;         //  当前值
        };
        size_t m_size;                                   //  已存放的结点数
        size_t m_outCount;                               //  超出MAX_NODE_NUM范围的数据，(m_pPtr可以分为两个区域[0 ~ MAX_NODE_NUM|MAX_NODE_NUM + 1 ~ MAX_SIZE]，变量就是存放在第二个区域的数量)
        static const size_t MAX_SIZE = MAX_NODE_NUM * 2; //  避免冲突
        ElsaCHashNode m_pPtr[MAX_SIZE];                  //  哈希表
    public:
        ElsaCStaticHashTable() : m_size(0), m_outCount(0)
        {
            Clear();
        }

        virtual ~ElsaCStaticHashTable()
        {
            Clear();
        }

        void Clear()
        {
            m_size = 0;
            m_outCount = 0;
            memset(m_pPtr, 0, MAX_SIZE * sizeof(ElsaCHashNode));
        }

        inline size_t Count() const
        {
            return m_size;
        }

    public:
        // 哈希线性搜索方法,获取键key在哈希表中的索引
        int GetIndex(uint64_t key) const
        {
            // 线性搜索方法，注意不会循环搜索
            size_t start = key % MAX_NODE_NUM, idx = start;
            while (m_pPtr[idx].m_bExists && idx < MAX_SIZE)
            {
                if (m_pPtr[idx].m_u64Key == key)
                    return static_cast<int>(idx);
                else
                    idx++;
            }
            return -1;
        }
        // 获取key对应的键值
        inline T *Get(uint64_t key)
        {
            int idx = GetIndex(key);
            return (idx >= 0) ? &m_pPtr[idx].m_value : nullptr;
        }
        // 插入一个key-value值
        inline T *Put(uint64_t key, T &data)
        {
            if (m_size > MAX_NODE_NUM)
                return nullptr;
            // 线性搜索可以插入的位置
            unsigned int start = key % MAX_NODE_NUM, idx = start;
            do
            {
                ElsaCHashNode *node = m_pPtr + idx;
                if (node->m_bExists == false)
                {
                    node->m_bExists = true;
                    node->m_u64Key = key;
                    m_size++;
                    node->m_value = data;
                    if (idx >= MAX_NODE_NUM)
                        ++m_outCount;
                    return &(node->m_value);
                }
                ++idx;
            } while (idx < MAX_SIZE);
        }
        // 更新键key对应的value值
        void Update(uint64_t key, const T &data)
        {
            int idx = GetIndex(key);

            if (idx >= 0)
            {
                m_pPtr[idx].m_value = data;
            }
        }
        // 删除功能本身就是禁止的，需要C++11的支持，所以后面的代码是有问题的
        inline bool Remove(uint64_t key) = delete;
        /*
        {
            int idx = GetIndex(key);

            if (idx < 0)
                return false;

            m_pPtr[idx].m_bExists = false;
            m_size--;

            // 将所有从被挤到[MAX_NODE_NUM ~ MAX_SIZE]元素向前移动一个位置进行填补
            int start = idx;
            idx++;
            while (m_pPtr[idx].m_bExists && idx < static_cast<int>(MAX_SIZE))
            {
                if (start >= MAX_NODE_NUM)
                {
                    int cnt = static_cast<int>(m_outCount) - (start - MAX_NODE_NUM);
                    if (cnt > 0)
                    {
                        memmove(m_pPtr + start, m_pPtr + idx, cnt * sizeof(ElsaCHashNode));
                        m_pPtr[MAX_NODE_NUM + m_outCount].m_bExists = false;
                    }
                    break;
                }

                int key_idx = m_pPtr[idx].m_u64Key % MAX_NODE_NUM;
                if (key_idx <= start)
                {
                    m_pPtr[start] = m_pPtr[idx];
                    m_pPtr[idx].m_bExists = false;
                    start = idx;

                    if (idx >= MAX_NODE_NUM)
                        --m_outCount;
                }
                ++idx;
            }
            return true;
        }
        */
    };
};

// #define TEST_DEBUG
#ifdef TEST_DEBUG
#include <cstdio>
using namespace ElsaNContainer;

inline void ElsaCStaticHashTableTestFunc()
{
    ElsaCStaticHashTable<int, 100> tm;
    int tm_arr[20];
    memset(tm_arr, 0, sizeof(tm_arr));
    for (int i = 0; i < 20; i++)
        tm_arr[i] = 2 * i;

    for (int i = 0; i < 20; i++)
        tm.Put(i, tm_arr[i]);
    for (int i = 0; i < 20; i++)
        printf("i:%d -- ", *tm.Get(i));
    puts("\n");

    for (int i = 0; i < 10; i++)
        tm.Update(i, 2 * i + 100);
    for (int i = 0; i < 20; i++)
        printf("i:%d -- ", *tm.Get(i));
    puts("\n");
}
#endif