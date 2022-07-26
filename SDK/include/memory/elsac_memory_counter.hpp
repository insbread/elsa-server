/***
 * @Author: insbread
 * @Date: 2022-07-13 22:23:02
 * @LastEditTime: 2022-07-13 22:23:03
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/memory/elsac_memory_counter.hpp
 * @版权声明
 */

#pragma once
#include <vector>

#include "macro/os_def.h"
#include "macro/share_utils.h"

#ifdef _MEMORY_TRACE_
/*
    内存统计基础类。
    所有内存分配器都应该继承这个类，以便于统计内存的使用情况。
    实现的具体功能：返回已使用的内存空间；为新元素分配内存；移除某个字节的内存，并紧凑内存分配；自动对内存进行扩充
*/
template <typename T, int ONE_TIME_NUM = 16>
class ElsaCMemoryItemList
{
protected:
    T *m_dataPtr;   //	内存空间起始地址
    int m_iMaxSize; //	内存空间真实大小(以T为单位)
    int m_iCount;   //	已分配内存空间大小(以T为单位)

public:
    ElsaCMemoryItemList() : m_dataPtr(nullptr), m_iMaxSize(0), m_iCount(0) {}
    virtual ~ElsaCMemoryItemList()
    {
        Empty();
    }

    inline int GetCount() const
    {
        return m_iCount;
    }

    // 将内存拓展为count，如果count < m_iCount或者原来内存没有变化则不发生改变
    virtual void Reverse(int count)
    {
        if (count > m_iCount && count != m_iMaxSize)
        {
            m_iMaxSize = count;
            m_dataPtr = static_cast<T *>(realloc(m_dataPtr, sizeof(T) * count));
        }
    }
    // 将data数据写入缓冲区中，会自动扩充数据缓冲区
    int Add(const T &data)
    {
        if (m_iCount >= m_iMaxSize)
            Reverse((m_iMaxSize > 0) ? m_iMaxSize * 2 : ONE_TIME_NUM);
        memcpy(m_dataPtr + m_iCount, &data, sizeof(data));
        m_iCount++;
        return m_iCount - 1;
    }
    // 移除缓冲区中[idx ~ idx + count)的数据，即移除[idx, idx + count - 1]的数据或者说是从idx往后数count个数据
    void Remove(const int index, const int count)
    {
        assert(index + count <= m_iCount && index >= 0);
        if (count > 0)
        {
            memmove(m_dataPtr + index, m_dataPtr + index + count, sizeof(T) * (m_iCount - index - count));
            m_iCount -= count;
        }
    }
    // 移除缓冲区下标是index的数据
    void Remove(const int index)
    {
        assert(index >= 0 && index < m_iCount);
        Remove(index, 1);
    }
    // 逻辑清控缓冲区
    inline void Clear()
    {
        m_iCount = 0;
    }
    // 物理清空缓冲区
    virtual void Empty()
    {
        Clear();
        m_iMaxSize = 0;
        if (m_dataPtr)
        {
            free(m_dataPtr);
            m_dataPtr = nullptr;
        }
    }

    inline operator T *() const
    {
        return m_dataPtr;
    }
};

class ElsaCMemoryCounter;
/*
    内存日志类
    不具备内存，但是能够记录内存的使用情况，同时提供函数返回信息日志；
    每次创建构造函数会自动调用JieCMemoryCounter的regist进行登记；析构的时候自动调用unregist进行移除；
*/
class ElsaCAllocatorCounterItem
{
    friend class ElsaCMemoryCounter;

public:
    static const size_t LargeCount = 100 * 1024 * 1024;
    static const size_t MidCount = 10 * 1024 * 1024;
    enum
    {
        MAX_NAME_LENGTH = 32, // 最大描述字符串长度
    };

protected:
    size_t m_allocTotal;            // 分配的内存总数
    size_t m_freeCount;             // 空闲的内存数量
    size_t m_usedCount;             // 正在使用的内存数量
    char m_cpName[MAX_NAME_LENGTH]; // 统计器的名字
public:
    ElsaCAllocatorCounterItem(const char *namestr);
    virtual ~ElsaCAllocatorCounterItem();

    const char *GetName()
    {
        return m_cpName;
    }
    size_t GetAllocCount()
    {
        return m_allocTotal;
    }
    const char *GetTip(size_t s1, size_t s2 = 0)
    {
        const char *tip = "";

        if (s1 > LargeCount || s2 > LargeCount)
        {
            tip = "*****";
        }
        else if (s1 > MidCount || s2 > MidCount)
        {
            tip = "**";
        }

        return tip;
    }

    virtual int Log(char *buf, int num)
    {
        const char *fmt = "%s, alloc total:%llu, used:%llu %s";
        const char *tip = GetTip(m_allocTotal, m_usedCount);
        return SNPRINTFA(buf, num, fmt, m_cpName, m_allocTotal, m_usedCount, tip);
    }
};

/*
    桶内存统计器
    继承自内存统计器，所以没有内存空间，只有内存使用状况。统计的方式是将内存分为多个桶进行统计，每个桶的内存分配各自独立；
    在父类ElsaCAllocatorCounterItem的功能上，能够对多个统计器的内存进行输出统计。
    目前并没有使用，是一个仅仅做测试使用的类
*/
class ElsaCSimpleAllocCounter : public ElsaCAllocatorCounterItem
{
    static const int MaxCounter = 8;

private:
    size_t m_counter[MaxCounter]; // 多个统计器的数组，数组的元素分别是统计器中统计使用的数据量

public:
    ElsaCSimpleAllocCounter(const char *name) : ElsaCAllocatorCounterItem(name)
    {
        memset(m_counter, 0, sizeof(m_counter));
    }

    size_t Counter(size_t s, int idx)
    {
        assert(idx >= 0 && idx < MaxCounter);
        m_counter[idx] += s;
        m_allocTotal += s;
        return m_counter[idx];
    }

    virtual int Log(char *buf, int num)
    {
        const char *fmt = "%d, total:%llu %s\r\n";
        int ret = 0;

        for (int i = 0; i < MaxCounter; i++)
        {
            size_t c = m_counter[i];
            if (c <= 0)
                continue;
            const char *tip = GetTip(c);
            ret += SNPRINTFA(buf + ret, num - ret, fmt, i, c, tip);
        }
        return ret;
    }
};

/*
    内存统计管理器(单例类)。对所有注册在内部的ElsaCAllocatorCounterItem及其子类进行统计。
    注意每创建一个ElsaCAllocatorCounterItem或其子类(会在构造函数中自动调用regist，析构函数自动调用unregist)，
    都会将ElsaCAllocatorCounterItem或其子类自动注册到该类中。

    这里ElsaCMemoryCounter和ElsaCAllocatorCounterItem形成观察者模式，
    ElsaCMemoryCounter是观察者，ElsaCAllocatorCounterItem是观察目标。
*/
class ElsaCMemoryCounter
{
private:
    ElsaCMemoryItemList<ElsaCAllocatorCounterItem *, 2048> m_allocatorList; // 管理所有注册的统计项

public:
    ElsaCMemoryCounter();
    ~ElsaCMemoryCounter();

    static ElsaCMemoryCounter &GetSingleton()
    {
        static ElsaCMemoryCounter mgr;
        return mgr;
    }

    void Clear()
    {
        m_allocatorList.Clear();
        m_allocatorList.Empty();
    }
    // 添加一个统计项
    void Regist(ElsaCAllocatorCounterItem *item);
    // 移除一个统计项
    void Unregist(ElsaCAllocatorCounterItem *item);
    // 统计所有统计器的信息到日志，将所有管理的AllocatorCounterItem的数据都进行日志登录
    void LogToFile();
};

#endif

#ifdef TEST_DEBUG
inline void ElsaCAllocatorCounterItemTest()
{
    ElsaCAllocatorCounterItem tmp("Test");
    char buf[4096] = {0};
    printf(buf);
}

inline void ElsaCMemoryCounterTest()
{
    ElsaCAllocatorCounterItem tmp1("Test1");
    ElsaCAllocatorCounterItem tmp2("Test2");
    ElsaCAllocatorCounterItem tmp3("Test3");
    ElsaCAllocatorCounterItem tmp4("Test4");

    ElsaCMemoryCounter::GetSingleton().LogToFile();
    
}
#endif