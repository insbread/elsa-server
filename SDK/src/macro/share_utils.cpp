/***
 * @Author: insbread
 * @Date: 2022-07-13 22:32:15
 * @LastEditTime: 2022-07-13 22:32:15
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/macro/share_utils.cpp
 * @版权声明
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macro/os_def.h"
#include "macro/share_utils.h"
#include "elsac_second_time.h"
#include <sys/time.h>

#ifdef _MSC_VER
#include <locale.h>
#else
#include "pthread.h"
#endif

#ifdef _MSC_VER
RTL_CRITICAL_SECTION g_OutputMsgLock;
#define InitMutex(x) InitializeCriticalSection(x)
#define DestroyMutex(x) DeleteCriticalSection(x)
#define Lock(x) EnterCriticalSection(x)
#define UnLock(x) LeaveCriticalSection(x)
static int wExceptionDumpFlag = -1; //这个是生成dump的标记
#else
pthread_mutex_t g_OutputMsgLock = PTHREAD_MUTEX_INITIALIZER;
#define InitMutex(x) pthread_mutex_init(x, NULL)
#define DestroyMutex(x) pthread_mutex_destroy(x)
#define Lock(x) pthread_mutex_lock(x)
#define UnLock(x) pthread_mutex_unlock(x)
#endif

SHAREOUTPUTMSGFN g_OutputMsgFn;

static char tempMsgBuf[8192 * 4];
const int TMPBUFSIZE = (sizeof(tempMsgBuf) / sizeof(tempMsgBuf[0]));

// 程序限制的使用时间
void CheckDate()
{
#ifndef _MSC_VER
    ElsaCSecondTime now = ElsaCSecondTime::Now();
    ElsaCSecondTime check = check.Encode(2099, 12, 31, 0, 0, 0);
    if (now > check)
    {
        assert(false);
    }
#endif
}

//默认消息输出函数
int STDCALL StdDefOutputMsg(SHAREOUTPUTMSGTYPE MsgType, const char *msg, int)
{
    time_t timep;
    struct tm *sysTime;
    time(&timep);
    sysTime = localtime(&timep);

    int ret = printf(("[%d-%d-%d %d:%d:%d]"),
                     1900 + sysTime->tm_year, sysTime->tm_mon + 1, sysTime->tm_mday, sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec);

    switch (MsgType)
    {
    case rmWarning:
        ret += printf(("[WRN]:"));
        break;

    case rmTip:
        ret += printf(("[TIP]:"));
        break;

    case rmError:
        ret += printf(("[ERR]:"));
        break;

    default:
        ret += printf((":"));
        break;
    }

    printf(("%s\n"), msg);

    return ret;
}

//全局消息输出函数
int STDCALL OutputMsg(SHAREOUTPUTMSGTYPE MsgType, const char *format, ...)
{
    va_list args;
    int ret = 0;

    if (g_OutputMsgFn)
    {
        Lock(&g_OutputMsgLock);

        va_start(args, format);
        char buf[1024];
        _STRNCPY_S(buf, format, sizeof(buf) - 2);
        buf[sizeof(buf) - 2] = 0;
        _STRNCAT_A(buf, " ");
        size_t len = __min(strlen(format) + 1, sizeof(buf) - 1);
        buf[len] = 0;
        ret = VSNPRINTFA(tempMsgBuf, TMPBUFSIZE - 1, buf, args);
        va_end(args);

        ret = g_OutputMsgFn(MsgType, tempMsgBuf, ret);

        UnLock(&g_OutputMsgLock);
    }

    return ret;
}

int STDCALL OutputError(const int errcode, const char *format, ...)
{
    int ret = 0;
    va_list args;

    if (g_OutputMsgFn)
    {
        Lock(&g_OutputMsgLock);

        char *sptr = tempMsgBuf;

        va_start(args, format);
        sptr += VSNPRINTFA(tempMsgBuf, TMPBUFSIZE - 1, format, args);
        va_end(args);

        size_t len = TMPBUFSIZE - 1 - (sptr - tempMsgBuf);
        sptr += SNPRINTFA(sptr, (int)len, (" %d "), errcode);

        ret = (int)(TMPBUFSIZE - 1 - (sptr - tempMsgBuf));
#ifdef _MSC_VER
        GetSysErrorMessage(errcode, sptr, ret, &len);
#else
        len = SNPRINTFA(sptr, ret, "%s", strerror(errcode));
#endif
        sptr += __min((int)len, ret);

        ret = int(sptr - tempMsgBuf);
        ret = g_OutputMsgFn(rmError, tempMsgBuf, ret);

        UnLock(&g_OutputMsgLock);
    }

    return ret;
}

void STDCALL InitDefMsgOut()
{
    //	全局消息上锁
    InitMutex(&g_OutputMsgLock);

    g_OutputMsgFn = StdDefOutputMsg;

#ifdef _MSC_VER
    //设置CTYPE为本地代码页，防止出现无法打印UNICODE 255之后的字符的问题。
    char sACP[16];
    SNPRINTFA(sACP, sizeof(sACP), ".%d", GetACP());
    setlocale(LC_CTYPE, sACP);
#endif
}

void STDCALL UninitDefMsgOut()
{
    DestroyMutex(&g_OutputMsgLock);
}

//设置全局消息输出函数,返回当前的消息输出函数
SHAREOUTPUTMSGFN STDCALL SetOutputMsgFn(SHAREOUTPUTMSGFN fn)
{
    Lock(&g_OutputMsgLock);
    SHAREOUTPUTMSGFN curfn = g_OutputMsgFn;
    g_OutputMsgFn = fn;
    UnLock(&g_OutputMsgLock);

    return curfn;
}

int SNPRINTFA(char *dst, int len, const char *format, ...)
{
    if (NULL == dst || len <= 0 || NULL == format)
        return -1;

    va_list va;
    va_start(va, format);

    int ret = vsnprintf(dst, len, format, va);

    if (ret >= len)
        ret = -1;

    va_end(va);

    dst[len - 1] = '\0';

    return ret;
}

int VSNPRINTFA(char *dst, int len, const char *format, va_list args)
{
    if (NULL == dst || len <= 0 || NULL == format)
        return -1;

    int ret = vsnprintf(dst, len, format, args);

    if (ret >= len)
        ret = -1;

    dst[len - 1] = '\0';

    return ret;
}

// 本函数代码出自 openbsd 的 strlcpy.c 文件，用于非bsd并且不提供strlcpy函数的系统
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0)
    {
        while (--n != 0)
        {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0)
    {
        if (siz != 0)
            *d = '\0'; /* NUL-terminate dst */
        while (*s++)
            ;
    }

    return (s - src - 1); /* count does not include NUL */
}

// 本函数代码出自 openbsd 的 strlcat.c 文件，用于非bsd并且不提供strlcat函数的系统
size_t
strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return (dlen + strlen(s));
    while (*s != '\0')
    {
        if (n != 1)
        {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src)); /* count does not include NUL */
}

void GetSystemTime(SYSTEMTIME &sysTime)
{
#ifdef _MSC_VER
    SYSTEMTIME nowSysTime; // 服务器的开启时间
    GetLocalTime(&nowSysTime);
    sysTime.sec_ = nowSysTime.wSecond;
    sysTime.min_ = nowSysTime.wMinute;
    sysTime.hour_ = nowSysTime.wHour;
    sysTime.mday_ = nowSysTime.wDay;
    sysTime.mon_ = nowSysTime.wMonth;
    sysTime.year_ = nowSysTime.wYear;
    sysTime.wday_ = nowSysTime.wDayOfWeek;
#else
    time_t lcurtime;
    time(&lcurtime);
    struct tm *curTime = localtime(&lcurtime);
    sysTime.wSecond = curTime->tm_sec;
    sysTime.wMinute = curTime->tm_min;
    sysTime.wHour = curTime->tm_hour;
    sysTime.wDay = curTime->tm_mday;
    sysTime.wMonth = curTime->tm_mon + 1;
    sysTime.wYear = curTime->tm_year + 1900;
    sysTime.wDayOfWeek = curTime->tm_wday;
#endif
}

void GetSystemTimeFull(SYSTEMTIME &sysTime)
{
    time_t lcurtime;
    time(&lcurtime);
    struct tm *curTime = localtime(&lcurtime);
    sysTime.wSecond = curTime->tm_sec;
    sysTime.wMinute = curTime->tm_min;
    sysTime.wHour = curTime->tm_hour;
    sysTime.wDay = curTime->tm_mday;
    sysTime.wMonth = curTime->tm_mon + 1;
    sysTime.wYear = curTime->tm_year + 1900;
    sysTime.wDayOfWeek = curTime->tm_wday;
    // sysTime.wday_ = curTime->tm_wday;
    struct timeval tv;
    gettimeofday(&tv, NULL);                   // NULL is only legal value
    sysTime.wMilliseconds = tv.tv_usec / 1000; // Get Millisecond;
}

int StrToInt(const char *sText)
{
    if (!sText)
        return 0;

    return atoi(sText);
}

long long int StrToInt64(const char *sText)
{
    if (!sText)
        return 0;

    long long int ret = 0;
#ifdef _MSC_VER
    sscanf(sText, "%I64d", &ret); // Unix中的libc库使用%lld
#else
    sscanf(sText, "%lld", &ret); // Unix中的libc库使用%lld
#endif
    return ret;
}

// 15位编码：
//第一、第二数字表示公民所在地的省份（或自治区、直辖市）；
//第三、第四位数字表示公民所在地的市（或州）；
//第五、第六位数字表示公民所在地的县（或县级市）；
//第七至第十二位数字表示公民的出生年、月、日；
//第十三、十四位数字表示户籍管理派出所在其所属地市的编码；
//第十五位是性别编码，单数为男，双数为女。
//
// 18位编码：
//第一、第二数字表示公民所在地的省份（或自治区、直辖市）；
//第三、第四位数字表示公民所在地的市（或州）；
//第五、第六位数字表示公民所在地的县（或县级市）；
//第七至第十四位数字表示公民的出生年、月、日；
//第十五、十六位数字表示户籍管理派出所在其所属地市的编码；
//第十七位是性别编码，单数为男，双数为女；
//第十八位是一个随机数，又称为校验码（把前17位代入公式计算得出,0-10,如果是10用x表

bool CheckIdentity(const char *id, bool *enough18)
{
    int y, m, d;
    int len = (int)strlen(id);

    if (len == 15)
    {
        const char *str = id;
        const char *endstr = str + 15;

        while (str != endstr)
            if (*str < 48 || *str++ > 57)
                return false;

        char tmp[5];

        tmp[0] = id[6];
        tmp[1] = id[7];
        tmp[2] = 0;
        y = atoi(tmp);

        if (y < 0 || y > 99)
            return false;

        tmp[0] = id[8];
        tmp[1] = id[9];
        tmp[2] = 0;
        m = atoi(tmp);

        if (m < 1 || m > 12)
            return false;

        tmp[0] = id[10];
        tmp[1] = id[11];
        tmp[2] = 0;
        d = atoi(tmp);

        if (d < 1 || d > 31)
            return false;
    }
    else if (len == 18)
    {
        const char *str = id;
        const char *endstr = str + 17;

        while (str != endstr)
            if (*str < 48 || *str++ > 57)
                return false;

        if (id[17] != 'x' && id[17] != 'X' && (id[17] < 48 || id[17] > 57))
            return false;

        char tmp[5];

        tmp[0] = id[6];
        tmp[1] = id[7];
        tmp[2] = id[8];
        tmp[3] = id[9];
        tmp[4] = 0;
        y = atoi(tmp);

        if (y < 1900 || y > 2020)
            return false;

        y -= 1900;

        tmp[0] = id[10];
        tmp[1] = id[11];
        tmp[2] = 0;
        m = atoi(tmp);

        if (m < 1 || m > 12)
            return false;

        tmp[0] = id[12];
        tmp[1] = id[13];
        tmp[2] = 0;
        d = atoi(tmp);

        if (d < 1 || d > 31)
            return false;

        // 校验码
        static const int weight[17] = {7, 9, 10, 5, 8, 4, 2, 1, 6, 3, 7, 9, 10, 5, 8, 4, 2};
        static const char *parity = "10X98765432";
        static const char *parity_ex = "10x98765432";
        int s = 0;

        for (int i = 0; i < 17; i++)
        {
            s += (id[i] - 48) * weight[i];
        }

        s = s % 11;

        if (id[17] != parity[s] && id[17] != parity_ex[s])
            return false;
    }
    else
    {
        return false;
    }

    time_t now_t;
    time(&now_t);
    struct tm *tt = localtime(&now_t);

    *enough18 = true;

    int delta_y = tt->tm_year - y;

    if (delta_y > 18)
        return true;

    if (delta_y == 18)
    {
        int delta_m = tt->tm_mon + 1 - m;

        if (delta_m > 0)
            return true;

        if (delta_m == 0 && (tt->tm_mday >= d))
            return true;
    }

    *enough18 = false;

    return true;
}

//字符串hash函数
unsigned int hashstr(const char *str)
{
    return hashlstr(str, strlen(str));
}

//字符串hash函数，需要提供长度
unsigned int hashlstr(const char *str, size_t len)
{
    unsigned int h = (unsigned int)len;
    size_t step = (len >> 5) + 1; /* if string is too long, don't hash all its chars */
    size_t l1;

    for (l1 = len; l1 >= step; l1 -= step) /* compute hash */
        h = h ^ ((h << 5) + (h >> 2) + (unsigned char)str[l1 - 1]);

    return h;
}

void OutputWatchFile(const char *str, const char *fn)
{
    if (!str)
        return;

#ifndef _MSC_VER

    if (!fn)
        fn = "/usr/local/server_err";

    // 如果执行sql错误较多，则报警
    FILE *fp = fopen(fn, "a");

    if (fp != NULL)
    {
        SYSTEMTIME sysTime;
        GetSystemTime(sysTime);
        char sTimeStr[64];
        SNPRINTFA(sTimeStr, sizeof(sTimeStr),
                  ("[%02d-%02d-%02d %02d:%02d:%02d]"),
                  sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wDay, sysTime.wMinute, sysTime.wSecond);
        fputs(sTimeStr, fp);
        fputs(str, fp);
        fputs("\n", fp);

        fclose(fp);
    }

#endif
}
#ifndef _MSC_VER
void Fork()
{
    // 创建守护进程
    pid_t pid = fork();

    if (pid < 0)
    {
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0);
    }

    setsid();
    pid = fork(); // 第二次fork

    if (pid < 0)
    {
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0);
    }
}
#else

const char *exceptionDumpFile = NULL;

void SetMiniDumpFlag(int nFlag, const char *dump_file)
{
    wExceptionDumpFlag = nFlag;
    exceptionDumpFile = dump_file;
}

long WINAPI DefaultUnHandleExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
    assert(exceptionDumpFile);

    char dmp_file[256];
    _STRNCPY_A(dmp_file, exceptionDumpFile);

    int pos = (int)strlen(dmp_file);

    for (; pos >= 0; pos--)
    {
        if (dmp_file[pos] == '.')
        {
            break;
        }
    }

    if (pos < 0)
    {
        pos = 0;
    }

    time_t timep;
    struct tm *sysTime;
    time(&timep);
    sysTime = localtime(&timep);

    SNPRINTFA(&dmp_file[pos], sizeof(dmp_file), ("-%d-%d-%d-%d-%d-%d"),
              1900 + sysTime->tm_year, sysTime->tm_mon + 1, sysTime->tm_mday, sysTime->tm_hour, sysTime->tm_min, sysTime->tm_sec);

    _STRNCAT_A(dmp_file, (".dmp"));

    HANDLE hFile = CreateFile(dmp_file, GENERIC_READ | GENERIC_WRITE,
                              0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    MINIDUMP_TYPE wDumpFlag = MiniDumpWithFullMemory;

    if (wExceptionDumpFlag >= 0)
    {
        wDumpFlag = (MINIDUMP_TYPE)wExceptionDumpFlag;
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        MINIDUMP_EXCEPTION_INFORMATION ExInfo;

        ExInfo.ThreadId = GetCurrentThreadId();
        ExInfo.ExceptionPointers = ExceptionInfo;
        ExInfo.ClientPointers = NULL;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                          hFile, wDumpFlag, &ExInfo, NULL, NULL);
        CloseHandle(hFile);
    }

    // ExitProcess( -1 );
    return EXCEPTION_EXECUTE_HANDLER;
}

const char *GetSysErrorMessage(const int ErrorCode, char *sBuffer, size_t dwBufferSize, size_t *dwBufferNeeded)
{
    char Buf[2048];

    int dwLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              NULL, (DWORD)ErrorCode, 0, Buf, sizeof(Buf), NULL);

    while (dwLen > 0)
    {
        if ((uint8_t)Buf[dwLen - 1] <= 0x32)
            dwLen--;
        else
            break;
    }

    Buf[dwLen] = 0;

    if (sBuffer)
    {
        if (dwBufferSize > dwLen)
        {
            memcpy(sBuffer, Buf, dwLen * sizeof(Buf[0]));
            sBuffer[dwLen] = 0;
        }
        else
        {
            memcpy(sBuffer, Buf, dwBufferSize * sizeof(Buf[0]));
            sBuffer[dwBufferSize - 1] = 0;
        }
    }

    *dwBufferNeeded = dwLen;
    return sBuffer;
}

#endif
