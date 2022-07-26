/***
 * @Author: insbread
 * @Date: 2022-07-15 20:09:07
 * @LastEditTime: 2022-07-15 20:09:08
 * @LastEditors: insbread
 * @Description: 对锁进行简单的封装
 * @FilePath: /elsa-server/SDK/include/elsac_x_lock.h
 * @版权声明
 */

#pragma once

#include <pthread.h>

namespace ElsaNLock
{
    class ElsaCMutex
    {
    private:
        pthread_mutex_t m_lock;

    public:
        ElsaCMutex();
        virtual ~ElsaCMutex();

        void Lock();
        void Unlock();
    };
};