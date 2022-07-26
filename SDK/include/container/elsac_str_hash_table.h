/***
 * @Author: insbread
 * @Date: 2022-07-21 15:46:40
 * @LastEditTime: 2022-07-21 15:46:40
 * @LastEditors: insbread
 * @Description: 基于暴雪哈希算法的字符串哈希表
 * @FilePath: /elsa-server/SDK/include/container/elsac_str_hash_table.h
 * @版权声明
 */
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include "elsa_bzhash.h"

namespace ElsaNContainer
{
    template <typename T>
    class ElsaCStrHashTable;

    /*
        哈希表迭代器
    */
    template <typename T>
    class ElsaCStrHashTableIterator
    {
    private:
        const ElsaCStrHashTable<T> *m_pTable; // 要遍历的哈希表
        int m_iIndex;                         // 当前正在遍历的哈希表的索引下标

    public:
        ElsaCStrHashTableIterator()
        {
            m_pTable = nullptr;
            m_iIndex = 0;
        }

        ElsaCStrHashTableIterator(const ElsaCStrHashTable<T> &table)
        {
            SetTable(table);
        }

        inline void SetTable(const ElsaCStrHashTable<T> &table)
        {
            m_pTable = &table;
            m_iIndex = 0;
        }

        inline T *First()
        {
            int iLen = (int)(m_pTable->m_nLen);

            m_iIndex = 0;
            while (m_iIndex < iLen)
            {
                typename ElsaCStrHashTable<T>::NodeType *pNode = &m_pTable->m_pTable[m_iIndex];

                m_iIndex++;
                if (pNode->m_uiHash1)
                    return &pNode->m_value;
            }

            return nullptr;
        }

        inline T *Next()
        {
            int iLen = (int)(m_pTable->m_nLen);
            while (m_iIndex < iLen)
            {
                typename ElsaCStrHashTable<T>::NodeType *pNode = &m_pTable->m_pTable[m_iIndex];
                m_iIndex++;

                if (pNode->m_uiHash1)
                    return &pNode->m_value;
            }
            return nullptr;
        }
    };

    /*
        基于暴雪哈希算法构建的字符串哈希表。
        基本思想是开放定址法，即冲突就顺延找下一个位置。为了避免字符串会被哈希到同一个值上，
        会采用三个哈希值验证一个字符串，第一个哈希值用于定位，第二第三个哈希值用于验证，只有找到位置之后只有后两个哈希值都相等才是正确的字符串。
    */
    template <typename T>
    class ElsaCStrHashTable
    {
        friend class ElsaCStrHashTableIterator<T>;
    protected:
        // 哈希表结点，每个结点包含三个哈希值
        template <typename TA>
        class ElsaCHashNode
        {
        public:
            unsigned int m_uiHash1;
            unsigned int m_uiHash2;
            unsigned int m_uiHash3;
            TA m_value;
        };

        typedef ElsaCHashNode<T> NodeType;

    public:
        static const size_t MiniSize = 16;

    protected:
        size_t m_nInitSize;         //  初始化桶的最小数量
        size_t m_nLen;              //  所有可以使用的桶数量
        size_t m_nFree;             //  空桶数量
        ElsaCHashNode<T> *m_pTable; //  哈希表

    public:
        ElsaCStrHashTable(size_t len = 0)
        {
            m_pTable = nullptr;
            m_nLen = m_nFree = 0;
            m_nInitSize = len;

            // 找到第一个比MiniSize以及len大的2的幂方数，例如len = 1023，那么目标数就是2 ** 10 = 1024，并且将哈希长度设置为这个值
            if (len > MiniSize)
            {
                size_t val;
                for (int i = 0; i < 32; i++)
                {
                    val = size_t(1 << i);

                    if (len <= val)
                    {
                        m_nInitSize = val;
                        break;
                    }
                }
            }
            else
            {
                m_nInitSize = MiniSize;
            }
        }

        virtual ~ElsaCStrHashTable()
        {
            Clear();
        }

        // 物理清空哈希表，会调用哈希表中值对象的析构函数，同时删除哈希表的内存空间
        void Clear()
        {
            for (int i = (int)m_nLen - 1; i >= 0; i--)
            {
                if (m_pTable[i].m_uiHash1)
                {
                    m_pTable[i].m_value.~T();
                }
            }

            if (m_pTable != nullptr)
                Realloc(m_pTable, 0);

            m_pTable = nullptr;
            m_nLen = m_nFree = 0;
        }
        // 返回当前已经使用的桶数量
        inline size_t Count() const
        {
            return m_nLen - m_nFree;
        }

    public:
        //  获取字符串sKey对应的索引值
        int GetIndex(const char *sKey) const
        {
            unsigned int idx, start;
            size_t len;

            if (m_nLen <= 0)
                return -1;

            unsigned int hash1 = bzhashstr(sKey, 0);
            unsigned int hash2 = bzhashstr(sKey, 1);
            unsigned int hash3 = bzhashstr(sKey, 2);

            len = m_nLen;

            // 先进行一次就能哈希成功的尝试，因为每次哈希表扩容都是两倍扩容，所以收缩也只需要收缩一半即可。
            // 所谓一次哈希成功就是在某个长度的情况下，哈希一次就成功，由于一次哈希成功的前提是基于某个长度，所以要遍历这个可能的长度。
            // 对于当前哈希表长度的可能性是：m_iInitSize, 2 * m_iInitSize, 2^2 * m_iInitSize, ... , 2^n * m_iInitSize，因此缩短一半就是遍历
            while (len >= m_nInitSize)
            {
                idx = hash1 & ((unsigned int)len - 1); //  保证hash1指定的位置不会超过范围
                NodeType *pNode = &m_pTable[idx];

                // 如果三个哈希值全部都验证成功，就表示这个哈希字符串确实在这个位置上
                if (pNode->m_uiHash1 == hash1 && pNode->m_uiHash2 == hash2 && pNode->m_uiHash3 == hash3)
                {
                    return idx;
                }

                len >>= 1; //  长度减少为原来的一半
            }

            start = idx = hash1 & ((unsigned int)m_nLen - 1);
            // 尝试进行顺延的操作找到索引，需要循环遍历整个表（因为存在扩容，所以无法搜索到空桶就停止）
            do
            {
                NodeType *pNode = &m_pTable[idx];
                if (pNode->m_uiHash1 == hash1 && pNode->m_uiHash2 == hash2 && pNode->m_uiHash3 == hash3)
                {
                    return idx;
                }
                idx = (idx + 1) & ((unsigned int)m_nLen - 1); //  避免超出长度限制
            } while (start != idx);
            return -1;
        }

        inline T *Get(const char *sKey)
        {
            int idx = GetIndex(sKey);
            return (idx >= 0) ? &m_pTable[idx].m_value : nullptr;
        }
        inline const T *Get(const char *sKey) const
        {
            int idx = GetIndex(sKey);
            return (idx >= 0) ? &m_pTable[idx].m_value : nullptr;
        }

        inline T *Put(const char *sKey)
        {
            unsigned int hash1, idx, start;
            unsigned int __attribute__((unused)) hash2, hash3;

            // 如果没有剩余的空桶，就分配新的空间
            if (m_nFree <= 0)
            {
                size_t oldLen = m_nLen;
                m_nLen = (oldLen <= 0) ? m_nInitSize : m_nLen << 1; //  两倍增长
                m_nFree = m_nLen - oldLen;
                m_pTable = static_cast<NodeType *>(Realloc(m_pTable, m_nLen * sizeof(NodeType)));
                memset(m_pTable + oldLen, 0, m_nFree * sizeof(NodeType)); //  将新分配的空间刷新为0
            }

            hash1 = bzhashstr(sKey, 0);
            hash2 = bzhashstr(sKey, 1);
            hash3 = bzhashstr(sKey, 2);
            start = idx = hash1 & ((unsigned int)m_nLen - 1);

            // 循环顺延找到对应的位置
            do
            {
                NodeType *pNode = &m_pTable[idx];

                if (!pNode->m_uiHash1) //  如果当前结点还没有被使用过
                {
                    pNode->m_uiHash1 = hash1;
                    pNode->m_uiHash2 = bzhashstr(sKey, 1);
                    pNode->m_uiHash3 = bzhashstr(sKey, 2);
                    m_nFree--;
                    new (&pNode->m_value) T();
                    return &pNode->m_value;
                }
                idx = (idx + 1) & ((unsigned int)m_nLen - 1);
            } while (start != idx);
            return nullptr;
        }
        inline int Update(const char *sKey, const T &value)
        {
            int idx = GetIndex(sKey);
            if (idx >= 0)
                m_pTable[idx].m_value = value;
            return idx;
        }
        inline int Remove(const char *sKey)
        {
            int idx = GetIndex(sKey);
            if (idx >= 0)
            {
                NodeType *pNode = &m_pTable[idx];
                pNode->m_uiHash1 = pNode->m_uiHash2 = pNode->m_uiHash3 = 0;
                m_nFree++;
                pNode->m_value.~T();
                return idx;
            }

            return -1;
        }

    protected:
        virtual void *Realloc(void *p, size_t s)
        {
            return realloc(p, s);
        }

#ifdef TEST_DEBUG
#include <cstdio>
    public:
        size_t Show()
        {
            printf("MaxSize: %u, FreeSize: %u, UsedSize: %u\n", m_nLen, m_nFree, m_nLen - m_nFree);
        }
#endif
    };
};

// #define TEST_DEBUG
#ifdef TEST_DEBUG
#include <iostream>
using namespace ElsaNContainer;
inline void ElsaCStrHashTableTestFunc()
{
    ElsaCStrHashTable<int> tm(8);
    char const *str_arr[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
                             "100", "101", "102", "103", "104", "105", "106", "107", "108", "109"};
    for (int i = 0; i < 20; i++)
        tm.Put(str_arr[i]);
    for (int i = 0; i < 20; i++)
        tm.Update(str_arr[i], i);

    tm.Show();
    for (int i = 0; i < 20; i++)
        printf("|%s: %d| --> ", str_arr[i], *tm.Get(str_arr[i]));
    puts("\n");

    for (int i = 0; i < 10; i++)
        tm.Remove(str_arr[i]);
    tm.Show();
    for (int i = 10; i < 20; i++)
        printf("|%s: %d| --> ", str_arr[i], *tm.Get(str_arr[i]));
    puts("\n");
}

inline void ElsaCStrHashTableIteratorTestFunc()
{
    ElsaCStrHashTable<int> tm(8);
    char const *str_arr[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
                             "100", "101", "102", "103", "104", "105", "106", "107", "108", "109"};
    ElsaCStrHashTableIterator<int> it;
    it.SetTable(tm);
    for (int i = 0; i < 20; i++)
        tm.Put(str_arr[i]);
    for (int i = 0; i < 20; i++)
        tm.Update(str_arr[i], i);

    tm.Show();
    int *tmp = it.First();
    while (tmp != nullptr)
    {
        printf("%d ", *tmp);
        tmp = it.Next();
    }
    puts("\n");
}
#endif