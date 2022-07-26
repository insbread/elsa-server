/***
 * @Author: insbread
 * @Date: 2022-07-17 17:12:12
 * @LastEditTime: 2022-07-17 17:12:12
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/elsac_x_tick.h
 * @版权声明
 */
#pragma once
#include "macro/os_def.h"

#define _getTickCount ElsaNTick64::GetTickCountEx

namespace ElsaNTick64
{
    //取机器运行时间的函数，函数返回的是毫秒值，
    //如果硬件支持查询PerformanceCounter则可避免49天的整数溢出的问题
    /***
      实际测试后此函数性能较差，因为查询CPU中断会导致进入内核并可能挂起
      r3级的线程。此外在多处理器中如果各个处理器之间频率有误差，则可能
      导致由不同线程的连续的两次调用，后者取得的时间值比前者要晚的问题。
    ***/
#ifdef _MTICK64
    typedef int64_t TICKCOUNT64;
    typedef TICKCOUNT64 TICKCOUNT;
#define GetTickCountEx GetTickCount64
    TICKCOUNT64 GetTickCount64();
#else
    typedef int64_t TICKCOUNT;
// typedef TICKCOUNT32	TICKCOUNT;
#define GetTickCountEx GetTickCount32
    TICKCOUNT GetTickCount32();
#endif
};