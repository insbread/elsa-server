/***
 * @Author: insbread
 * @Date: 2022-07-14 22:19:46
 * @LastEditTime: 2022-07-14 22:20:46
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/memory/elsac_memory_counter.hpp
 * @版权声明
 */

#include "memory/elsac_memory_counter.hpp"

#ifdef _MEMORY_TRACE_

ElsaCAllocatorCounterItem::ElsaCAllocatorCounterItem(const char *namestr)
{
    m_allocTotal = 0;
    m_freeCount = 0;
    m_usedCount = 0;
    if (!namestr)
    {
        m_cpName[0] = 0;
    }
    else
    {
        _STRNCPY_S(m_cpName, namestr, sizeof(m_cpName) - 1);
        m_cpName[sizeof(m_cpName) - 1] = 0;
    }

    ElsaCMemoryCounter::GetSingleton().Regist(this);
}

ElsaCAllocatorCounterItem::~ElsaCAllocatorCounterItem()
{
    ElsaCMemoryCounter::GetSingleton().Unregist(this);
}

ElsaCMemoryCounter::ElsaCMemoryCounter()
{
    CheckDate();
}

ElsaCMemoryCounter::~ElsaCMemoryCounter()
{
    Clear();
}

void ElsaCMemoryCounter::Regist(ElsaCAllocatorCounterItem *item)
{
    m_allocatorList.Add(item);
}

void ElsaCMemoryCounter::Unregist(ElsaCAllocatorCounterItem *item)
{
    for (int i = m_allocatorList.GetCount() - 1; i >= 0; --i)
    {
        if (item == m_allocatorList[i])
        {
            m_allocatorList.Remove(i);
            break;
        }
    }
}

void ElsaCMemoryCounter::LogToFile()
{
    FILE *fp = fopen("memory.txt", "w");
    if (!fp)
        return;

    size_t total = 0, used = 0;
    char buf[1024];

    for (int i = 0; i < m_allocatorList.GetCount(); i++)
    {
        m_allocatorList[i]->Log(buf, sizeof(buf) - 1);
        //记录到文件中
        printf("%s\n", buf);
        fputs(buf, fp);
        fputs("\n", fp);

        total += m_allocatorList[i]->m_allocTotal;
        used += m_allocatorList[i]->m_usedCount;
    }

    const char *fmt = "all: alloc total:%llu, used:%llu";
    SNPRINTFA(buf, sizeof(buf) - 1, fmt, total, used);
    printf("%s\n", buf);
    fputs(buf, fp);
    fputs("\n", fp);

    fclose(fp);
}

#endif