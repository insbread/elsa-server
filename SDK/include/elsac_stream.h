/***
 * @Author: insbread
 * @Date: 2022-07-27 21:59:37
 * @LastEditTime: 2022-07-27 22:07:12
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/elsac_stream.h
 * @版权声明
 */
#pragma once
#include <zlib.h>
#include "memory/elsac_base_allocator.hpp"

namespace ElsaNStream
{
    class ElsaCBaseStream
    {
    protected:
        ElsaCBaseAllocator *m_pAlloc;

    public:
        ElsaCBaseStream(ElsaCBaseAllocator *alloc) : m_pAlloc(alloc) {}
        virtual ~ElsaCBaseStream() {}

        // 获取当前流的大小
        virtual int GetSize();
        virtual bool SetSize(int) { return false; }
        //  获取当前流指针距离流开始位置的字节数
        inline int GetPosition() { return Seek(0, SEEK_CUR); }
        // 设置从流开始偏移tPosition位置的偏移量
        int SetPosition(const int tPosition);
        /*** 设置当前流的指针，偏移计算式origin + tOffset
         * @param {int} tOffset 偏移量
         * @param {int} origin 可以是SEEK_CUR当前位置，SEEK_SET开头位置，SEEK_END结尾位置
         * @return {int} 当前流指针距离开头的字节数
         */
        virtual int Seek(const int tOffset, const int origin) = 0;
        virtual int Read(void *lpBuffer, const int tSizeToRead) = 0;
        virtual int Write(const void *lpBuffer, const size_t tSizeToWrite) = 0;
        // 从另外一个流中拷贝tSizeCopy字节过来，如果tSizeCopy是0，那么就将stream所有数据都拷贝过来
        virtual int CopyFrom(ElsaCBaseStream &stream, int tSizeToCopy = 0);
    };

    class ElsaCFileStream : public ElsaCBaseStream
    {
    protected:
        FILE *m_pFp; //  文件指针
    public:
        // 文件打开方式
        enum ElsaEFileAccessType
        {
            faRead = 0x0001,   // 读取
            faWrite = 0x0002,  // 写入
            faCreate = 0x1000, // 创建

            faShareRead = 0x0100,   // 读共享
            faShareWrite = 0x0200,  // 写共享
            faShareDelete = 0x0400, // 删除共享
        };
        // win32文件创建模式
        enum ElsaEOpenDisposition
        {
            CreateIfNotExists = 1, //  文件不存在则创建
            AlwaysCreate,          //  总是创建文件，存在则截断原文件为0字节
            OpenExistsOnly,        //  仅打开存在的文件
            AlwaysOpen,            //  总是打开文件，存在则打开，不存在则创建
            TruncExistsOnly,       //  文件存在则打开文件并截断
        };

    private:
        char m_sFn[256]; //  文件名称

    protected:
        void SetFileName(const char *lpFileName);
        void Construct(const char *lpFileName, unsigned int dwAccessType, unsigned int dwWin32CreationDisposition);

    public:
        /*** 初始化文件流系统
         * @param {const char*} lpFileName 文件名称
         * @param {unsigned int} dwAccessType 文件访问属性
         * @param {ElsaCBaseAllocator*} alloc 分配器
         */
        ElsaCFileStream(const char *lpFileName, unsigned int dwAccessType, ElsaCBaseAllocator *alloc);
        /*** 初始化文件流系统
         * @param {const char*} lpFileName文件名称
         * @param {unsigned int} dwAccessType 文件访问属性
         * @param {ElsaEOpenDisposition} eWin32CreateionDisposition 文件创建模式
         * @param {ElsaCBaseAllocator*} alloc 分配器
         */
        ElsaCFileStream(const char *lpFileName, unsigned int dwAccessType, ElsaEOpenDisposition eWin32CreateionDisposition, ElsaCBaseAllocator *alloc);
        virtual ~ElsaCFileStream();

        /*** 调整当前的流指针位置
         * @param {int} tOffset 偏移量
         * @param {int} origin 可以是SEEK_CUR当前位置，SEEK_SET开头位置，SEEK_END结尾位置
         * @return {int} 当前流指针距离流开始位置的距离
         */
        int Seek(const int tOffset, const int origin);
        /*** 从文件流中读取数据
         * @param {void*} lpBuffer 数据读取存放的缓冲区
         * @param {size_t} tSizeToRead 需要读取的字节数
         * @return {int} 真正读取的字节数量
         */
        int Read(void *lpBuffer, const int tSizeToRead);
        /***
         * @brief:
         * @description:
         * @param {void} *lpBuffer 需要写入的缓冲区数据
         * @param {size_t} tSizeToWrite 需要写入的数据
         * @return {int} 真正写入到文件流中的数据量
         */
        int Write(const void *lpBuffer, const size_t tSizeToWrite);
        inline const char *GetFileName()
        {
            return m_sFn;
        }
    };
};

#define TEST_DEBUG
#ifdef TEST_DEBUG
#include <iostream>
using namespace std;
using namespace ElsaNStream;

inline void ElsaCFileStreamTestReadFunc()
{
    const char *filename = "/home/zhenjie/project/snowlegend/tmp/test_file.txt";
    ElsaCFileStream fm(filename, ElsaCFileStream::ElsaEFileAccessType::faRead, nullptr);

    char *buffer = new char[1024];
    memset(buffer, 0, 1024 * sizeof(char));

    int read_len = fm.Read(buffer, 1024);
    cout << buffer << endl;
}

inline void ElsaCFileStreamTestWriteFunc()
{
    const char *filename = "/home/zhenjie/project/snowlegend/tmp/test_file.txt";
    ElsaCFileStream fm(filename, ElsaCFileStream::ElsaEFileAccessType::faWrite, nullptr);

    char *buffer = new char[1024];
    strcpy(buffer, "Hi");

    int read_len = fm.Write(buffer, 2);
}

#endif