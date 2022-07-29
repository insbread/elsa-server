/***
 * @Author: insbread
 * @Date: 2022-07-29 17:02:21
 * @LastEditTime: 2022-07-29 17:03:26
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/elsac_stream.cpp
 * @版权声明
 */

#include <cstdio>
#include <cstdlib>

#include "macro/os_def.h"
#include "macro/_ast.h"
#include "macro/share_utils.h"

#include "elsac_stream.h"

using namespace ElsaNStream;

ElsaCFileStream::ElsaCFileStream(const char *lpFileName, unsigned int dwAccessType, ElsaCBaseAllocator *alloc) : ElsaCBaseStream(alloc)
{
    unsigned int dwWin32CreationDisposition = 0;
    m_pFp = nullptr;

    Construct(lpFileName, dwAccessType, dwWin32CreationDisposition);
}

ElsaCFileStream::ElsaCFileStream(const char *lpFileName, unsigned int dwAccessType, ElsaEOpenDisposition eWin32CreateionDisposition, ElsaCBaseAllocator *alloc) : ElsaCBaseStream(alloc)
{
    Construct(lpFileName, dwAccessType, eWin32CreateionDisposition);
}

ElsaCFileStream::~ElsaCFileStream()
{
    if (m_pFp != nullptr)
        fclose(m_pFp);
}

void ElsaCFileStream::SetFileName(const char *lpFileName)
{
    _STRNCPY_A(m_sFn, lpFileName);
}

void ElsaCFileStream::Construct(const char *lpFileName, unsigned int dwAccessType, unsigned int dwWin32CreationDisposition)
{
    SetFileName(lpFileName);
    const char *mode = "r";
    if (dwAccessType & faRead)
        mode = "r";
    else
        mode = "w";
    m_pFp = fopen(m_sFn, mode);
}

int ElsaCFileStream::Seek(const int tOffset, const int origin)
{
    if (m_pFp != nullptr)
    {
        if (fseek(m_pFp, tOffset, origin) == 0)
        {
            // 返回当前流指针距离开始位置的偏移量
            return ftell(m_pFp);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

int ElsaCFileStream::Read(void *lpBuffer, const int tSizeToRead)
{
    if (m_pFp != nullptr)
    {
        return fread(lpBuffer, 1, tSizeToRead, m_pFp);
    }

    return 0;
}

int ElsaCFileStream::Write(const void *lpBuffer, const size_t tSizeToWrite)
{
    if (lpBuffer == nullptr)
        return 0;

    if (m_pFp != nullptr && lpBuffer != nullptr)
    {
        return fwrite(lpBuffer, 1, tSizeToWrite, m_pFp);
    }
    return 0;
}