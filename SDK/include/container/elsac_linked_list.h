/***
 * @Author: insbread
 * @Date: 2022-07-19 16:24:17
 * @LastEditTime: 2022-07-19 16:24:17
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/container/elsac_linked_list.h
 * @版权声明
 */

#pragma once
#include <assert.h>
#include "memory/elsac_objpool.hpp"

// #define TEST_DEBUG
namespace ElsaNContainer
{
    template <typename T>
    class ElsaCBaseLinkedList;

    template <typename T>
    class ElsaCListIterator;

    // 链表结点类（双向链表结点）
    template <typename T>
    class ElsaCLinkedNode
    {
    public:
        ElsaCLinkedNode<T> *m_pPrev;
        ElsaCLinkedNode<T> *m_pNext;
        ElsaCBaseLinkedList<T> *m_pSelf; //  指向结点所在链表的指针

        T m_data; //  存放的数据

    public:
        ElsaCLinkedNode()
        {
            m_pPrev = nullptr;
            m_pNext = nullptr;
            m_pSelf = nullptr;
        }

        inline operator T &()
        {
            return m_data;
        }
    };

    /*
        链表迭代器，用于遍历某个链表的迭代器。

        注意对于同一个数据链表，所有的迭代器也会连接成为一个迭代器链表，
        因为如果对数据链表删除结点的时候，需要遍历所有迭代器检测是否需要修改迭代器的当前位置。
    */
    template <typename T>
    class ElsaCListIterator
    {
        friend class ElsaCBaseLinkedList<T>;

    private:
        ElsaCLinkedNode<T> *m_pEnuming;  //  当前正在遍历的结点
        ElsaCBaseLinkedList<T> *m_pList; //  正在遍历的链表
        ElsaCListIterator<T> *m_pPrev;   //  链表中的前一个迭代器链表
        ElsaCListIterator<T> *m_pNext;   //  链表中的后一个迭代器链表

    public:
        ElsaCListIterator()
        {
            m_pList = nullptr;
            m_pEnuming = nullptr;
            m_pNext = m_pPrev = nullptr;
        }
        ElsaCListIterator(ElsaCBaseLinkedList<T> &list)
        {
            m_pList = &list;
            m_pEnuming = nullptr;
            m_pNext = nullptr;
            m_pPrev = list.m_pIterator;

            if (m_pPrev)
                m_pPrev->m_pNext = this;

            list.m_pIterator = this;
        }
        ~ElsaCListIterator()
        {
            if (m_pList != nullptr)
            {
                // 从Iterator链表中移除
                if (m_pPrev != nullptr)
                    m_pPrev->m_pNext = m_pNext;
                if (m_pNext != nullptr)
                    m_pNext->m_pPrev = m_pPrev;

                m_pList = nullptr;
                m_pEnuming = nullptr;
                m_pPrev = m_pNext = nullptr;
            }
        }

        // 设置需要遍历的链表
        inline void SetList(ElsaCBaseLinkedList<T> &list)
        {
            this->~ElsaCListIterator();
            m_pList = &list;
            m_pEnuming = nullptr;
            m_pNext = nullptr;
            m_pPrev = list.m_pIterator;

            if (m_pPrev != nullptr)
                m_pPrev->m_pNext = this;
            list.m_pIterator = this;
        }
        // 利用迭代器移除一个节点，方便在迭代的过程中移除节点从而避免破坏迭代过程，会返回当前迭代器的前一个结点
        inline void Remove(ElsaCLinkedNode<T> *pNode, bool boFree = true)
        {
            if (pNode == m_pEnuming)
                m_pEnuming = pNode->m_pPrev;
            m_pList->Remove(pNode, boFree);
        }
        //  返回第一个节点
        inline ElsaCLinkedNode<T> *First()
        {
            m_pEnuming = m_pList->First();
            return m_pEnuming;
        }
        //  返回下一个结点
        inline ElsaCLinkedNode<T> *Next()
        {
            m_pEnuming = (m_pEnuming ? m_pEnuming->m_pNext : m_pList->First());
            return m_pEnuming;
        }
        //  返回当前遍历的结点
        inline ElsaCLinkedNode<T> *Current()
        {
            return m_pEnuming;
        }
    };
    /*
        链表类，操作类似vector，但是在插入删除的时间复杂度比较低，但是花费的内存比较多，同时无法随机存储。
    */
    template <typename T>
    class ElsaCBaseLinkedList
    {
        friend class ElsaCListIterator<T>;

    public:
        typedef T InstancesType;
        typedef ElsaCLinkedNode<T> NodeType;
        typedef ElsaCListIterator<T> Iterator;

    private:
        ElsaCLinkedNode<T> *m_pFirstNode;               // 首结点
        ElsaCLinkedNode<T> *m_pLastNode;                // 尾结点
        Iterator *m_pIterator;                          // 关于当前链表的迭代器链表
        int m_iCount;                                   // 结点个数
        ElsaCObjPool<ElsaCLinkedNode<T>> *m_pAllocator; // 内存分配器

    public:
        ElsaCBaseLinkedList(ElsaCObjPool<ElsaCLinkedNode<T>> *alloc = nullptr)
        {
            m_pFirstNode = m_pLastNode = nullptr;
            m_pIterator = nullptr;
            m_iCount = 0;
            m_pAllocator = alloc;
        }

        virtual ~ElsaCBaseLinkedList()
        {
            Clear();
        }

        inline ElsaCLinkedNode<T> *First()
        {
            return m_pFirstNode;
        }
        inline ElsaCLinkedNode<T> *Last()
        {
            return m_pLastNode;
        }
        inline int Count() const
        {
            return m_iCount;
        }

        virtual ElsaCLinkedNode<T> *AllocNode()
        {
            if (m_pAllocator)
            {
                return m_pAllocator->Alloc();
            }
            else
            {
                return new ElsaCLinkedNode<T>();
            }
        }
        virtual void FreeNode(ElsaCLinkedNode<T> *pNode)
        {
            if (m_pAllocator)
            {
                m_pAllocator->Free(pNode);
            }
            else
            {
                delete pNode;
            }
        }

        // 在node前插入一个结点，如果是nullptr则插入在最前面
        ElsaCLinkedNode<T> *LinkBefore(const T &data, ElsaCLinkedNode<T> *node = nullptr)
        {
            assert(node == nullptr || node->m_pSelf == this);
            if (node == nullptr)
                node = m_pFirstNode;
            ElsaCLinkedNode<T> *pPrev = ((node == nullptr) ? nullptr : node->m_pPrev);
            ElsaCLinkedNode<T> *newNode = AllocNode();

            // 插入双向链表中的
            newNode->m_data = data;
            newNode->m_pPrev = pPrev;
            newNode->m_pNext = node;
            newNode->m_pSelf = this;

            if (pPrev != nullptr) // 这个时候就是插入到链表的最前面了
                pPrev->m_pNext = newNode;
            else
                m_pFirstNode = newNode;

            if (node != nullptr) //  这个时候是整个链表都没有结点的时候才会出现的
                node->m_pPrev = newNode;
            else
                m_pLastNode = newNode;

            m_iCount++;
            return newNode;
        }
        ElsaCLinkedNode<T> *LinkBefore(ElsaCLinkedNode<T> *pNewNode, ElsaCLinkedNode<T> *node = nullptr)
        {
            assert(node == nullptr || node->m_pSelf == this);
            if (node == nullptr)
                node = m_pFirstNode;

            ElsaCLinkedNode<T> *prev = ((node == nullptr) ? nullptr : node->m_pPrev);

            pNewNode->m_pPrev = prev;
            pNewNode->m_pNext = node;
            pNewNode->m_pSelf = this;

            if (prev != nullptr)
                prev->m_pNext = pNewNode;
            else
                m_pFirstNode = pNewNode;

            if (node != nullptr)
                node->m_pPrev = pNewNode;
            else
                m_pLastNode = pNewNode;
            m_iCount++;

            return pNewNode;
        }
        // 在node后面插入一个结点，如果是nullptr则插入在最后面
        ElsaCLinkedNode<T> *LinkAfter(const T &data, ElsaCLinkedNode<T> *node = nullptr)
        {
            assert(node == nullptr || node->m_pSelf == this);

            if (node == nullptr)
                node = m_pLastNode;

            ElsaCLinkedNode<T> *pNext = ((node == nullptr) ? nullptr : node->m_pNext);
            ElsaCLinkedNode<T> *pNewNode = AllocNode();

            // 插入到链表中
            pNewNode->m_data = data;
            pNewNode->m_pNext = pNext;
            pNewNode->m_pPrev = node;
            pNewNode->m_pSelf = this;

            // 查看是不是插入到了最后的位置中
            if (pNext != nullptr)
                pNext->m_pPrev = pNewNode;
            else
                m_pLastNode = pNewNode;

            // 检测前一个结点是不是存在，这种情况在空链表的情况下是会出现的
            if (node != nullptr)
                node->m_pNext = pNewNode;
            else
                m_pFirstNode = pNewNode;

            pNewNode->m_pSelf = this;
            m_iCount++;
            return pNewNode;
        }
        ElsaCLinkedNode<T> *LinkAfter(ElsaCLinkedNode<T> *pNewNode, ElsaCLinkedNode<T> *node = nullptr)
        {
            assert(node == nullptr || node->m_pSelf == this);
            if (node == nullptr)
                node = m_pLastNode;

            ElsaCLinkedNode<T> *next = ((node == nullptr) ? nullptr : node->m_pNext);

            pNewNode->m_pNext = next;
            pNewNode->m_pPrev = node;
            pNewNode->m_pSelf = this;

            if (next != nullptr)
                next->m_pPrev = pNewNode;
            else
                m_pLastNode = pNewNode;

            if (node != nullptr)
                node->m_pNext = pNewNode;
            else
                m_pFirstNode = pNewNode;
            m_iCount++;
            return pNewNode;
        }

        // 插入头结点
        ElsaCLinkedNode<T> *LinkFirst(const T &data)
        {
            return LinkBefore(data, nullptr);
        }
        ElsaCLinkedNode<T> *TransferAtFirst(ElsaCLinkedNode<T> *pNewNode)
        {
            LinkBefore(pNewNode, nullptr);
        }
        // 插入尾结点
        ElsaCLinkedNode<T> *LinkLast(const T &data)
        {
            return LinkAfter(data, nullptr);
        }
        ElsaCLinkedNode<T> *TransferAtLast(ElsaCLinkedNode<T> *pNewNode)
        {
            LinkAfter(pNewNode, nullptr);
        }
        // 在pNode后面插入结点pNewNode，pNewNode为nullptr的时候插入到末尾结点，注意pNewNode必须不存在任何结点中
        ElsaCLinkedNode<T> *Transfer(ElsaCLinkedNode<T> *pNewNode, ElsaCLinkedNode<T> *pNode = nullptr)
        {
            assert(pNode == nullptr || pNode->m_pSelf == this);
            assert(pNewNode->m_pSelf == nullptr);

            if (pNode == nullptr)
                pNode = m_pLastNode;

            ElsaCLinkedNode<T> *next = ((pNode == nullptr) ? nullptr : pNode->m_pNext);

            pNewNode->m_pPrev = pNode;
            pNewNode->m_pNext = next;
            pNewNode->m_pSelf = this;

            if (next != nullptr)
                next->m_pPrev = pNewNode;
            else
                m_pLastNode = pNewNode;

            if (pNode != nullptr)
                pNode->m_pNext = pNewNode;
            else
                m_pFirstNode = pNewNode;
            m_iCount++;
            return pNewNode;
        }

        // 从链表中删除结点，注意需要遍历迭代器，将所有枚举当前结点的迭代器进行删除
        inline void Remove(ElsaCLinkedNode<T> *node, bool bFreeFlag = true)
        {
            if (node != nullptr && node->m_pSelf == this)
            {
                // 将node从链表中移除
                if (node->m_pPrev != nullptr)
                    node->m_pPrev->m_pNext = node->m_pNext;
                if (node->m_pNext != nullptr)
                    node->m_pNext->m_pPrev = node->m_pPrev;
                if (node == m_pFirstNode)
                    m_pFirstNode = node->m_pNext;
                if (node == m_pLastNode)
                    m_pLastNode = node->m_pPrev;

                if (m_pIterator != nullptr)
                {
                    // 遍历所有关于当前链表的迭代器，将所有正在枚举当前结点的迭代器进行移动
                    Iterator *it = m_pIterator;
                    while (it != nullptr)
                    {
                        if (node == it->m_pEnuming)
                        {
                            it->m_pEnuming = node->m_pPrev;
                        }

                        it = it->m_pPrev;
                    }
                }

                node->m_pSelf = nullptr;
                // 释放内存
                if (bFreeFlag == true)
                    FreeNode(node);
                m_iCount--;
                assert(m_iCount >= 0);
            }
        }
        virtual void Clear()
        {
            ElsaCLinkedNode<T> *pNode = m_pFirstNode, *pNext = nullptr;
            while (pNode != nullptr)
            {
                pNext = pNode->m_pNext;
                FreeNode(pNode);
                pNode = pNext;
            }

            m_pFirstNode = m_pLastNode = nullptr;
            m_iCount = 0;
        }

#ifdef TEST_DEBUG
        ElsaCLinkedNode<int> *Index(int idx)
        {
            if (idx < 0 && idx >= m_iCount)
                return nullptr;
            ElsaCLinkedNode<T> *pNode = m_pFirstNode;
            while (idx > 0 && pNode != nullptr)
            {
                pNode = pNode->m_pNext;
                idx--;
            }
            return pNode;
        }
#endif
    };

};

#ifdef TEST_DEBUG
using namespace ElsaNContainer;
inline void ElsaCBaseLinkedListTestFunc()
{
    ElsaCBaseLinkedList<int> tm_list;
    tm_list.LinkFirst(1);
    tm_list.LinkLast(8);
    tm_list.LinkLast(9);
    tm_list.LinkFirst(0);
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    tm_list.LinkAfter(2, tm_list.Index(1));
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    tm_list.LinkBefore(7, tm_list.Index(3));
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    ElsaCLinkedNode<int> *tm_node1 = tm_list.AllocNode();
    tm_node1->m_data = 3;
    ElsaCLinkedNode<int> *tm_node2 = tm_list.AllocNode();
    tm_node2->m_data = 6;
    tm_list.LinkAfter(tm_node1, tm_list.Index(2));
    tm_list.LinkBefore(tm_node2, tm_list.Index(4));
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    ElsaCLinkedNode<int> *tm_node3 = tm_list.AllocNode();
    tm_node3->m_data = 4;
    tm_list.Transfer(tm_node3, tm_list.Index(3));
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    tm_list.Remove(tm_node3);
    for (int i = 0; i < tm_list.Count(); i++)
        printf("%d->", tm_list.Index(i)->m_data);
    puts("");

    tm_list.Clear();
    return;
}

inline void ElsaCListIteratorTestFunc()
{
    ElsaCBaseLinkedList<int> tm_list;
    for (int i = 0; i < 10; i++)
        tm_list.LinkLast(i);

    ElsaCListIterator<int> it1(tm_list);
    ElsaCListIterator<int> it2(tm_list);
    ElsaCListIterator<int> it3(tm_list);
    ElsaCListIterator<int> it4(tm_list);
    it1.First(), it2.First(), it3.First(), it4.First();
    for (int i = 0; i < 10; i++)
    {
        printf("%d->", it1.Current()->m_data);
        it1.Next();
    }
    puts("");
    tm_list.Remove(it1.First());

    //  一旦使用迭代器进行删除之后，就需要重置迭代器，否则会出错
    it1.First(), it2.First(), it3.First(), it4.First();
    for (int i = 0; i < 9; i++)
    {
        printf("%d->", it1.Current()->m_data);
        it1.Next();
    }
    puts("");
    for (int i = 0; i < 9; i++)
    {
        printf("%d->", it2.Current()->m_data);
        it2.Next();
    }
    puts("");
    for (int i = 0; i < 9; i++)
    {
        printf("%d->", it3.Current()->m_data);
        it3.Next();
    }
    puts("");
    for (int i = 0; i < 9; i++)
    {
        printf("%d->", it4.Current()->m_data);
        it4.Next();
    }
    puts("");
}
#endif