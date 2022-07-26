/***
 * @Author: insbread
 * @Date: 2022-07-17 17:19:59
 * @LastEditTime: 2022-07-17 17:20:00
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/elsac_x_tick.cpp
 * @版权声明
 */
#include <time.h>
#include "elsa_x_tick.h"

using namespace ElsaNTick64;

#ifdef _MTICK64
static TICKCOUNT64 UserSetTick = 0;
/***
实际测试的性能表明，基于timeGetTime实现的GetTickCount64具有较高的性能，
而基于QueryPerformanceCounter的实现则是timeGetTime版本时间消耗的300倍
***/

static MMRESULT mmPeriod = timeBeginPeriod(1);
static TICKCOUNT64 llTickBase = 0;
static TICKCOUNT64 llStartTick = timeGetTime();
static unsigned long dwLastShortTick = timeGetTime();

TICKCOUNT64 ElsaNTick64::GetTickCount64()
{
    if (UserSetTick)
        return UserSetTick;

    /*
      此处需要将dwLastShortTick导致保存为局部变量并尝试通过保存的值以锁总线的方式改变dwLastShortTick的值。
          由于在检测并修改dwLastShortTick的值的期间，当前线程可能由于进程调度而使得线程被挂起。当前线程被挂
      起的期间如果其他线程也执行了此函数则该线程会先于此线程修改dwLastShortTick的值。当本线程在恢复后再修改
      dwLastShortTick的值将导致错误的结果并会错误的判断为进入了一个49天的循环并修改llTickBase的值，从而导
      致所有后续调用的返回值全部错误的问题！
          为尽量降低数据同步的开销，不能使用临界区等方式来同步数据，故必须使用原子加锁操作指令代替。
    */
    unsigned long dwLastTick = dwLastShortTick;
    unsigned long dwTick = timeGetTime();

    if (InterlockedCompareExchange((volatile long *)&dwLastShortTick, (long)dwTick, (long)dwLastTick) == dwLastTick)
    {
        if (dwTick < dwLastTick)
        {
            if (IsDebuggerPresent())
            {
                //请手动确认此处是否未发生BUG，而是系统真正的经过了49天的循环，并按继续键……
                OutputDebugString(("请手动确认此处是否未发生BUG，而是系统真正的经过了49天的循环"));
                DebugBreak();
            }

            llTickBase += 0x1UI64 << 32;
        }
    }

    return llTickBase + dwTick - llStartTick;
}
#else
TICKCOUNT ElsaNTick64::GetTickCount32()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / (1000000));
}
#endif