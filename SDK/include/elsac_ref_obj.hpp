/***
 * @Author: insbread
 * @Date: 2022-07-24 16:32:45
 * @LastEditTime: 2022-07-24 16:32:46
 * @LastEditors: insbread
 * @Description:  自动管理生命期的引用计数对象实现类
 * @FilePath: /elsa-server/SDK/include/elsac_ref_obj.hpp
 * @版权声明
 */
#pragma once

/************************************************************************

                     自动管理生命期的引用计数对象实现类

     以模板的函数提供对引用计数功能的实现，使用时直接将对象作为模板参数实
 例化CRefObject或CMTRefObject即可。实例在构造时引用计数为0。当引用计数变
 为0时会自动销毁实例。实现原理同于COM接口对象的引用计数；

   对计数对象的使用必须严格遵循获取指针后立刻addRef，不用时release。

   引用计数实现方式是多线程安全的。基于处理器原子保护锁的方式实现，增加以
 及解 除引用的时候会锁住总线以便在多个处理器之间保持缓存一致性。是依靠硬
 件实现的。

   通常情况下使用CRefObject类完成自动化管理的对象引用功能，但如果直接使用
 CRefObjectImpl模板类应当注意如下细节：
   ★在使用一个对象实例前必须调用addRef()函数以便增加引用计数；
   ★在解除对一个对象实例使用后必须调用release函数以便减少引用计数；
   ★不得使用delete销毁一个引用实例类，正确的做法是调用release解除引用；
   ★被实例化的类型只能使用new或malloc申请的内存块进行构造，不能对局部变
 量或非指针类成员的对象使用引用计数实现。

***********************************************************************/

// #define TEST_DEBUG
#include "elsac_x_lock.h"
using namespace ElsaNLock;

namespace ElsaNMisc
{
    typedef int COUNTTYPE;
    /*
        继承类型T的针对T的引用计数实现工具。
        内部会存储对其他变量对当前对象T的引用次数，提供增加引用计数，减少引用过计数且自动析构的功能。
        实例化的时候需要使用new进行实例化。
    */
    template <typename T>
    class ElsaCRefObjectImpl : public T
    {
    private:
        COUNTTYPE m_nRefer; //  引用计数
        ElsaCMutex m_lock;  //  由于共享所以需要上锁
    public:
        using T::T; //  继承父类的构造函数
        ElsaCRefObjectImpl() : T()
        {
            m_nRefer = 0;
        }

        // 增加引用计数，
        inline void AddRef()
        {
            m_lock.Lock();
            m_nRefer++;
            m_lock.Unlock();
        }
        // 减少引用计数，减少引用后引用计数为0则自动销毁对象自身。
        inline void Release()
        {
            m_lock.Lock();
            m_nRefer--;
            int n = m_nRefer;
            m_lock.Unlock();

            if (n <= 0) // 销毁对象
                FinalRelease();
        }

    protected:
        // 最终引用计数为0时，调用这个函数
        virtual void FinalRelease()
        {
            delete this;
        }

#ifdef TEST_DEBUG
    public:
        COUNTTYPE GetCount()
        {
            return m_nRefer;
        }
#endif
    };

    /*
        多线程安全下的引用计数对象管理类。
        内部包含ElsaCRefObjectImpl成员，该成员能够记录对某个对象的引用次数以及对次数的增减、减少以及自动析构。
        使用的时候就像智能指针一样，ElsaCRefObject<T> tm(new ElsaCRefObjectImpl<T>());这样就能实例化一个对对象T的引用计数器。
    */
    template <typename T>
    class ElsaCRefObject
    {
    protected:
        ElsaCRefObjectImpl<T> *m_pRef; //  计数实现对象
    public:
        ElsaCRefObject()
        {
            m_pRef = nullptr;
        }
        // 拷贝函数会拷贝计数对象，并增加引用计数
        ElsaCRefObject(ElsaCRefObject<T> &obj)
        {
            m_pRef = obj.m_pRef;
            if (m_pRef != nullptr)
                m_pRef->AddRef();
        }
        // 拷贝计数对象也会增加引用计数
        ElsaCRefObject(ElsaCRefObjectImpl<T> *pObj)
        {
            m_pRef = pObj;
            if (m_pRef != nullptr)
                m_pRef->AddRef();
        }
        // 析构会自动减少引用次数
        virtual ~ElsaCRefObject()
        {
            if (m_pRef != nullptr)
                m_pRef->Release();
        }

        inline ElsaCRefObjectImpl<T> *RawPtr()
        {
            return m_pRef;
        }
        inline const ElsaCRefObjectImpl<T> *RawPtr() const
        {
            return m_pRef;
        }

        inline operator T *()
        {
            return m_pRef; //  注意这里利用父类指针指向子类的特性将ElsaCRefObject转化为T
        }
        inline operator const T *()
        {
            return m_pRef;
        }

        // 赋值函数：在减少之前的引用次数(如果存在)之后，增减需要赋值的引用计数器
        inline void operator=(ElsaCRefObject<T> &obj)
        {
            if (&obj != nullptr)
            {
                // 避免自己给自己赋值
                if (obj.m_pRef == m_pRef)
                    return;

                // 接下来的逻辑：增加要拷贝的引用次数，删除原来引用计数器的计数，将引用器指向拷贝的计数器上

                if (obj.m_pRef)
                    obj.m_pRef->AddRef();

                if (m_pRef)
                    m_pRef->Release();

                m_pRef = obj.m_pRef;
            }
            else if (m_pRef != nullptr)
            {
                // 否则这里就是指向nullptr指针，相当于不再指向当前对象了
                m_pRef->Release();
                m_pRef = nullptr;
            }
        }
        inline void operator=(ElsaCRefObjectImpl<T> *pObj)
        {
            if (pObj != nullptr)
            {
                if (pObj == m_pRef)
                    return;

                if (pObj != nullptr)
                    pObj->AddRef();

                if (m_pRef != nullptr)
                    m_pRef->Release();
                m_pRef = pObj;
            }
            else if (m_pRef != nullptr)
            {
                m_pRef->Release();
                m_pRef = nullptr;
            }
        }

        inline bool operator==(const ElsaCRefObject<T> &obj) const
        {
            return obj.m_pRef == m_pRef;
        }
        inline bool operator==(const ElsaCRefObjectImpl<T> *pObj) const
        {
            return pObj == m_pRef;
        }
        // 主要目的是和0比，因为C定义的NULL本质上是0，所以需要重载这个函数
        inline bool operator==(const int n) const
        {
            if (n == 0)
                return m_pRef == nullptr;
            else
                return m_pRef != nullptr;
        }

        inline bool operator!=(const ElsaCRefObject<T> &obj) const
        {
            return obj.m_pRef != m_pRef;
        }
        inline bool operator!=(const ElsaCRefObjectImpl<T> &pObj) const
        {
            return pObj != m_pRef;
        }
        inline bool operator!=(const int n) const
        {
            if (n == 0)
                return m_pRef != nullptr;
            else
                return m_pRef == nullptr;
        }

        inline bool operator!() const
        {
            return m_pRef == nullptr;
        }
    };
};

#ifdef TEST_DEBUG
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
using namespace std;
using namespace ElsaNMisc;
class ElsaCRefObjectStudent
{
private:
    static int m_objCount;
    int name[20];
    int stu_io;

public:
    ElsaCRefObjectStudent()
    {
        m_objCount++;
        stu_io = rand() % 100;

        bzero(name, sizeof(name));
        for (int i = 0; i < 10; i++)
            name[i] = (rand() % 26 + 'a');
    }
    ElsaCRefObjectStudent(int stuio)
    {
        m_objCount++;
        stu_io = stuio;

        bzero(name, sizeof(name));
        for (int i = 0; i < 10; i++)
            name[i] = (rand() % 26 + 'a');
    }
    virtual ~ElsaCRefObjectStudent()
    {
        printf("%s-%d的析构函数执行了\n", name, stu_io);
    }

    void Show()
    {
        printf("name: %s\nstuio: %d, \nObj Count:%d\n\n", name, stu_io, m_objCount);
    }
};


inline void ElsaCRefObjectTestFunc()
{
    srand((unsigned int)time(nullptr));
    ElsaCRefObject<ElsaCRefObjectStudent> ref1(new ElsaCRefObjectImpl<ElsaCRefObjectStudent>());
    printf("Ref Count: %d\n", ref1.RawPtr()->GetCount());
    ref1.RawPtr()->Show();

    ElsaCRefObject<ElsaCRefObjectStudent> ref2 = ref1;
    printf("Ref Count: %d\n", ref1.RawPtr()->GetCount());
    ref1.RawPtr()->Show();

    ElsaCRefObject<ElsaCRefObjectStudent> ref3(new ElsaCRefObjectImpl<ElsaCRefObjectStudent>(10));
    printf("Ref Count: %d\n", ref3.RawPtr()->GetCount());
    ref3.RawPtr()->Show();
    return;
}
#endif