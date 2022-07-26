/***
 * @Author: insbread
 * @Date: 2022-07-23 17:29:09
 * @LastEditTime: 2022-07-23 17:29:10
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/datapacker/elsac_data_packet.hpp
 * @版权声明
 */

#pragma once

#include "macro/_ast.h"
#include "macro/os_def.h"
#include "memory/elsac_base_allocator.hpp"
#include "datapacker/elsac_data_packet_reader.hpp"

class ElsaCDataPacket : public ElsaCDataPacketReader
{
public:
    typedef ElsaCDataPacketReader Inherit;

protected:
    ElsaCBaseAllocator *m_pAllocator;
    int m_iAllocCount; //  每次内存扩展时，都需要额外添加的尾部防卫区

public:
    ElsaCDataPacket() : Inherit()
    {
        m_pAllocator = nullptr;
        m_iAllocCount = 0;
    }
    ElsaCDataPacket(const ElsaCDataPacket &) : Inherit()
    {
        assert(false); //  禁止拷贝函数
    }
    // 这个初始化函数仅仅表示需要使用外部的缓冲区存储数据，类不认为buf内有数据(和父类不一样)
    ElsaCDataPacket(void *buf, size_t size) : Inherit(buf, size)
    {
        m_cpDataEnd = m_cpMem; //  此时和父类不同，默认为设置的缓冲区是没有数据的
        m_pAllocator = nullptr;
        m_iAllocCount = 0;
    }
    ElsaCDataPacket(ElsaCBaseAllocator *allocator) : Inherit()
    {
        m_pAllocator = allocator;
        m_iAllocCount = 0;
    }
    ~ElsaCDataPacket()
    {
        Empty();
    }

public:
    // 物理清空缓冲区，注意只有指定了内存分配器才会释放内存，否则是由外部来释放内存
    void Empty()
    {
        if (m_cpMem && m_pAllocator)
        {
            m_pAllocator->FreeBuffer(m_cpMem);
            m_cpMem = m_cpDataEnd = m_cpOffset = m_cpEnd = nullptr;
        }
    }

public:
    // 适用于在通过开启预定义宏_DPPACK_MEM_CHK_的情况下，对数据包进行内存越界检查
    inline void _Assert()
    {
#ifdef _DPPACK_MEM_CHK_
        DbgAssert(!m_cpEnd || *(int *)m_cpEnd == 0xCCDDCCEE);
#endif
    }

protected:
    /*** 重新设置数据缓冲区的长度，并拷贝原来的数据到新的缓冲区中，各个内存区的指针尽量保持和原来是一样的。
     * 但是这个函数不能由外部使用，因为这个函数设置的新长度没有做限制，而offset、dataend是保持原来的偏移量的，
     * 如果缩短长度，此时就可能导致offset、dataend指针失效。
     * @param {size_t} new_size 需要重新设置的缓冲区长度
     */
    void SetSize(size_t new_size)
    {
        char *old_mem = m_cpMem;
        new_size += m_iAllocCount; //  注意设置防卫区
#ifdef _DPPACK_MEM_CHK_
        char *new_mem = static_cast<char *>(m_pAllocator->AllocBuffer(new_size + sizeof(int)));
#else
        char *new_mem = static_cast<char *>(m_pAllocator->AllocBuffer(new_size));
#endif
        size_t offset = m_cpOffset - m_cpMem;
        size_t length = m_cpDataEnd - m_cpMem;

        if (length > 0)
        {
            memcpy(new_mem, old_mem, __min(new_size, length));
        }

        m_cpMem = new_mem;
        m_cpEnd = new_mem + new_size;
        m_cpDataEnd = new_mem + length;
        m_cpOffset = new_mem + offset;
#ifdef _DPPACK_MEM_CHK_
        *reinterpret_cast<int *>(m_cpEnd) = 0xCCDDCCEE; //  做一个标识检测数据的结束位置
#endif
        if (old_mem)
        {
            _Assert();
            m_pAllocator->FreeBuffer(old_mem);
        }
    }

public:
    // 设置额外分配的内存
    inline void SetAllocSize(int size)
    {
        m_iAllocCount = size;
    }
    inline void SetAllocator(ElsaCBaseAllocator *allocator)
    {
        m_pAllocator = allocator;
    }

    // 在ElsaCDataPacket中，凡是调整长度、偏移位置都会进行长度的扩充，然后再进行偏移量的纠正。

    // 设置长度，并且只能设置更长的长度，能够保证内部各种变量指针的有效性
    inline void SetLength(size_t new_length)
    {
        if (new_length > static_cast<size_t>(m_cpEnd - m_cpMem))
        {
            SetSize(new_length);
        }

        // 由于SetSize并不会对长度变化后各个指针的有效性进行检测，所以需要这里进行检测
        m_cpDataEnd = m_cpMem + new_length;
        if (m_cpOffset > m_cpDataEnd)
        {
            m_cpOffset = m_cpDataEnd;
        }
    }
    // 设置偏移量，注意如果设置的偏移量比现有物理内存还要大，那么会自动进行扩充，并且保证各个变量的有效性
    inline size_t SetPosition(size_t newPos)
    {
        size_t mem_size = m_cpEnd - m_cpMem;

        if (newPos > mem_size)
        {
            SetSize(newPos);
        }

        m_cpOffset = m_cpMem + newPos;
        // 这里不用担心m_cpDataEnd出问题，因为扩充后的长度newPos是大于原来的长度的，所以m_cpDataEnd并不会出错
        if (m_cpOffset > m_cpDataEnd)
        {
            m_cpDataEnd = m_cpOffset;
        }
        return newPos;
    }

    // 获取当前可写的内存剩余空间
    inline size_t GetAvaliabeleBufLen()
    {
        return m_cpEnd - m_cpOffset;
    }

    // 调整并设置偏移量，这里设置的偏移量可正可负，
    // 如果负值过大会限制偏移量指向起始位置，如果正值过大那么会先将内存进行扩充，然后限制偏移量指向数据最后一区
    inline size_t AdjustOffset(int64_t adjust_offset)
    {
        m_cpOffset += adjust_offset;

        if (m_cpOffset < m_cpMem) //  如果往前越界则只能在开头
            m_cpOffset = m_cpMem;
        else if (m_cpOffset > m_cpEnd) //  如果往后越界，则扩充
            SetSize(m_cpOffset - m_cpMem);

        if (m_cpOffset > m_cpDataEnd) //  限制offset不能超过m_cpDataEnd
            m_cpDataEnd = m_cpOffset;
        return m_cpOffset - m_cpMem;
    }
    // 扩充内存长度，返回原来的长度
    inline size_t Reverse(size_t newSize)
    {
        size_t result = m_cpEnd - m_cpMem;

        if (result < newSize)
            SetSize(newSize);
        return result;
    }

protected:
    /***  写入短字符串以及字符串数据，
        写入的字符串格式为：[sizeof(TL)字符长度数据][字符串字节数据，字符串长度在TL类型的长度范围以内][字符串终止字符0]；
        在字符串之前写入sizeof(TL)字节的长度值，且会在字符串末尾写入终止字符。
        一般TL要么是unsigned char用于描述短字符串，要么是unsigned short用于描述长字符串，当然其实还可以是int
        @param {const char*} str 表示一个C风格字节串，
        @param {size_t} len表示字符串长度，-1表示采用自适应确定字符串长度，注意此时str必须是C风格
     */
    template <typename TL>
    void RawWriteStringLen(const char *str, size_t len)
    {
        if (str == nullptr)
            str = "";
        if (len == static_cast<size_t>(-1))
            len = str ? static_cast<TL>(strlen(str)) : 0;
        else
            len = __min(len, strlen(str));

        WriteAtom<TL>(static_cast<TL>(len));
        WriteBuf(str, len * sizeof(*str));
        WriteAtom<char>(0);
    }

public:
    /*** 写入二进制数据，会自动扩充内存区长度
     * @param {void*} buf 要写入的内存数据
     * @param {size_t} size 要写入的数据的长度
     */
    inline void WriteBuf(const void *buf, size_t size)
    {
        // 先计算当前可写的量，注意m_cpOffset在这里表示可写的位置
        size_t mem_size = m_cpEnd - m_cpOffset;

        if (mem_size < size)
        {
            // 剩余量不够写，就扩充
            SetSize(m_cpEnd - m_cpMem + size);
        }

        memcpy(m_cpOffset, buf, size);
        m_cpOffset += size;

        // 这里写入的量比较大，就需要调整数据尾的指针
        if (m_cpOffset > m_cpDataEnd)
            m_cpDataEnd = m_cpOffset;
    }
    // 写入原子数据，原子数据指的是int，long，char等等
    template <typename T>
    inline void WriteAtom(const T &data)
    {
        size_t mem_size = m_cpEnd - m_cpOffset;
        if (mem_size < sizeof(T))
        {
            SetSize(m_cpEnd - m_cpMem + sizeof(T));
        }

        *reinterpret_cast<T *>(m_cpOffset) = data;
        m_cpOffset += sizeof(T);

        if (m_cpOffset > m_cpDataEnd)
            m_cpDataEnd = m_cpOffset;
    }
    // 写入短字符串类型，
    inline void WriteString(const char *str, size_t len = -1)
    {
        RawWriteStringLen<unsigned short>(str, len);
    }
    // 写入长字符串类型
    inline void WriteStringBuf(const char *str, size_t len = -1)
    {
        RawWriteStringLen<unsigned int>(str, len);
    }
    // 将str按照 [unsigend short字符长度数据][字符串字节数据，字符串长度在0~65535类型的长度范围以内][字符串终止字符0] 格式写入到buf，buf不够长就会截断str
    static int WriteString_s(char *buf, int maxLen, const char *str)
    {
        // 注意，至少需要一个2字节的长度数据以及一个结束字符，写不进去直接返回
        if (buf == nullptr || maxLen < 3)
            return 0;

        if (str == nullptr)
            str = "";

        // 获取字符串长度
        int strLen = static_cast<int>(strlen(str));

        // 注意这里会限制长度，避免无法写入buf中
        int cpyLen = __min(maxLen - 3, strLen);
        unsigned short *tmp = reinterpret_cast<unsigned short *>(buf);

        *tmp = static_cast<unsigned short>(cpyLen);
        memcpy(buf + 2, str, cpyLen);
        buf[cpyLen + 2] = 0;
        return cpyLen + 3;
    }

public:
    // 将变量val原子数据写入到缓冲区中
    template <typename T>
    inline ElsaCDataPacket &operator<<(const T &val)
    {
        if (sizeof(T) <= sizeof(int))
            WriteAtom<T>(val);
        else
            WriteBuf(&val, sizeof(val)); //  太长就当二进制数据写入，例如结构体、枚举、longlong之类的
        return *this;
    }
    // 写字符串
    inline ElsaCDataPacket &operator<<(const char *val)
    {
        static const char *def = "";
        val == nullptr ? WriteString(def, strlen(def)) : WriteString(val, strlen(val));
        return *this;
    }
    // 写字符串
    inline ElsaCDataPacket &operator<<(char *val)
    {
        static const char *def = "";
        val == nullptr ? WriteString(def, strlen(def)) : WriteString(val, strlen(val));
        return *this;
    }
    // 禁止使用赋值函数，会被asset报错
    inline ElsaCDataPacket &operator=(const ElsaCDataPacket &)
    {
        assert(false);
    }
};

// #define TEST_DEBUG
#ifdef TEST_DEBUG
#include <iostream>
using namespace std;
inline void ElsaCDataPacketTestFunc()
{
    ElsaCBaseAllocator allo("ElsaCDataPacket");
    ElsaCDataPacket tm(&allo);
    char *str1 = "ABCD";
    char *str2 = "EFGHIJK";
    char *str3 = "LMNOPQRSTUVWXYZ";

    tm.WriteAtom<int>(101);
    tm.WriteString(str1);
    tm.WriteString(str2);
    tm.WriteString(str3);

    char str_tmp[1000];
    tm.AdjustOffset(-tm.GetSize());
    cout << "WriteAtom res: " << tm.ReadAtom<int>() << endl;
    tm.RawReadStringLen(str_tmp, 1000);
    cout << "WriteStr res1: " << str_tmp << endl;

    tm.RawReadStringLen(str_tmp, 1000);
    cout << "WriteStr res2: " << str_tmp << endl;

    tm.RawReadStringLen(str_tmp, 1000);
    cout << "WriteStr res3: " << str_tmp << endl;

    cout << endl
         << endl;

    tm.Empty();
    tm.WriteStringBuf(str1);
    tm.WriteStringBuf(str2);
    tm.WriteStringBuf(str3);

    tm.AdjustOffset(-tm.GetSize());
    cout << "WriteStringBuf res1: " << tm.ReadStringBuf() << endl;
    cout << "WriteStringBuf res2: " << tm.ReadStringBuf() << endl;
    cout << "WriteStringBuf res3: " << tm.ReadStringBuf() << endl;

    return;
}
#endif