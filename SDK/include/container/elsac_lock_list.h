/***
 * @Author: insbread
 * @Date: 2022-07-19 22:04:50
 * @LastEditTime: 2022-07-19 22:04:50
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/container/elsac_lock_list.h
 * @版权声明
 */
#pragma once
#include "elsac_x_lock.h"
#include "container/elsac_vector.h"

namespace ElsaNContainer
{
    using namespace ElsaNLock;
    /*
        具有上锁功能的Vector
    */
    template <typename T>
    class ElsaCLockList : public ElsaCVector<T>
    {
    public:
        typedef ElsaCVector<T> Inherited;
        typedef ElsaCLockList<T> ListClass;

    private:
        ElsaCMutex *m_pLock;

    public:
        ElsaCLockList(ElsaCMutex *lock = nullptr) : Inherited()
        {
            m_pLock = lock;
        }
        // 获取锁
        inline ElsaCMutex *GetLock()
        {
            return m_pLock;
        }
        // 设置新的锁，返回旧的锁
        ElsaCMutex *SetLock(ElsaCMutex *lock)
        {
            ElsaCMutex *pOldMutex = m_pLock;
            m_pLock = lock;
            return pOldMutex;
        }
        // 上锁
        inline void Lock()
        {
            assert(m_pLock);
            if (m_pLock != nullptr)
                m_pLock->Lock();
        }
        //  解锁
        inline void Unlock()
        {
            if (m_pLock != nullptr)
                m_pLock->Unlock();
        }
    };
};