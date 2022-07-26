/***
 * @Author: insbread
 * @Date: 2022-07-25 15:39:01
 * @LastEditTime: 2022-07-25 15:39:01
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/string/elsac_ansi_string.hpp
 * @版权声明
 */
/******************************************************************
 *
 *	$ UCS2字符串类 $
 *
 *  - 主要功能 -
 *
 *	实现UCS字符串的基本操作，包括与UCS2字符指针的相互操作
 *
 *****************************************************************/
#pragma once
#include <cstdarg>
#include <cstdlib>
#include <cstring>

namespace ElsaNString
{
    class ElsaCAnsiString
    {
    public:
        typedef char CharType;
        typedef char *RawStrType;

    private:
        char *m_sStr;                //  需要保存的字符串
        static const char *EmptyStr; //  空字符串的指针

    protected:
        /*** 分配新的内存，如果ptr不为nullptr，则在ptr的基础上将其数据拷贝到新的内存上，子类可以继承这个函数自定义分配功能
         * @param {void*} ptr 原来的数据
         * @param {size_t} newsize 新分配的数据大小
         * @return {void*} 分配后的空间
         */
        virtual void *AllocAnsi(void *ptr, size_t newsize)
        {
            if (newsize == 0)
            {
                if (ptr != nullptr)
                    free(ptr);
                return nullptr;
            }
            return realloc(ptr, newsize);
        }
        /*** 设置新的字符串数据
         * @param {char} *data C风格的字符串，如果是nullptr就相当于将内部字符串清空
         */
        void SetData(const char *data)
        {
            if (data != nullptr)
            {
                // 获取需要分配的内存大小
                size_t size = strlen(data) + 1;
                size = size * sizeof(*data);
                // 分配足够的内存空间，然后拷贝数据到新空间上
                m_sStr = static_cast<char *>(AllocAnsi(m_sStr, size));
                memcpy(m_sStr, data, size);
            }
            else
            {
                // 如果是赋值为nullptr，就清空原来的数据
                m_sStr = static_cast<char *>(AllocAnsi(m_sStr, 0));
            }
        }
        /*** 将字符串data拼接到当前字符串的后面
         * @param {char} *data 需要拼接的字符串
         */
        void CatData(const char *data)
        {
            if (data != nullptr)
            {
                size_t cat_size = strlen(data);
                size_t cur_size = Length();
                // 分配足够的内存空间
                m_sStr = static_cast<char *>(AllocAnsi(m_sStr, (cat_size + cur_size + 1) * sizeof(*data)));
                memcpy(m_sStr + cur_size, data, (cat_size + 1) * sizeof(*data));
            }
        }

    public:
        ElsaCAnsiString()
        {
            m_sStr = nullptr;
        }
        ElsaCAnsiString(const char *data)
        {
            m_sStr = nullptr;
            SetData(data);
        }
        ElsaCAnsiString(const ElsaCAnsiString &str)
        {
            m_sStr = nullptr;
            SetData(str.m_sStr);
        }
        ElsaCAnsiString(size_t len)
        {
            m_sStr = nullptr;
            SetLength(len);
        }
        virtual ~ElsaCAnsiString()
        {
            SetData(nullptr);
        }

    public:
        // 字符串的一些常规操作

        // 取得C风格的字符串指针
        inline const char *RawStr() const
        {
            return m_sStr;
        }
        inline char *RawStr()
        {
            return m_sStr;
        }
        // 获取字符串长度
        inline size_t Length() const
        {
            return m_sStr != nullptr ? strlen(m_sStr) : 0;
        }
        // 设置字符串长度(保留空间)
        inline void SetLength(const size_t len)
        {
            if (len > 0)
            {
                // 分配新的空间
                m_sStr = static_cast<char *>(AllocAnsi(m_sStr, sizeof(m_sStr[0]) * (len + 1)));
                if (m_sStr != nullptr)
                    m_sStr[len] = 0;
            }
            else
            {
                m_sStr = static_cast<char *>(AllocAnsi(m_sStr, 0));
            }
        }
        // 比较两个字符串是否相等
        inline int Compare(const char *data) const
        {
            // 当两个字符串之间有一个是空字符串，需要额外的进行判断
            if (m_sStr == nullptr || m_sStr[0] == '\0')
            {
                if (data == nullptr || data[0] == '\0')
                    return 0;
                else
                    return 1;
            }
            return strcmp(m_sStr, data);
        }
        inline int Compare(const ElsaCAnsiString &str) const
        {
            if (m_sStr == nullptr || m_sStr[0] == 0)
            {
                if (str.m_sStr == nullptr || str.m_sStr[0] == 0)
                    return 0;
                else
                    return 1;
            }
            return strcmp(m_sStr, str.m_sStr);
        }
        // 判断是不是空字符串
        inline bool IsEmpty() const
        {
            return m_sStr == nullptr || m_sStr[0] == 0;
        }
        // 格式化字符串
        size_t Format(const char *fmt, ...);
        size_t FormatArgs(const char *fmt, va_list args);

    public:
        // 重载字符串的操作

        // 类型转化为const char*，nullptr字符串会返回""
        inline operator const char *() const
        {
            return m_sStr == nullptr ? EmptyStr : m_sStr;
        }
        // 注意nullptr字符串则会返回nullptr字符串
        inline operator char *()
        {
            return m_sStr == nullptr ? nullptr : m_sStr;
        }
        // 赋值函数
        inline void operator=(const char *data)
        {
            if (data != m_sStr)
            {
                SetData(data);
            }
        }
        inline void operator=(const ElsaCAnsiString &str)
        {
            if (&str != this)
            {
                SetData(str.m_sStr);
            }
        }
        // 字符串拼接到自身操作函数
        inline void operator+=(const char *data)
        {
            if (data != m_sStr)
            {
                CatData(data);
            }
            else
            {
                ElsaCAnsiString wstr(m_sStr);
                CatData(wstr.m_sStr);
            }
        }
        inline void operator+=(const ElsaCAnsiString &str)
        {
            if (&str != this)
            {
                CatData(str.m_sStr);
            }
            else
            {
                ElsaCAnsiString wstr(str.m_sStr);
                CatData(wstr.m_sStr);
            }
        }
        // 字符串拼接操作函数
        inline const ElsaCAnsiString operator+(const char *data)
        {
            ElsaCAnsiString str(m_sStr);
            str.CatData(data);
            return str;
        }
        inline const ElsaCAnsiString operator+(const ElsaCAnsiString &str)
        {
            ElsaCAnsiString res(m_sStr);
            res.CatData(str.m_sStr);
            return res;
        }
        inline bool operator==(const char *data) const
        {
            return Compare(data) == 0;
        }
        inline bool operator==(const ElsaCAnsiString &str) const
        {
            return Compare(str.m_sStr) == 0;
        }
        inline bool operator!=(const char *data) const
        {
            return Compare(data) != 0;
        }
        inline bool operator!=(const ElsaCAnsiString &str) const
        {
            return Compare(str.m_sStr) != 0;
        }
        inline bool operator!() const
        {
            return m_sStr == nullptr || m_sStr[0] == 0;
        }
    };
};

    // #define TEST_DEBUG

#ifdef TEST_DEBUG
#include <iostream>
using namespace std;
using namespace ElsaNString;
inline void ElsaCAnsiStringTestFunc()
{
    ElsaCAnsiString str1("I am a cool boy!");

    cout << "Raw str: " << str1.RawStr() << endl;
    cout << "Length: " << str1.Length() << endl;
    cout << "Compare \"I am a cool boy!\": " << (str1.Compare("I am a cool boy!") == 0 ? "Same" : "Different") << endl;
    cout << endl;

    str1.SetLength(str1.Length() - 3);
    cout << "After SetLength" << endl;
    cout << "Raw str: " << str1.RawStr() << endl;
    cout << "Length: " << str1.Length() << endl;
    cout << endl;

    str1.Format("Today is %d Month, %d Day, %s", 7, 25, "Yeah!");
    cout << "After Format Today is %d Month, %d Day, %s" << endl;
    cout << "Raw str: " << str1.RawStr() << endl;
    cout << "Length: " << str1.Length() << endl;
    cout << endl;

    str1 = "Yeah!";
    cout << "operator = \"Yeah!\"" << endl;
    cout << "Raw str: " << str1.RawStr() << endl;
    cout << "Length: " << str1.Length() << endl;
    cout << endl;

    cout << "str1 == Yeah! is " << (str1 == "Yeah!" ? "True" : "False") << endl;
    cout << "str1 != Yeah! is " << (str1 != "Yeah!" ? "True" : "False") << endl;

    str1 += "  Oh Yead!";
    cout << "After operator+= \"  Oh Yead!\"" << endl;
    cout << "Raw str: " << str1.RawStr() << endl;
    cout << "Length: " << str1.Length() << endl;
    cout << endl;

    str1 = "";
    cout << "After str1 = \"\"" << endl;
    cout << "Is Str Empty? " << (!str1 ? "True" : "False") << endl;
    cout << endl;
}
#endif