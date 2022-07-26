/***
 * @Author: insbread
 * @Date: 2022-07-19 21:53:51
 * @LastEditTime: 2022-07-19 21:53:51
 * @LastEditors: insbread
 * @Description: 使用内存管理器扩展了一下List链表,
 * @FilePath: /elsa-server/SDK/include/container/elsac_linked_list_ex.h
 * @版权声明
 */

#pragma once
#include "container/elsac_linked_list.h"

namespace ElsaNContainer
{
    // 对原来的链表在内存的分配上做进一步扩展，能够控制分配器一次性分配的空间
    template <typename DATA, int ONE_TIME_COUNT = 1024>
    class ElsaCLinkedListEx : public ElsaCBaseLinkedList<DATA>
    {
        typedef ElsaCObjPool<ElsaCLinkedNode<DATA>, ONE_TIME_COUNT> LinkNodeMgr;

    private:
        static LinkNodeMgr *m_pNodePool;

    public:
        virtual ~ElsaCLinkedListEx()
        {
            this->Clear();
        }

    protected:
        virtual ElsaCLinkedNode<DATA> *AllocNode()
        {
            ElsaCLinkedNode<DATA> *result = m_pNodePool->Alloc();
            return result;
        }

        virtual void FreeNode(ElsaCLinkedNode<DATA> *pNode)
        {
            m_pNodePool->Free(pNode);
        }
    };
};
