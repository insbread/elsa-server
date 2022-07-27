/***
 * @Author: insbread
 * @Date: 2022-07-27 16:21:12
 * @LastEditTime: 2022-07-27 16:21:13
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/luabase/elsac_lua_config.cpp
 * @版权声明
 */
#include <cstdarg>

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "macro/os_def.h"
#include "macro/_ast.h"
#include "macro/share_utils.h"
#include "container/elsac_vector.h"
#include "elsa_x_tick.h"
#include "elsac_ref_obj.hpp"
#include "string/elsac_ref_string.hpp"
#include "string/elsac_string_ex.h"
#include "luabase/elsac_base_lua.h"
#include "luabase/elsac_lua_config.h"

using namespace ElsaNString;

ElsaCBaseLuaConfig::ElsaCBaseLuaConfig() : Inherit()
{
}

ElsaCBaseLuaConfig::~ElsaCBaseLuaConfig()
{
    for (int i = m_vecTableStacks.Count() - 1; i >= 0; --i)
    {
        m_vecTableStacks[i]->Release();
    }
    m_vecTableStacks.Clear();
}

void ElsaCBaseLuaConfig::ShowError(const char *err)
{
    SetErrDesc(err);
    printf("%s[Configuration Error]", err);
}

void ElsaCBaseLuaConfig::ShowTableNotExists(const char *sTableName)
{
    const char *sTableNamePtr;

    ElsaCString s;
    GetTablePath(s);

    const char *sFmt = (sTableName && !s.IsEmpty())
                           ? ("table \"%s.%s\" not exists")
                           : ("table \"%s%s\" not exists");
    ShowErrorFormat(sFmt, (const char *)s, sTableNamePtr);
}

void ElsaCBaseLuaConfig::ShowFieldDoesNotContainValue(const char *sFieldName, const char *sValueType)
{
    const char *sFieldNamePtr;
    ElsaCString s;
    GetTablePath(s);

    sFieldNamePtr = sFieldName;

    const char *sFmt = (sFieldName && !s.IsEmpty())
                           ? ("field \"%s.%s\" does not contain valid %s value")
                           : ("field \"%s%s\" does not contain valid %s value");
    ShowErrorFormat(sFmt, (const char *)s, sFieldName, sValueType);
}

ElsaCBaseLuaConfig::TableName *ElsaCBaseLuaConfig::CreateTableName(const char *sName)
{
    TableName *tn = new TableName();
    // 子类指针转化为父类(这里转化的目的是不要使用TableName本身的=而是String的=)
    *static_cast<ElsaCString *>(tn) = sName;
    tn->AddRef();
    return tn;
}

bool ElsaCBaseLuaConfig::OpenGlobalTable(const char *sTableName)
{
    lua_getglobal(m_pLua, sTableName);
    bool result = lua_istable(m_pLua, -1);

    if (!result)
    {
        lua_pop(m_pLua, -1);
        ShowTableNotExists(sTableName);
    }
    else
    {
        m_vecTableStacks.Add(CreateTableName(sTableName));
    }
    return result;
}

bool ElsaCBaseLuaConfig::OpenFieldTable(const char *sTableName)
{
    lua_getfield(m_pLua, -1, sTableName);
    bool result = lua_istable(m_pLua, -1);

    if (!result)
    {
        lua_pop(m_pLua, -1);
        ShowTableNotExists(sTableName);
    }
    else
    {
        m_vecTableStacks.Add(CreateTableName(sTableName));
    }
    return result;
}

bool ElsaCBaseLuaConfig::EnumTableFirst()
{
    lua_pushnil(m_pLua); //  遍历表第一个key需要栈顶是nil
    bool result = lua_next(m_pLua, -2) != 0;

    if (result)
    {
        m_vecTableStacks.Add(CreateTableName("[1]"));
    }
    return result;
}

bool ElsaCBaseLuaConfig::EnumTableNext()
{
    lua_pop(m_pLua, -1);                     // 弹出value值
    bool result = lua_next(m_pLua, -2) != 0; // 获取下一个k-v对
    int nListIndex = m_vecTableStacks.Count() - 1;

    if (result)
    {
        int nTableIndex = 0;
        TableName *tn = m_vecTableStacks[nListIndex];
        sscanf((const char *)(*static_cast<ElsaCString *>(tn)), ("[%d]"), &nTableIndex); //  解析[i]中的i到nTableIndex中
        nTableIndex++;
        tn->Format(("[%d]"), nTableIndex); // 将表名从[i]修改为[i + 1]
    }
    else
    {
        // 遍历完了，就需要释放了
        m_vecTableStacks[nListIndex]->Release();
        m_vecTableStacks.Remove(nListIndex);
    }

    return result;
}

void ElsaCBaseLuaConfig::EndTableEnum()
{
    int nListIndex = m_vecTableStacks.Count() - 1;

    if (nListIndex > -1)
    {
        TableName *tn = m_vecTableStacks[nListIndex];
        if (!tn->IsEmpty() && tn->RawStr()[0] == '[')
        {
            m_vecTableStacks[nListIndex]->Release();
            m_vecTableStacks.Remove(nListIndex);
            lua_pop(m_pLua, 2); //  将最后的k-v都弹出来
        }
        else
        {
            ShowErrorFormat(("table is not under enumeration"));
        }
    }
    else
    {
        ShowErrorFormat(("table stack was empty"));
    }
}

bool ElsaCBaseLuaConfig::GetFieldIndexTable(const int indexStartFromOne)
{
    // 基于索引的方式访问表中的成员

    lua_pushinteger(m_pLua, indexStartFromOne); // 将索引压入栈顶
    lua_rawget(m_pLua, -2);                     // 从当前表(表位于倒数第二个位置，倒数第一个位置是一个整数)获取对应栈顶索引的元素压入到栈顶中
    bool result = lua_istable(m_pLua, -1);      // 判断该元素是不是一个表

    if (!result)
    {
        lua_pop(m_pLua, 1); //  弹出元素
        ElsaCString s;
        GetTablePath(s);

        const char *sFmt = !s.IsEmpty()
                               ? ("table \"%s.[%d]\" does not exists")
                               : ("table \"%s[%d]\" does not exists");
        ShowErrorFormat(sFmt, (const char *)s, indexStartFromOne);
    }
    else
    {
        char s[64];
        sprintf(s, "[%d]", indexStartFromOne);
        m_vecTableStacks.Add(CreateTableName(s));
    }

    return result;
}

bool ElsaCBaseLuaConfig::FieldTableExists(const char *sTableName)
{
    lua_getfield(m_pLua, -1, sTableName); //  获取栈顶表中k=sTableName的成员，压入栈顶
    bool result = lua_istable(m_pLua, -1);
    lua_pop(m_pLua, -1); //  弹出栈顶元素
    return result;
}

void ElsaCBaseLuaConfig::CloseTable()
{
    if (lua_gettop(m_pLua) > 0) //  栈元素个数
    {
        lua_pop(m_pLua, 1);
        int nIndex = m_vecTableStacks.Count() - 1;
        m_vecTableStacks[nIndex]->Release();
        m_vecTableStacks.Remove(nIndex);
    }
    else
    {
        ShowErrorFormat(("table stack was empty"));
    }
}

ElsaCString &ElsaCBaseLuaConfig::GetTablePath(ElsaCString &retval)
{
    int i = 0, nCount = 0;
    retval = ("");

    nCount = m_vecTableStacks.Count();

    if (nCount > 0)
    {
        for (i = 0; i < nCount - 1; ++i)
        {
            retval += m_vecTableStacks[i]->RawStr();
            retval += (".");
        }
        retval += m_vecTableStacks[nCount - 1]->RawStr();
    }

    return retval;
}

ElsaCAnsiString &ElsaCBaseLuaConfig::GetKeyName(ElsaCAnsiString &retval)
{
    /*
    ★attention★
    不得使用lua_tostring来直接获取键名称！因为在遍历数组表的时候，键的类型为number，这个时候使用lua_tostring会将键的数据类型修改为string，从而导致后续的遍历将无法进行！！！
    */
    switch (lua_type(m_pLua, -2)) //  栈顶是value，倒数第二个是key值
    {
    case LUA_TNUMBER: //  key值是整数的时候
    {
        lua_Integer n = lua_tointeger(m_pLua, -2) - 1; // LUA中数组索引是从1开始，基于CBP规范，数组索引应当从0开始
        retval.Format("%d", (int)n);
    }
    break;

    case LUA_TSTRING: //  key值是字符串的时候
    {
        const char *s = lua_tostring(m_pLua, -2);
        retval = s;
    }
    break;

    default:
        retval = nullptr;
        break;
    }
    return retval;
}

bool ElsaCBaseLuaConfig::GetFieldBoolean(const char *sFieldName, const bool *pDefValue, BOOL *pIsValid)
{
    if (sFieldName != nullptr)
    {
        // 获取当前表(栈顶表)的sFieldName对应的value并压入到栈顶
        lua_getfield(m_pLua, -1, sFieldName);
    }

    bool result;
    BOOL isValid = lua_isboolean(m_pLua, -1); // 检测是不是boolean值

    if (pIsValid != nullptr)
        *pIsValid = isValid;

    if (isValid)
    {
        // 获取布尔值
        result = (lua_toboolean(m_pLua, -1) != 0);
    }
    else if (pDefValue)
    {
        //  如果存在默认的布尔值，则返回默认的布尔值
        result = *pDefValue;
    }
    else
    {
        // 即无法找到布尔值，也没有默认的布尔值，就错误提示
        result = false;
        ShowFieldDoesNotContainValue(sFieldName, ("boolean"));
    }

    if (sFieldName)
    {
        lua_pop(m_pLua, 1);
    }
    return result;
}

double ElsaCBaseLuaConfig::GetFieldNumber(const char *sFieldName, const double *pDefValue, BOOL *pIsValid)
{
    if (sFieldName != nullptr)
    {
        lua_getfield(m_pLua, -1, sFieldName);
    }

    double result = 0.0;
    BOOL isValid = lua_isnumber(m_pLua, -1);

    if (isValid)
    {
        result = lua_tonumber(m_pLua, -1);
    }
    else if (pDefValue != nullptr)
    {
        result = *pDefValue;
    }
    else
    {
        result = 0;
        ShowFieldDoesNotContainValue(sFieldName, ("numeric"));
    }

    if (sFieldName)
    {
        lua_pop(m_pLua, 1);
    }
    return result;
}

int64_t ElsaCBaseLuaConfig::GetFieldInt64(const char *sFieldName, const int64_t *pDefValue, BOOL *pIsValid)
{
    if (pDefValue)
    {
        double defValue = static_cast<double>(*pDefValue);
        return static_cast<int64_t>(GetFieldNumber(sFieldName, &defValue, pIsValid));
    }
    return static_cast<int64_t>(GetFieldNumber(sFieldName, nullptr, pIsValid));
}

int ElsaCBaseLuaConfig::GetFieldInt(const char *sFieldName, const int *pDefValue, BOOL *pIsValid)
{
    if (pDefValue)
    {
        double defValue = static_cast<double>(*pDefValue);
        return static_cast<int>(GetFieldNumber(sFieldName, &defValue, pIsValid));
    }
    return static_cast<int>(GetFieldNumber(sFieldName, nullptr, pIsValid));
}

const char *ElsaCBaseLuaConfig::GetFieldString(const char *sFieldName, const char *pDefValue, BOOL *pIsValid)
{
    if (sFieldName)
    {
        lua_getfield(m_pLua, -1, sFieldName);
    }

    const char *result;
    BOOL isValid = lua_isstring(m_pLua, -1);

    if (pIsValid)
        *pIsValid = isValid;

    if (isValid)
    {
        result = lua_tostring(m_pLua, -1);
    }
    else if (pDefValue)
    {
        result = pDefValue;
    }
    else
    {
        result = nullptr;
        ShowFieldDoesNotContainValue(sFieldName, ("string"));
    }

    if (sFieldName)
    {
        lua_pop(m_pLua, 1);
    }
    return result;
}

int ElsaCBaseLuaConfig::GetFieldStringBuffer(const char *sFieldName, char *buf, size_t buflen)
{
    if (buflen <= 0)
        return -1;

    BOOL isValid;
    const char *s = GetFieldString(sFieldName, nullptr, &isValid);

    if (isValid == false)
        return 0;

    size_t len = strlen(s);
    len = __min(buflen - 1, len);

    memcpy(buf, s, len * sizeof(buf[0]));
    buf[len] = 0;
    return (int)len;
}

bool ElsaCBaseLuaConfig::ExcuteCjson(const char *funcname, const char *p1, const char *p2, const char *p3)
{
    bool ret = true;
    int top = lua_gettop(m_pLua);

    lua_getglobal(m_pLua, funcname);
    if (p1)
        lua_pushlstring(m_pLua, p1, strlen(p1));
    if (p2)
        lua_pushlstring(m_pLua, p2, strlen(p2));
    if (p3)
        lua_pushlstring(m_pLua, p3, strlen(p3));

    int err = lua_pcall(m_pLua, 3, 0, 0);
    if (err)
    {
        const char *errDesc = nullptr;
        int top = lua_gettop(m_pLua);

        if (top > 0)
        {
            errDesc = lua_tostring(m_pLua, -1);
        }
        if (!errDesc)
        {
            errDesc = "Undefined Error";
        }
        ShowError(errDesc);
        ret = false;
    }
    return ret;
}
