/***
 * @Author: insbread
 * @Date: 2022-07-13 22:30:06
 * @LastEditTime: 2022-07-13 22:30:06
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/macro/os_def.h
 * @版权声明
 */
#ifndef _OS_DEF_H_
#define _OS_DEF_H_
/*
 * 定义跨平台需要用的一些数据类型定义等
 * vc并不包含stdint.h文件直到2010版本，这里使用了一个网上找的替代
 */

#define _MEMORY_TRACE_

#ifdef _WIN32_WINNT
#if (_WIN32_WINNT < 0x0400)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#else
#define _WIN32_WINNT 0x0400
#endif

#define __STDC_LIMIT_MACROS

#ifdef _MSC_VER

#include <Windows.h>
#include <dbghelp.h>

#define SleepEx(x) Sleep(x * 1000)

#if _MSC_VER < 1600
#include "win/stdint.h"
#else
#include <stdint.h>
#endif // end _MSC_VER < 1600

#else

#include <stdint.h>
#include "unistd.h"
#define Sleep(x) usleep(x * 1000)
#define SleepEx(x) sleep(x)
#define TRUE 1
#define FALSE 0
#define _T
#define _I64_MAX LONG_MAX
typedef int BOOL;
#define ZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#endif // end _MSC_VER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdarg.h"
#include <assert.h>
#include <limits.h>
//为兼容之前代码额外定义的一些数据类型，因首先使用stdint.h里定义的类型
typedef char Int8;
typedef unsigned char UInt8;
typedef unsigned char BYTE;
typedef short Int16;
typedef unsigned short UInt16;
typedef int Int32;
typedef unsigned int UInt32;
typedef unsigned int UINT32;
typedef unsigned int UINT;
typedef long long int Int64;
typedef long long int INT64;
typedef uint64_t UInt64;
typedef uint64_t Uint64;
typedef uint64_t UINT64;

#define MAKEINT16(a, b) ((uint16_t)(((uint8_t)((uint16_t)(a)&0xff)) | ((uint16_t)((uint8_t)((uint16_t)(b)&0xff))) << 8))
#define LOINT8(a) ((uint8_t)((uint16_t)(a)&0xff))
#define HIINT8(a) ((uint8_t)((uint16_t)(a) >> 8))

#define MAKEINT32(a, b) ((uint32_t)(((uint16_t)((uint32_t)(a)&0xffff)) | ((uint32_t)((uint16_t)((uint32_t)(b)&0xffff))) << 16))
#define LOINT16(a) ((uint16_t)((uint32_t)(a)&0xffff))
#define HIINT16(a) ((uint16_t)((uint32_t)(a) >> 16))

#define MAKEINT64(a, b) ((uint64_t)(((uint32_t)((uint64_t)(a)&0xffffffff)) | ((uint64_t)((uint32_t)((uint64_t)(b)&0xffffffff))) << 32))
#define LOINT32(a) ((uint32_t)((uint64_t)(a)&0xffffffff))
#define HIINT32(a) ((uint32_t)((uint64_t)(a) >> 32))

#define __max(a, b) (((a) > (b)) ? (a) : (b))
#define __min(a, b) (((a) < (b)) ? (a) : (b))

#ifndef _MSC_VER

#if defined(_M_X64) || defined(__x86_64__)
typedef long long int INT_PTR; // INT_PTR在windows里定义了，不过linux下是没有的，增加这个定义兼容原来的代码
typedef unsigned long long int UINT_PTR;
#else
typedef int INT_PTR;
typedef unsigned int UINT_PTR;
#endif

#endif

#endif
