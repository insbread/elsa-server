/***
 * @Author: insbread
 * @Date: 2022-07-20 15:58:07
 * @LastEditTime: 2022-07-20 15:58:07
 * @LastEditors: insbread
 * @Description:类对象的缓存池，从池中取出的就是一个可用的对象，放到池中的对象不执行析构函数，
 *              这样可避免执行构造函数和析构函数的开销，特别是需要分配内存的对象，可避免产生碎片
 * @FilePath: /elsa-server/SDK/include/container/elsac_obj_list.h
 * @版权声明
 */

#pragma once

#include <assert.h>
#include "container/elsac_vector.h"
#include "memory/elsac_buffer_allocator.h"
// #define TEST_DEBUG
namespace ElsaNContainer
{
    template <typename T, int ONE_TIME_COUNT = 1024>
    class ElsaCObjList : public ElsaCAllocatorCounterItem
    {
    private:
        ElsaCVector<T *> m_vecAll;        //  所有分配的对象
        ElsaCVector<T *> m_vecFree;       //  空闲池的对象
        int m_iAllObjCount;               //  总的对象数
        int m_iFreeCount;                 //  空闲对象数
        ElsaCBufferAllocator m_allocator; //  内存分配器
    public:
        ElsaCObjList(const char *namestr, const char *namestr2)
            : ElsaCAllocatorCounterItem(namestr), m_iAllObjCount(0), m_iFreeCount(0), m_allocator(namestr2) {}
        ~ElsaCObjList()
        {
            assert(m_iAllObjCount == m_iFreeCount);

            // 首先将空闲队列中的对象进行析构处理
            int count = m_vecFree.Count();
            T **list = m_vecFree;
            for (int i = 0; i < count; i++)
            {
                list[i]->~T(); //  定位new需要手动调用析构函数
            }

            // 然后由内存分配器将对象缓冲池中所有对象的物理内存
            count = m_vecAll.Count();
            list = m_vecAll;
            for (int i = 0; i < count; i++)
            {
                m_allocator.FreeBuffer(list[i]);
            }
            m_allocator.CheckFreeBuffers();
        }

        // 获取一个对象，并且这个对象已经初始化了，或者就是上一次的状态，所以这种对象分配之后其实还需要在外部再进行一次初始化操作。
        T *Get()
        {
            if (m_vecFree.Count() <= 0)
            {
                // 没有空闲对象，那么就一次性分配ONE_TIME_COUNT个对象
                T *newObj = m_allocator.AllocBuffer(sizeof(T) * ONE_TIME_COUNT);
                //  将分配的内存存放到空闲队列中
                m_vecAll.Add(newObj);
                m_iAllObjCount += ONE_TIME_COUNT;
                m_vecFree.reverse(m_iAllObjCount);

                // 将构造出来的对象进行初始化构造
                for (int i = 0; i < ONE_TIME_COUNT; i++)
                {
                    new (newObj) T();
                    m_vecFree[i] = newObj; //  注意这里必须是拷贝，而不能是Add，否则就不是对象的拷贝了，而是01拷贝了
                    newObj++;
                }

                m_iFreeCount += ONE_TIME_COUNT;
                m_vecFree.Trunc(m_iFreeCount);
            }

            // 从空闲队列中分配数据出去
            assert(m_iFreeCount == m_vecFree.Count());
            --m_iFreeCount;
            T *result = m_vecFree[m_iFreeCount];
            m_vecFree.Trunc(m_iFreeCount);
            return result;
        }
        // 回收一个对象，这个对象不会执行析构函数。
        void Release(T *p)
        {
            if (p == nullptr)
                return;
            m_vecFree.Add(p);
            m_iFreeCount++;
        }

#ifdef TEST_DEBUG
        void Show()
        {
            printf("All Size: %d, Free Size: %d\n", m_vecAll.Count(), m_vecFree.Count());
            printf("All Record Size: %d, Free Record Size: %d\n\n", m_iAllObjCount, m_iFreeCount);
        }
#endif
    };
};

#ifdef TEST_DEBUG
#include <cstdlib>
#include <time.h>
using namespace ElsaNContainer;

class StudentElsaCObjList
{
private:
    char name[40];
    int stu_no;
    int obj_no;

public:
    StudentElsaCObjList()
    {
        memset(name, 0, sizeof(name));
        for (int i = 0; i < 10; i++)
            name[i] = rand() % 26 + 97;
        stu_no = rand() % 100;
        obj_no = rand() % 200;
    }
    ~StudentElsaCObjList()
    {
        printf("%s have been deleted!\n", name);
    }

    void Show()
    {
        printf("name: %s, stu_no: %d, obj_no: %d\n", name, stu_no, obj_no);
    }
};

void ElsaCObjListTestFunc()
{
    srand((unsigned int)time(nullptr));
    ElsaCObjList<StudentElsaCObjList, 2> tm("ElsaCBufferAllocator", "StudentElsaCObjList");
    StudentElsaCObjList *tmp1 = tm.Get();
    StudentElsaCObjList *tmp2 = tm.Get();
    tm.Show();

    StudentElsaCObjList *tmp3 = tm.Get();
    tm.Show();
    StudentElsaCObjList *tmp4 = tm.Get();
    tm.Show();

    tmp1->Show();
    tmp2->Show();
    tmp3->Show();
    tmp4->Show();
    puts("");

    tm.Release(tmp1);
    tm.Release(tmp2);
    tm.Show();

    tmp1 = tm.Get();
    tmp2 = tm.Get();
    tmp1->Show();
    tmp2->Show();

    ElsaCMemoryCounter::GetSingleton().LogToFile();

    tm.Release(tmp1);
    tm.Release(tmp2);
    tm.Release(tmp3);
    tm.Release(tmp4);
    return;
}
#endif