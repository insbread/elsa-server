/***
 * @Author: insbread
 * @Date: 2022-07-13 22:30:30
 * @LastEditTime: 2022-07-13 22:30:30
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/macro/share_utils.h
 * @版权声明
 */
#ifndef _SHAREUTIL_H_
#define _SHAREUTIL_H_

#include <time.h>
#include <stdarg.h>
#include "os_def.h"

#ifndef STDCALL
#ifdef _MSC_VER
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7)))
#define GCC_UNUSED __attribute__((unused))
#else
#define GCC_UNUSED
#endif

#define STATIC_ASSERT(Expr) typedef char UnName##__LINE__[(Expr) ? 1 : -1] GCC_UNUSED; // 静态断言
#define COMPILE_ASSERT STATIC_ASSERT

//等待并关闭线程
#ifdef _MSC_VER
#define CloseThread(ht)                    \
    if (ht)                                \
    {                                      \
        WaitForSingleObject(ht, INFINITE); \
        CloseHandle(ht);                   \
        (ht) = NULL;                       \
    }
#define itoa _itoa
#define I64FMT "%I64d"
#define os_assert assert
#else
#define itoa(n, s, x) sprintf(s, "%d", n)
#define I64FMT "%lld"
#define os_assert
#endif

#ifndef SafeDelete
#define SafeDelete(p) \
    if (p)            \
    {                 \
        delete p;     \
        p = NULL;     \
    }
#endif

//获取数组长度
#ifndef ArrayCount
#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifdef _MSC_VER
//拷贝字符串到字符数组（支持MBCS和UCS）
#ifndef _asncpy
#define _asncpy(dest, src) _tcsncpy(dest, src, sizeof(dest) / sizeof(TCHAR) - 1)
//#define strlcpy strncpy
#endif
//拷贝字符串到字符数组并添加终止字符（支持MBCS和UCS）
#ifndef _asncpyt
#define _asncpyt(dest, src)                                    \
    {                                                          \
        _tcsncpy(dest, src, sizeof(dest) / sizeof(TCHAR) - 1); \
        dest[sizeof(dest) / sizeof(TCHAR) - 1] = 0;            \
    }
#endif
#endif

//拷贝MBCS字符串到MBCS字符数组并添加终止字符
#define _STRNCPY_S strlcpy
#define _STRNCAT_S strlcat
#define _STRNCPY_A(dest, src)                 \
    {                                         \
        strlcpy((dest), (src), sizeof(dest)); \
    } // 会自动补NULL
#define _STRNCAT_A(dest, src)                 \
    {                                         \
        strlcat((dest), (src), sizeof(dest)); \
    }

//将字符串终止于指定长度
#ifndef STRNTERM
#define STRNTERM(s, l) s[l] = 0;
#endif

//取64位整数的低32位整数
#ifndef LO_INT64
#define LO_INT64(S) ((unsigned int)((uint64_t)(S)&0xffffffff))
#endif
//取64位整数的高32位整数
#ifndef HI_INT64
#define HI_INT64(S) ((unsigned int)((uint64_t)(S) >> 32))
#endif

//貌似vc里没有snprintf这个函数,提供了_snprintf代替,但gcc的snprintf和_snprintf返回值是有差异的，这个要注意
//如果格式化的长度大于提供的字节长度，_snprintf返回-1，而snprintf是返回实际的长度
#ifndef _MSC_VER
#define SNPRINTF snprintf
#define STRNCASECMP strncasecmp
#else
#define SNPRINTF _snprintf
#define STRNCASECMP _strnicmp
#endif

//超出指定长度len, 就返回值在[0-len]的格式化字符，需要对snprintf返回长度做判断的统一用这个函数，如果格式化后的字符串超过len，就会返回-1
int SNPRINTFA(char *dst, int len, const char *format, ...);

int VSNPRINTFA(char *dst, int len, const char *format, va_list args);

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

//字符串hash函数
unsigned int hashstr(const char *str);
//字符串hash函数，需要提供长度
unsigned int hashlstr(const char *str, size_t len);

// struct SystemTime
// {
// 	int sec_;     /* seconds after the minute - [0,59] */
// 	int min_;     /* minutes after the hour - [0,59] */
// 	int hour_;    /* hours since midnight - [0,23] */
// 	int mday_;    /* day of the month - [1,31] */
// 	int mon_;     /* months since January - [1,12] */
// 	int year_;    /* years */
// 	int wday_;    /* days since Sunday[0--6], Sunday:0, Monday:1 ...*/
// };

struct SYSTEMTIME
{
    unsigned short wYear;
    unsigned short wMonth;
    unsigned short wDayOfWeek;
    unsigned short wDay;
    unsigned short wHour;
    unsigned short wMinute;
    unsigned short wSecond;
    unsigned short wMilliseconds;
};

void GetSystemTime(SYSTEMTIME &sysTime);

void GetSystemTimeFull(SYSTEMTIME &sysTime);
typedef enum tagShareOutputMsgType
{
    rmNormal = 0,
    rmWarning,
    rmTip,
    rmError,
} SHAREOUTPUTMSGTYPE;

//消息输出函数类型
typedef int(STDCALL *SHAREOUTPUTMSGFN)(SHAREOUTPUTMSGTYPE MsgType, const char *msg, int len);

//全局消息输出函数
int STDCALL OutputMsg(SHAREOUTPUTMSGTYPE MsgType, const char *format, ...);
//全局错误输出函数
//删除的错误格式为：sprintf(sFormat, ...) + 错误码 + 错误描述
int STDCALL OutputError(const int errcode, const char *format, ...);
//初始化全局消息输出
void STDCALL InitDefMsgOut();
//反初始化全局消息输出
void STDCALL UninitDefMsgOut();

//设置全局消息输出函数,返回当前的消息输出函数
SHAREOUTPUTMSGFN STDCALL SetOutputMsgFn(SHAREOUTPUTMSGFN fn);

#ifdef _MSC_VER

//默认的异常处理钩子
long WINAPI DefaultUnHandleExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);
//获取系统提供的错误描述内容
const char *GetSysErrorMessage(const int ErrorCode, char *sBuffer, size_t dwBufferSize, size_t *dwBufferNeeded);
void SetMiniDumpFlag(int nFlag, const char *dump_file);
#endif

//字符串转64位整数
long long int StrToInt64(const char *sText);
//字符串转32位整数
int StrToInt(const char *sText);

#define NOW_T time(NULL)
#define TIME_CHECK(X, Y) (X) > (Y)
#define TIME_CHECK_NOW(Y) TIME_CHECK((NOW_T), (Y))

bool CheckIdentity(const char *id, bool *enough18);

#define MAKEFOUR(ch0, ch1, ch2, ch3)                                                  \
    ((unsigned int)(unsigned char)(ch0) | ((unsigned int)(unsigned char)(ch1) << 8) | \
     ((unsigned int)(unsigned char)(ch2) << 16) | ((unsigned int)(unsigned char)(ch3) << 24))

#ifndef _MSC_VER
void Fork();

#else

//#define _M_DBGFUNC_BEGIN { __try {
//#define _M_DBGFUNC_END }__except (DefaultUnHandleExceptionFilter(GetExceptionInformation()),EXCEPTION_EXECUTE_HANDLER){OutputMsg( rmTip, ("abnormal output!!!") );}}

#endif
#define _M_DBGFUNC_BEGIN
#define _M_DBGFUNC_END

void OutputWatchFile(const char *str, const char *fn = NULL);

void CheckDate();
#endif
