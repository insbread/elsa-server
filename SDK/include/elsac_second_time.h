/***
 * @Author: insbread
 * @Date: 2022-07-13 22:35:54
 * @LastEditTime: 2022-07-13 22:35:55
 * @LastEditors: insbread
 * @Description: 以秒数定义的类
 * @FilePath: /elsa-server/SDK/include/elsac_second_time.h
 * @版权声明
 */

#pragma once

#include <time.h>

#include "macro/share_utils.h"

/*
    存储从2010年到现在所经历的秒数
*/
class ElsaCSecondTime
{
public:
    // 定义短时间类型开始的年份，注意需要从这一年开始定义，因为uint只能存储不到100年的秒数，如果从1900年开始记录会导致溢出
    static const int YearBase = 2010;
    // 为方便移植性，日期类型使用tm，这个时间是以1900年的1月1日凌晨开始算
    static const int BaseYear = 1900;
    // 定义时间是否已经开始计时的标志位，意思应该是用最高位标识是否开始计时，
    // 开始之后只要取反则后面全部是1，和当前时间相与能保留时间；否则没开始计时应该是0x8000 - 1；
    static const unsigned int RecordFlag = 0x80000000;
    // 定义各种单位的时间的秒数
    static const unsigned int SecOfMin = 60;
    static const unsigned int SecOfHour = SecOfMin * 60;
    static const unsigned int SecOfDay = SecOfHour * 24;
    static const unsigned int SecOfYear[2];
    static const unsigned int SecOfMonth[2][13]; //以月为第二数组的下标，免去月份-1的操作！
    static const unsigned int MonthDays[2][13];  //以月为第二数组的下标，免去月份-1的操作！
public:
    unsigned int m_uiTv; //  存储的秒数
public:
    ElsaCSecondTime()
    {
        m_uiTv = 0;
    }
    ElsaCSecondTime(const ElsaCSecondTime &mt)
    {
        m_uiTv = mt.m_uiTv;
    }
    ElsaCSecondTime(const unsigned int tva)
    {
        m_uiTv = tva;
    }

#ifdef TEST_DEBUG
    // 仅做测试的时候使用
    void SetTime(const unsigned int tva)
    {
        this->m_uiTv = tva;
    }
#endif

public:
    // -------------------------------------- 时间的编码解码 ----------------------------------------------begin
    // 将时间转化为内部存储的秒数
    ElsaCSecondTime &Encode(const unsigned int year, const unsigned int mon, const unsigned int day,
                            const unsigned int hour, const unsigned int min, const unsigned int sec);
    inline ElsaCSecondTime &Encode(time_t sysTime)
    {
        return Encode(*localtime(&sysTime));
    }
    inline ElsaCSecondTime &Encode(const tm &sysTime)
    {
        return Encode(1900 + sysTime.tm_year, sysTime.tm_mon + 1, sysTime.tm_mday,
                      sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec);
    }
    inline ElsaCSecondTime &Encode(const SYSTEMTIME &sysTime)
    {
        return Encode(sysTime.wYear, sysTime.wMonth, sysTime.wDay,
                      sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
    }

    // 从MiniDataTime转化到系统时间。暂时只精确到天。
    void Decode(tm &sysTime);
    void Decode(SYSTEMTIME &sysTime);
    // 将当前时间解码为经过的秒数。注意：这里是借助了decode(tm)进行时间的解析的，所以如果decode(tm)发生了变化，这个函数也需要变化
    time_t Decode()
    {
        tm hehe;
        Decode(hehe);
        hehe.tm_year -= 1900;
        hehe.tm_mon = -1;
        return mktime(&hehe);
    }
    // -------------------------------------- 时间的编码解码 ----------------------------------------------end

    // -------------------------------------- 时间的相关判断 ------------------------------------------begin
    // 判断是否是同一天
    inline bool IsSameDay(const ElsaCSecondTime &rhs)
    {
        return ((m_uiTv & (~RecordFlag)) / SecOfDay) == ((rhs.m_uiTv & (~RecordFlag)) / SecOfDay) ? true : false;
    }
    // 判断是否已经开始记录时间
    inline bool IsStartRecord()
    {
        return (m_uiTv & RecordFlag) ? true : false;
    }
    //开始记录时间，timeOfNow参数表示当前时间的短时间类型值，从RecordFlag为起始点开始记录时间
    inline ElsaCSecondTime &StartRecord(const unsigned int timeOfNow)
    {
        if (!(m_uiTv & RecordFlag))
            m_uiTv = RecordFlag | ((timeOfNow & (~RecordFlag)) + m_uiTv);

        return *this;
    }
    // -------------------------------------- 时间的相关判断 ------------------------------------------end

    // -------------------------------------- 时间的相关操作 ------------------------------------------begin
    // 返回当前时间的秒数
    inline static unsigned int Now()
    {
        ElsaCSecondTime tv;
        time_t timep;
        struct tm *sysTime;
        time(&timep);
        sysTime = localtime(&timep);
        tv.Encode(1900 + sysTime->tm_year, sysTime->tm_mon + 1, sysTime->tm_mday,
                  sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec);
        return tv.m_uiTv;
    }
    // 计算今天凌晨的时间
    inline static unsigned int Today()
    {
        ElsaCSecondTime tv;
        time_t timep;
        struct tm *sysTime;
        time(&timep);
        sysTime = localtime(&timep);
        tv.Encode(1900 + sysTime->tm_year, sysTime->tm_mon + 1, sysTime->tm_mday, 0, 0, 0);
        return tv.m_uiTv;
    }
    // 计算昨天凌晨时间
    inline static unsigned int Yesterday()
    {
        return Today() - 3600 * 24;
    }
    // 计算明日凌晨的值
    inline static unsigned int Tomorrow()
    {
        return Today() + 3600 * 24;
    }
    // 计算相对于当前MiniDateTime所指示的时间点的明天
    inline unsigned int RelTomorrow()
    {
        unsigned int v = m_uiTv & (~RecordFlag);  //  移除标识位
        v = (v + SecOfDay) / SecOfDay * SecOfDay; //  计算到明天的时间
        return (m_uiTv & RecordFlag) | v;
    }
    // 计算相对于当前MiniDateTime所指示的时间点的当前开始时间点
    inline unsigned int RelToday()
    {
        unsigned int v = m_uiTv & (~RecordFlag);
        v = v / SecOfDay * SecOfDay;
        return (m_uiTv & RecordFlag) | v;
    }
    // 判断是否平年
    inline static bool IsLeapYear(const unsigned int year)
    {
        return ((((year % 4) == 0) && (year % 100 != 0)) || (year % 400 == 0));
    }
    // -------------------------------------- 时间的相关操作 ------------------------------------------end

public:
    // -------------------------------------- 时间的重载操作 ------------------------------------------begin
    // 转换为时间秒数
    inline operator unsigned int()
    {
        return m_uiTv;
    }
    // unsigned int运算符
    inline ElsaCSecondTime &operator=(const unsigned int time)
    {
        m_uiTv = time;
        return *this;
    }
    // 重载相等判断的运算符
    inline bool operator==(const unsigned int time)
    {
        return m_uiTv == time;
    }
    inline bool operator!=(const unsigned int time)
    {
        return m_uiTv != time;
    }
    inline bool operator>(const unsigned int time)
    {
        return (m_uiTv & (~RecordFlag)) > (time & (~RecordFlag));
    }
    inline bool operator>=(const unsigned int time)
    {
        return (m_uiTv & (~RecordFlag)) >= (time & (~RecordFlag));
    }
    inline bool operator<(const unsigned int time)
    {
        return (m_uiTv & (~RecordFlag)) < (time & (~RecordFlag));
    }
    inline bool operator<=(const unsigned int time)
    {
        return (m_uiTv & (~RecordFlag)) <= (time & (~RecordFlag));
    }
    // 重载算术运算操作
    inline unsigned int operator+(const unsigned int time)
    {
        return ((m_uiTv & (~RecordFlag)) + (time & (~RecordFlag))) & (~RecordFlag);
    }
    inline unsigned int operator-(const unsigned int time)
    {
        return ((m_uiTv & (~RecordFlag)) - (time & (~RecordFlag))) & (~RecordFlag);
    }
    inline ElsaCSecondTime &operator+=(const unsigned int time)
    {
        m_uiTv = (m_uiTv & RecordFlag) | (((m_uiTv & (~RecordFlag)) + (time & (~RecordFlag))) & (~RecordFlag));
        return *this;
    }
    inline ElsaCSecondTime &operator-=(const unsigned int time)
    {
        m_uiTv = (m_uiTv & RecordFlag) | (((m_uiTv & (~RecordFlag)) - (time & (~RecordFlag))) & (~RecordFlag));
        return *this;
    }
    // -------------------------------------- 时间的重载操作 ------------------------------------------end
};

#ifdef TEST_DEBUG
#include <iostream>
using std::cout;
using std::endl;

inline void ElsaCSecondTimeTestFunc()
{
    ElsaCSecondTime cur_tm;
    cout << "Now Sec: " << ElsaCSecondTime::Now() << endl;
    cout << "Yesterday Morn Sec: " << ElsaCSecondTime::Yesterday() << endl;
    cout << "Today Morn Sec: " << ElsaCSecondTime::Today() << endl;
    cout << "Tomorrow Morn Sec: " << ElsaCSecondTime::Tomorrow() << endl;
    struct tm stm;
    cur_tm.SetTime(ElsaCSecondTime::Now());
    cur_tm.Decode(stm);
    cout << "Yeas: " << stm.tm_year << ",Month: " << stm.tm_mon << ",Day: " << stm.tm_mday
         << ",Hour: " << stm.tm_hour << ",Min: " << stm.tm_min << ",Sec: " << stm.tm_sec << endl;

    cur_tm.SetTime(ElsaCSecondTime::Yesterday());
    cur_tm.Decode(stm);
    cout << "Yeas: " << stm.tm_year << ",Month: " << stm.tm_mon << ",Day: " << stm.tm_mday
         << ",Hour: " << stm.tm_hour << ",Min: " << stm.tm_min << ",Sec: " << stm.tm_sec << endl;

    cur_tm.SetTime(ElsaCSecondTime::Today());
    cur_tm.Decode(stm);
    cout << "Yeas: " << stm.tm_year << ",Month: " << stm.tm_mon << ",Day: " << stm.tm_mday
         << ",Hour: " << stm.tm_hour << ",Min: " << stm.tm_min << ",Sec: " << stm.tm_sec << endl;

    cur_tm.SetTime(ElsaCSecondTime::Tomorrow());
    cur_tm.Decode(stm);
    cout << "Yeas: " << stm.tm_year << ",Month: " << stm.tm_mon << ",Day: " << stm.tm_mday
         << ",Hour: " << stm.tm_hour << ",Min: " << stm.tm_min << ",Sec: " << stm.tm_sec << endl;
}

#endif