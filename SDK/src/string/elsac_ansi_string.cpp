/***
 * @Author: insbread
 * @Date: 2022-07-25 16:37:39
 * @LastEditTime: 2022-07-25 16:37:39
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/string/elsac_ansi_string.cpp
 * @版权声明
 */
#include <cstdio>
#include "macro/os_def.h"
#include "macro/_ast.h"
#include "macro/share_utils.h"
#include "string/elsac_ansi_string.hpp"

const char *ElsaNString::ElsaCAnsiString::EmptyStr = "\0";

size_t ElsaNString::ElsaCAnsiString::Format(const char *fmt, ...)
{
    va_list args;
    size_t result = 0;

    va_start(args, fmt);
    result = FormatArgs(fmt, args);
    va_end(args);

    return result;
}

size_t ElsaNString::ElsaCAnsiString::FormatArgs(const char *fmt, va_list args)
{
    CharType buffer[4096 / sizeof(CharType)];
    size_t result = 0;

    result = VSNPRINTFA(buffer, ArrayCount(buffer), fmt, args);
    if (result >= 0)
        buffer[result] = 0;
    SetData(buffer);
    return result;
}