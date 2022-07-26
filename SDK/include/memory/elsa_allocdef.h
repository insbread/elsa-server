/***
 * @Author: insbread
 * @Date: 2022-07-15 20:05:09
 * @LastEditTime: 2022-07-15 20:05:10
 * @LastEditors: insbread
 * @Description: 内存分配头文件定义
 * @FilePath: /elsa-server/SDK/include/memory/elsa_allocdef.h
 * @版权声明
 */
#pragma once
#include "macro/os_def.h"
#ifdef _MEMORY_TRACE_

typedef struct
{
    size_t m_buffSize;
    struct
    {
        const char *m_cpFn; //申请内存块的代码位置，用于调试
        int m_iLine;
    } m_al, m_fl;
    bool m_bUsingFlag;
} AllocBuffHead;

#endif
