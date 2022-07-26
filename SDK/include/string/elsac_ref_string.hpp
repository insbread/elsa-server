/***
 * @Author: insbread
 * @Date: 2022-07-25 17:18:34
 * @LastEditTime: 2022-07-25 17:18:34
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/include/string/elsac_ref_string.hpp
 * @版权声明
 */

#pragma once
#include "string/elsac_string_ex.h"
#include "elsac_ref_obj.hpp"

namespace ElsaNString
{
    /*
        一个采用引用计数器管理的字符串类，管理由TS实例化对象的引用，这个类其实并不建议由外部直接使用，
        因为这个类要求模板TS和TC有许多特殊的接口，因此建议采用后面实例化类型ElsaCCRefAnsiString或者ElsaCRefString。
        <TS是一个自定义的字符串类，TC则是存储在TS内部的字符串缓冲区的字符类型>

        这里如果要理解，那么TS=ElsaCAnsiString, TC = char理解是最合理的。
    */
    template <typename TS, typename TC>
    class ElsaCCTRefString : public ElsaNMisc::ElsaCRefObject<TS>
    {
        typedef ElsaNMisc::ElsaCRefObject<TS> Inherited;
        typedef ElsaNMisc::ElsaCRefObjectImpl<TS> CRefTS;

    public:
        ElsaCCTRefString() : Inherited() {}
        ElsaCCTRefString(const size_t len)
        {
            this->m_pRef = new CRefTS(len);
        }
        ElsaCCTRefString(const TC *str)
        {
            this->operator=(str);
        }
        ElsaCCTRefString(const TS &str)
        {
            this->m_pRef = new CRefTS(str);
        }
        ElsaCCTRefString(const ElsaCCTRefString<TS, TC> &rStr)
        {
            this->operator=(rStr);
        }

    public:
        inline operator const TC *() const
        {
            return this->m_pRef ? this->m_pRef->RawStr() : nullptr;
        }
        inline operator TC *()
        {
            // 这里相当于调用TS::RawStr()
            return this->m_pRef ? this->m_pRef->RawStr() : nullptr;
        }
        // 赋值函数，赋予(const TC*)类型的值，会释放原来指向的字符串的引用，指向新的字符串
        inline void operator=(const TC *data)
        {
            CRefTS *newPtr = new CRefTS();
            *static_cast<TS *>(newPtr) = data;

            if (this->m_pRef != nullptr)
                this->m_pRef->Release();
            this->m_pRef = newPtr;
        }
        // 赋值函数，赋予(const TS &)类型的值，会释放原来指向的字符串的引用，指向新的字符串
        inline void operator=(const TS &str)
        {
            CRefTS *newPtr = new CRefTS();
            *static_cast<TS *>(newPtr) = str;

            if (this->m_pRef != nullptr)
                this->m_pRef->Release();
            this->m_pRef = newPtr;
        }
        // +=函数会释放原来的指向的字符串，指向新的字符串
        inline void operator+=(const TC *data)
        {
            // 拷贝的思路：先构建一个新的对象，利用子类可以隐式转化为父类，拷贝原来的数据到新对象上，将参数TC拼接到新对象上，释放旧数据，然后指向新的数据
            CRefTS *newPtr = new CRefTS();
            if (this->m_pRef != nullptr)
            {
                // 获取CRefTS的父类TS，并进行赋值
                *static_cast<TS *>(newPtr) = *(this->m_pRef);
            }
            // 这里会调用父类的operator +=，即相当于调用了TS += data
            *newPtr += data;
            if (this->m_pRef != nullptr)
            {
                this->m_pRef->Release(); // 释放原来引用的对象
            }
            this->m_pRef = newPtr;
        }
        inline void operator+=(const TS &str)
        {
            CRefTS *newPtr = new CRefTS();
            if (this->m_pRef != nullptr)
            {
                *static_cast<TS *>(newPtr) = *(this->m_pRef);
            }
            // 这里左边会隐式转化为父类TS，相当于调用了TS+=TS
            *newPtr += str;
            if (this->m_pRef != nullptr)
            {
                this->m_pRef->Release();
            }
            this->m_pRef = newPtr;
        }

        inline bool operator==(const TC *data) const
        {
            return this->Compare(data) == 0;
        }
        inline bool operator==(const TS &str) const
        {
            return this->Compare(str) == 0;
        }
        inline bool operator==(const ElsaCCTRefString<TS, TC> &str) const
        {
            return this->Compare(str) == 0;
        }

        inline bool operator!=(const TC *data) const
        {
            return this->Compare(data) != 0;
        }
        inline bool operator!=(const TS &str) const
        {
            return this->Compare(str) != 0;
        }
        inline bool operator!=(const ElsaCCTRefString<TS, TC> &str) const
        {
            return this->Compare(str) != 0;
        }

    public:
        inline const TC *RawStr() const
        {
            return this->m_pRef == nullptr ? nullptr : this->m_pRef->RawStr();
        }
        inline TC *RawStr()
        {
            return this->m_pRef == nullptr ? nullptr : this->m_pRef->RawStr();
        }

        inline size_t Length()
        {
            return this->m_pRef == nullptr ? 0 : this->m_pRef->Length();
        }
        //  重新设置长度，注意不会拷贝原来的数据，而是直接清空数据
        inline void SetLength(const size_t len)
        {
            // 至于重置长度之后不会拷贝原来的数据
            CRefTS *newPtr = new CRefTS(len);
            if (this->m_pRef != nullptr)
                this->m_pRef->Release();
            this->m_pRef = newPtr;
        }

        inline int Compare(const TC *data) const
        {
            if (this->m_pRef == nullptr)
                return data == nullptr ? 0 : -1;
            if (data == this->m_pRef->RawStr())
                return 0;
            return this->m_pRef->Compare(data);
        }
        inline int Compare(const TS &str) const
        {
            if (this->m_pRef == nullptr)
                return str.IsEmpty() ? 0 : -1;
            if (this->m_pRef == &str)
                return 0;
            return this->m_pRef->Compare(str);
        }
        inline int Compare(const ElsaCCTRefString<TS, TC> &str) const
        {
            if (this->m_pRef == str.m_pRef)
                return 0;
            if (this->m_pRef == nullptr)
                return -1;
            return this->m_pRef->Compare(str->RawPtr());
        }

        inline size_t Format(const TC *fmt, ...)
        {
            va_list args;
            size_t result = 0;
            va_start(args, fmt);
            result = this->FormatArgs(fmt, args);
            va_end(args);
            return result;
        }

        inline size_t FormatArgs(const TC *fmt, va_list args)
        {
            size_t result = 0;
            CRefTS *newPtr = new CRefTS();

            result = newPtr->FormatArgs(fmt, args);
            this->m_pRef = newPtr;
            return result;
        }
    };

    typedef ElsaCCTRefString<ElsaCAnsiString, char> ElsaCCRefAnsiString;
};

typedef ElsaNString::ElsaCCRefAnsiString ElsaCRefString;

// #define TEST_DEBUG
#ifdef TEST_DEBUG
#include <iostream>
using namespace std;
void ElsaCCTRefStringTestFunc()
{
    ElsaCRefString str1("I am a cool boy!");
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

}
#endif