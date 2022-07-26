/***
 * @Author: insbread
 * @Date: 2022-07-19 22:04:20
 * @LastEditTime: 2022-07-19 22:04:20
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/container/elsac_queue_list.h
 * @版权声明
 */
#pragma once

#include "container/elsac_lock_list.h"
// #define TEST_DEBUG
namespace ElsaNContainer
{
    /*
        带数据缓冲区的vector
    */
    template <typename T>
    class ElsaCQueueList : public ElsaCLockList<T>
    {
    public:
        typedef ElsaCLockList<T> Inherited;
        typedef ElsaCQueueList<T> ListClass;

    private:
        ElsaCVector<T> m_vecAppendList; //  数据缓冲区

    public:
        //  添加一个数据到缓冲区中
        inline void Append(const T &data)
        {
            this->Lock();
            m_vecAppendList.Add(data);
            this->Unlock();
        }
        //  添加一组数据到缓冲区中
        inline void AppendList(ElsaCVector<T> &list)
        {
            this->Lock();
            m_vecAppendList.AddArray(list, list.Count());
            this->Unlock();
        }
        //  添加一组数据到缓冲区中
        inline void AppendArray(T *data, int length)
        {
            this->Lock();
            m_vecAppendList.AddArray(data, length);
            this->Unlock();
        }
        //  返回缓冲区中的数量
        inline int AppendCount()
        {
            return m_vecAppendList.Count();
        }
        //  将数据从缓冲区刷进数据区
        inline void Flush()
        {
            this->Lock();
            if (m_vecAppendList.Count() > 0)
            {
                this->AddList(m_vecAppendList);
                m_vecAppendList.Trunc(0);
            }
            this->Unlock();
        }
#ifdef TEST_DEBUG
        void Show()
        {
            printf("Data Cache Len:%d, Data Len:%d\n", this->m_vecAppendList.Count(), this->Count());
        }
#endif
    };
};

#ifdef TEST_DEBUG
using namespace ElsaNContainer;
inline void ElsaCQueueListTestFunc()
{
    ElsaCQueueList<int> tm1;
    ElsaCQueueList<int> tm2;
    int tm3[10];
    ElsaNLock::ElsaCMutex mtx1;
    ElsaNLock::ElsaCMutex mtx2;
    tm1.SetLock(&mtx1);
    tm2.SetLock(&mtx2);

    for (int i = 0; i < 10; i++)
    {
        tm1.Append(i);
        tm2.Append(i + 11);
        tm3[i] = i + 21;
    }

    tm1.Show();
    tm1.Flush();
    tm1.Show();
    for (int i = 0; i < tm1.Count(); i++)
        printf("%d ", tm1[i]);
    puts("");

    tm2.Flush();
    tm1.AppendList(tm2);
    tm1.Show();
    tm1.Flush();
    tm1.Show();
    for (int i = 0; i < tm1.Count(); i++)
        printf("%d ", tm1[i]);
    puts("");

    tm1.AppendArray(tm3, 10);
    tm1.Show();
    tm1.Flush();
    tm1.Show();
    for (int i = 0; i < tm1.Count(); i++)
        printf("%d ", tm1[i]);
    puts("");
}
#endif