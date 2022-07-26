/***
 * @Author: insbread
 * @Date: 2022-07-14 16:18:24
 * @LastEditTime: 2022-07-14 16:18:25
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/elsac_second_time.cpp
 * @版权声明
 */
#include <stdio.h>
#include "macro/share_utils.h"
#include "elsac_second_time.h"

const unsigned int ElsaCSecondTime::SecOfYear[2] = {365 * SecOfDay, 366 * SecOfDay};

const unsigned int ElsaCSecondTime::SecOfMonth[2][13] = {
    {0, 31 * SecOfDay, 28 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay,
     31 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay},
    {0, 31 * SecOfDay, 29 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay,
     31 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay, 30 * SecOfDay, 31 * SecOfDay}};

const unsigned int ElsaCSecondTime::MonthDays[2][13] = {
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

ElsaCSecondTime &ElsaCSecondTime::Encode(const unsigned int year, const unsigned int mon, const unsigned int day,
                                         const unsigned int hour, const unsigned int min, const unsigned int sec)
{
    int i;
    unsigned int v = 0;

    // 从YearBase到当前所经过的秒数
    for (i = year - 1; i >= YearBase; i--)
    {
        v += SecOfYear[IsLeapYear(i)];
    }
    // 从1月到当前月份所经过的秒数
    bool boLeapYear = IsLeapYear(i);
    for (i = mon - 1; i >= 1; i--)
    {
        v += SecOfMonth[boLeapYear][i];
    }
    // 计算从某月1日开始当当月当日经过的秒数
    v += (day - 1) * SecOfDay;
    // 计算当前天经过的小时数
    v += hour * SecOfHour;
    // 计算当前小时经过的分钟数
    v += min * SecOfMin;
    //  累积秒数
    v += sec;

    m_uiTv = (m_uiTv & RecordFlag) | (v & (~RecordFlag));
    return *this;
}

void ElsaCSecondTime::Decode(tm &sysTime)
{
    unsigned int v = m_uiTv & (~RecordFlag);
    // 计算年份
    int year = YearBase;
    while (1)
    {
        unsigned int nSecOfCurYear = SecOfYear[IsLeapYear(year)];

        if (v >= nSecOfCurYear)
        {
            year++;
            v -= nSecOfCurYear;
        }
        else
        {
            break;
        }
    }
    // 计算月份
    bool isLeapYear = IsLeapYear(year);
    int month = 1;
    while (1)
    {
        unsigned int nSecOfCurMonth = SecOfMonth[isLeapYear][month];
        if (v >= nSecOfCurMonth)
        {
            month++;
            v -= nSecOfCurMonth;
        }
        else
        {
            break;
        }
    }
    // 计算日期
    int day = v / SecOfDay + 1;

    sysTime.tm_year = year;
    sysTime.tm_mon = month;
    sysTime.tm_mday = day;
    //小时、分、秒
    unsigned int SecOfToday = v % (24 * 3600);
    sysTime.tm_hour = SecOfToday / 3600;
    sysTime.tm_min = (SecOfToday % 3600) / 60;
    sysTime.tm_sec = SecOfToday % 60;
}

void ElsaCSecondTime::Decode(SYSTEMTIME &sysTime)
{
    unsigned int v = m_uiTv & (~RecordFlag);
    // 计算年份
    int year = YearBase;
    while (1)
    {
        unsigned int nSecOfCurYear = SecOfYear[IsLeapYear(year)];

        if (v >= nSecOfCurYear)
        {
            year++;
            v -= nSecOfCurYear;
        }
        else
        {
            break;
        }
    }
    // 计算月份
    bool isLeapYear = IsLeapYear(year);
    int month = 1;
    while (1)
    {
        unsigned int nSecOfCurMonth = SecOfMonth[isLeapYear][month];
        if (v >= nSecOfCurMonth)
        {
            month++;
            v -= nSecOfCurMonth;
        }
        else
        {
            break;
        }
    }
    // 计算日期
    int day = v / SecOfDay + 1;

    sysTime.wYear = year;
    sysTime.wMonth = month;
    sysTime.wDay = day;
    //小时、分、秒
    unsigned int SecOfToday = v % (24 * 3600);
    sysTime.wHour = SecOfToday / 3600;
    sysTime.wMinute = (SecOfToday % 3600) / 60;
    sysTime.wSecond = SecOfToday % 60;
}
