/***
 * @Author: insbread
 * @Date: 2022-07-29 17:50:56
 * @LastEditTime: 2022-07-29 17:50:57
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/stream/elsac_base_stream.cpp
 * @版权声明
 */

#include "macro/os_def.h"
#include "elsac_stream.h"
using namespace ElsaNStream;
int ElsaCBaseStream::GetSize()
{
    int nPos = Seek(0, SEEK_CUR);  // 获取当前位置
    int nSize = Seek(0, SEEK_END); // 获取整个文件长度
    Seek(nPos, SEEK_SET);          // 重置位置
    return nSize;
}

int ElsaCBaseStream::SetPosition(const int tPosition)
{
    return Seek(tPosition, SEEK_SET);
}

int ElsaCBaseStream::CopyFrom(ElsaCBaseStream &stream, int tSizeToCopy)
{
    static const int OnceReadBytes = 8192;

    int nOldPosition = stream.Seek(0, SEEK_CUR); //  获取当前位置，之后需要恢复

    if (tSizeToCopy == 0)
    {
        stream.Seek(0, SEEK_SET);       //  设置到开头
        tSizeToCopy = stream.GetSize(); //  获取整个文件的长度
    }

    if (tSizeToCopy <= 0) //  非法长度
        return tSizeToCopy;

    int nSizeCopyed = 0;
    void *pBuffer = m_pAlloc != nullptr
                        ? m_pAlloc->AllocBuffer(OnceReadBytes)
                        : malloc(OnceReadBytes);

    while (nSizeCopyed < tSizeToCopy)
    {
        // 设置一次性读取的长度
        int nSizeToRead = tSizeToCopy - nSizeCopyed;
        if (nSizeToRead > OnceReadBytes)
            nSizeToRead = OnceReadBytes;

        // 从stream读取数据，并将数据拷贝到自身的缓冲区中
        int nSizeReaded = stream.Read(pBuffer, nSizeToRead);
        if (nSizeReaded <= 0)
            break;
        Write(pBuffer, static_cast<unsigned int>(nSizeReaded));
        nSizeCopyed += nSizeReaded;
    }
    // 删除缓冲区
    m_pAlloc != nullptr ? m_pAlloc->FreeBuffer(pBuffer) : free(pBuffer);
    // 恢复原来的位置
    stream.Seek(nOldPosition, SEEK_SET);
    return nSizeCopyed;
}
