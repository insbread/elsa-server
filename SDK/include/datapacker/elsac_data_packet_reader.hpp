/***
 * @Author: insbread
 * @Date: 2022-07-22 21:07:10
 * @LastEditTime: 2022-07-22 21:07:11
 * @LastEditors: insbread
 * @Description: 数据读取器
 * @FilePath: /elsa-server/SDK/include/datapacker/elsac_data_packet_reader.hpp
 * @版权声明
 */

#pragma once
#include <cstring>
#include <cassert>
#include <cstddef>
#include <cstdint>
/*
    数据读取器DataPacketReader。
    这个类主要实现了多种读取缓冲区数据的功能。
    功能1：以二进制流读取缓冲区，此时数据是以字节为单位进行读取，没有边界的区别(对应于UDP的类型)；
    功能2：以某个类型为单位进行读取(除了类之外)，一次读取一个单位
    功能3：以字符串形式读取缓冲区，此时数据是以一个完整的字符串为单位进行读取，存在边界；

        每一个字符串格式是[2字节字符长度数据][字符串字节数据，字符串长度在65536以内][字符串终止字符0]；
        或者
        每一个字符串格式是[4字节字符长度数据][字符串字节数据，字符串长度在4294967295以内][字符串终止字符0]；

        每一次读取字符串之后，当前缓冲区偏移量offset总是跳过当前字符串指向下一个可读的字符串起始位置。
        即使当前读取的字符串不完整也会跳过当前的字符串。

    类内部缓冲区结构：
    m_cpMem            m_cpOffset       m_cpDataEnd        m_cpEnd
        | ++++++++++++++ | ----------------- | 0000000000000 |
*/
class ElsaCDataPacketReader
{
protected:
    char *m_cpMem;     //  内存始址
    char *m_cpEnd;     //  内存结束位置的下一个地址
    char *m_cpOffset;  //  当前可读写的偏移量位置
    char *m_cpDataEnd; //  当前存放的数据结束位置
public:
    ElsaCDataPacketReader()
    {
        m_cpMem = m_cpEnd = m_cpOffset = m_cpDataEnd = nullptr;
    }
    ElsaCDataPacketReader(void *buf, size_t size)
    {
        m_cpOffset = m_cpMem = static_cast<char *>(buf);
        m_cpEnd = m_cpDataEnd = m_cpMem + size;
    }

public:
    /*** 从缓冲区中读取size个字节数据到buf中，不一定能够读取size个字节数据，因为可能缓冲区数据量不足，所以会返回一个值告知真正读取的数据量是多少。
     *   注意：缓冲区的偏移指针会跳转返回值的距离（注意不一定会跳过size个字节，因为有可能缓冲区数据不足）
     * @param {void} *buf 需要保存的数据缓冲区
     * @param {size_t} size 缓冲区的长度
     * @return {size_t} 实际读取的数据长度
     * @use:
     */
    inline size_t ReadBuf(void *buf, size_t size)
    {
        if (buf == nullptr)
            return 0;

        // 查看剩余的数据长度
        size_t avaliable = m_cpDataEnd - m_cpOffset;
        if (size >= avaliable)
            size = avaliable;

        if (size > 0)
        {
            // 将缓冲区中的数据读取到buf中
            memcpy(buf, m_cpOffset, size);
            m_cpOffset += size;
        }
        return size;
    }
    /*** 读取原子数据，T表示原子数据类型，原子数据指的是int、long、char、void*之类的数据，
     *   如果缓冲区中的剩余数据不足，那么会将数据填充到返回值的低位，并将返回值的高位清零
     * @return {T} 读取到的原子数据
     */
    template <typename T>
    inline T ReadAtom()
    {
        T val;
        size_t avaliable = m_cpDataEnd - m_cpOffset;
        if (avaliable >= sizeof(T))
        {
            val = *reinterpret_cast<T *>(m_cpOffset);
            m_cpOffset += sizeof(T);
        }
        else if (avaliable > 0)
        {
            memset(&val, 0, sizeof(val));
            memcpy(&val, m_cpOffset, avaliable);
            m_cpOffset += avaliable;
        }
        else
        {
            memset(&val, 0, sizeof(val));
        }
        return val;
    }
    /*** 读取unsigned short长度的ASCII字符串，按照如下格式读取：
     * 字符串的数据格式为：[2字节字符长度数据][字符串字节数据，字符串长度在65536以内][字符串终止字符0]；
     * 无论str能不能保存这个么多的数据量，内部的m_cpOffset都会跳过这个段字符串数据区，然后跳到下一个字符串中。
     * @param {char} *str
     * @param {size_t} real_len
     * @return {*}
     * @use:
     */
    inline size_t RawReadStringLen(char *str, size_t read_len)
    {
        return RawReadStringLen<unsigned short>(str, read_len);
    }
    /*** 读取unsigned int超长字符串的数据
     * 字符串的数据格式为：[2字节字符长度数据][字符串字节数据，字符串长度在65536以内][字符串终止字符0]；
     * 无论str能不能保存这个么多的数据量，内部的m_cpOffset都会跳过这个段字符串数据区，然后跳到下一个字符串中。
     * @return {const char*} 返回的数据缓冲区
     */
    inline const char *ReadStringBuf()
    {
        return RawReadStringPtr<char, unsigned int>();
    }

public:
    // 获取整个物理内存的大小
    inline size_t GetSize()
    {
        return m_cpEnd - m_cpMem;
    }
    //  当前保存的数据长度
    inline size_t GetLength()
    {
        return m_cpDataEnd - m_cpMem;
    }
    //  获取当前数据可读的位置
    inline size_t GetPosition()
    {
        return m_cpOffset - m_cpMem;
    }
    //  查看当前缓冲区还有多少数据没有读取完毕
    inline size_t GetAvaliableLength()
    {
        return m_cpDataEnd - m_cpOffset;
    }
    // 获取缓冲区起始地址
    inline char *GetMemoryPtr()
    {
        return m_cpMem;
    }
    // 获取当前数据缓冲区地址
    inline char *GetOffsetPtr()
    {
        return m_cpOffset;
    }
    // 获取指定偏移量的指针
    inline char *GetPositionPtr(size_t pos)
    {
        return m_cpMem + pos;
    }

    // 调整当前偏移指针，同时返回调整后的偏移值，内部会对偏移指针做限制，避免指向不合理的区域
    inline size_t AdjustOffset(int64_t adjust_offset)
    {
        m_cpOffset += adjust_offset;
        if (m_cpOffset < m_cpMem)
            m_cpOffset = m_cpMem;
        if (m_cpOffset > m_cpEnd)
            m_cpOffset = m_cpEnd;
        return m_cpOffset - m_cpMem;
    }

public:
    /*读取类型T的数据，既可以读取原子类型，也可以读取其他类型(除了类之外)*/
    template <typename T>
    inline ElsaCDataPacketReader &operator>>(T &val)
    {
        if (sizeof(val) <= sizeof(int))
            val = ReadAtom<T>();
        else
            ReadBuf(&val, sizeof(val));
        return *this;
    }
    // 将数据读取到str中，读取unsigned int长度的字符串类型
    inline ElsaCDataPacketReader &operator>>(const char *&str)
    {
        str = RawReadStringPtr<char, unsigned int>();
        return *this;
    }

protected:
    /*** 读取长度类型是TL的ANSI字符串数据，读取指定长度的字符串到str中。注意即使读取不完整，缓冲区也会跳过当前的字符串到下一个字符串的开始位置
     * 字符串的数据格式为：[2字节字符长度数据][字符串字节数据，字符串长度在65535以内][字符串终止字符0]；
     * 即使读取参数读取当前字符串不完整，但offset还是会跳过当前字符串，跳到下一条字符串中；
     * 参数TL是存储长度数据变量的类型，最多不超过两字节的长度，一般来说是unsigned char或者unsigend short两种类型
     * @param {char*} 读取数据并存放到这个数据缓冲区中
     * @param {size_t} 缓冲区长度
     * @return {size_t} 最终应该读取的数据长度(不一定是实际读取的长度，但是char*必然是C风格的字符串)
     * @use:
     */
    template <typename TL>
    size_t RawReadStringLen(char *str, size_t read_len)
    {
        assert(read_len > 0);

        size_t avaliable = m_cpDataEnd - m_cpOffset; //  当前还可以读取的数据量长度
        size_t rlen = 0;
        size_t str_len = 0;

        if (avaliable >= 2)
        {
            //  先将两字节长度的数据读取出来
            TL *temp = reinterpret_cast<TL *>(m_cpOffset);
            rlen = str_len = static_cast<size_t>(*temp);

            // 读取数据到str中
            if (read_len > 0)
            {
                m_cpOffset += 2; //  跳过长度数据
                avaliable -= 2;

                // 保证读数据的范围合理，然后再读取
                if (rlen > read_len)
                    rlen = read_len;
                if (rlen > avaliable)
                    rlen = avaliable;
                if (rlen > 0)
                    ReadBuf(str, rlen);

                // 正常来说是要读取str_len数据量的，但是由于上面数据范围的限制可能没办法一次性读取完，
                // 但是没读完的数据必须要丢弃，避免下次读取出现错误。
                // 注意，ReadBuf会中已经跳过了读取的长度了，所以现在需要将剩余的数据长度也跳过。
                if (str_len > rlen)
                    m_cpOffset += (str_len - rlen);
                ++m_cpOffset; //  跳过0
                // 越界则限制指针范围
                if (m_cpOffset > m_cpDataEnd)
                    m_cpOffset = m_cpDataEnd;
            }
        }
        else
        {
            rlen = 0;
        }

        // 变成C风格字符串
        if (rlen < read_len)
        {
            str[rlen] = 0;
        }
        else if (read_len > 0)
        {
            str[read_len - 1] = 0;
        }

        return str_len;
    }
    /* 读取长度类型是TL数据类型是TS的数据，这个函数是一次性从当前的offset读取一个完整的字符串；
     * 字符串的数据格式为：[TL类型的字符长度数据][TS类型字符串字节数据][TS类型字符串终止字符0]
     * <TS>		定义字符数据类型，是char或wchar_t
     * <TL>		定义字符串长度描述数据的数据类型，对于短字符串应当为unsigned char(1字节)，否则即是unsigned short(2字节)
     * @return {const TS*} 一段完整的字符串数据，含结束符号
     */
    template <typename TS, typename TL>
    const TS *RawReadStringPtr()
    {
        size_t avaliable = m_cpDataEnd - m_cpOffset;
        size_t str_len = 0;

        if (avaliable >= sizeof(TL) + sizeof(TS))
        {
            // 获取字符串长度
            str_len = *reinterpret_cast<TL *>(m_cpOffset);

            // 二进制数据格式[TL表示的数据长度长度 | TS类型的数据两个 | TS类型的0结束符]
            if (avaliable >= str_len + sizeof(TL) + sizeof(TS))
            {
                const TS *str = reinterpret_cast<TS *>(m_cpOffset + sizeof(TL));
                m_cpOffset += str_len * sizeof(TS) + sizeof(TL) + sizeof(TS);
                if (m_cpOffset >= m_cpEnd)
                    m_cpOffset = m_cpEnd;
                return str;
            }
        }

        return nullptr;
    }
};

// #define TEST_DEBUG
#ifdef TEST_DEBUG
#include <iostream>
#include <vector>
using std::cout;
using std::endl;
using std::vector;
inline void ElsaCDataPacketReaderTestFunc1()
{

    vector<unsigned int> buf_len = {10, 30, 3, 1, 100, 1000};
    vector<char> copy_char = {'a', 'b', 'c', 'd', 'e', 'f'};
    unsigned int cur_sum = 0;
    for (auto x : buf_len)
        cur_sum = cur_sum + sizeof(unsigned int) + x + 1;
    char *cur_buf = new char[cur_sum];
    char *free_buf = new char[cur_sum];
    memset(free_buf, 0, sizeof(free_buf));
    char *ptr = cur_buf;
    for (int i = 0; i < buf_len.size(); i++)
    {
        *reinterpret_cast<unsigned int *>(ptr) = buf_len[i];
        ptr += sizeof(unsigned int);
        memset(ptr, copy_char[i], buf_len[i]);
        ptr += buf_len[i];
        memset(ptr, '\0', 1);
        ptr += 1;
    }

    ElsaCDataPacketReader reader(cur_buf, cur_sum);
    cout << "Mem Size: " << reader.GetSize() << "; Data Size: " << reader.GetLength() << "; Avaliable Size: " << reader.GetAvaliableLength() << "; Readable Pos: " << reader.GetPosition() << endl;
    reader.ReadBuf(free_buf, cur_sum);
    bool flag = true;
    for (int i = 0; i < (int)cur_sum; i++)
        if (free_buf[i] != cur_buf[i])
            flag = false;
    cout << "ReadBuf is Normal: " << (flag ? "Yes" : "No") << endl;
    reader.AdjustOffset(-reader.GetSize()); // 重置偏移量
    const char *tmp_buf = nullptr;

    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf1 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf2 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf3 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf4 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf5 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    tmp_buf = reader.ReadStringBuf();
    cout << "ReadStringBuf6 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    cout << endl
         << endl;
    reader.AdjustOffset(-reader.GetSize());

    reader >> tmp_buf;
    cout << "ReadStringBuf1 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    reader >> tmp_buf;
    cout << "ReadStringBuf2 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    reader >> tmp_buf;
    cout << "ReadStringBuf3 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    reader >> tmp_buf;
    cout << "ReadStringBuf4 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    reader >> tmp_buf;
    cout << "ReadStringBuf5 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    reader >> tmp_buf;
    cout << "ReadStringBuf6 len: " << strlen(tmp_buf) << " res : " << tmp_buf << endl;
    cout << endl;
    reader.AdjustOffset(-reader.GetSize());
    delete[] cur_buf;
    delete[] free_buf;
}

inline void ElsaCDataPacketReaderTestFunc2()
{

    vector<unsigned int> buf_len = {10, 30, 3, 1, 100, 1000};
    vector<char> copy_char = {'a', 'b', 'c', 'd', 'e', 'f'};
    unsigned int cur_sum = 0;
    for (auto x : buf_len)
        cur_sum = cur_sum + sizeof(unsigned short) + x + 1;
    char *cur_buf = new char[cur_sum];
    char *free_buf = new char[cur_sum];
    memset(free_buf, 0, sizeof(free_buf));
    char *ptr = cur_buf;
    for (int i = 0; i < buf_len.size(); i++)
    {
        *reinterpret_cast<unsigned short *>(ptr) = buf_len[i];
        ptr += sizeof(unsigned short);
        memset(ptr, copy_char[i], buf_len[i]);
        ptr += buf_len[i];
        memset(ptr, '\0', 1);
        ptr += 1;
    }

    ElsaCDataPacketReader reader(cur_buf, cur_sum);

    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf1 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf2 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf3 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf4 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf5 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    reader.RawReadStringLen(free_buf, cur_sum);
    cout << "ReadStringBuf6 len: " << strlen(free_buf) << " res : " << free_buf << endl;
    cout << endl
         << endl;
    reader.AdjustOffset(-reader.GetSize());

    delete[] cur_buf;
    delete[] free_buf;
}
#endif
