/***
 * @Author: insbread
 * @Date: 2022-07-24 16:19:37
 * @LastEditTime: 2022-07-24 16:19:38
 * @LastEditors: insbread
 * @Description:
 * @FilePath: /elsa-server/SDK/src/luabase/elsac_base_lua.cpp
 * @版权声明
 */
extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lstate.h"
#include "lundump.h"
}

#include <cstdarg>
#include "macro/_ast.h"
#include "macro/share_utils.h"

#include "elsa_x_tick.h"
#include "luabase/elsac_base_lua.h"
#include "luabase/elsa_custom_lua_myload.h"

#include "string/elsac_ref_string.hpp"
#include "datapacker/elsac_data_packet.hpp"

ElsaCBaseLua::ElsaCBaseLua()
{
    m_pLua = nullptr;
    m_iErrno = 0;
    m_cLastFn[0] = 0;
    m_cpErrDesc = nullptr;
}

ElsaCBaseLua::~ElsaCBaseLua()
{
    SetScript(nullptr);
    if (m_pLua)
    {
        lua_close(m_pLua);
        m_pLua = nullptr;
    }
    SafeDelete(m_cpErrDesc);
}

bool ElsaCBaseLua::FunctionExists(const char *fn)
{
    if (m_pLua == nullptr || fn == nullptr || fn[0] == 0)
        return false;

    lua_getglobal(m_pLua, fn); //  从全局变量表中找到将函数名称对应的函数变量，并压入栈顶中
    bool result = lua_isfunction(m_pLua, -1);
    lua_pop(m_pLua, 1); //  将函数变量弹出
    return result;
}

bool ElsaCBaseLua::CallFinal()
{
    return true;
}

bool ElsaCBaseLua::CallInit()
{
    return true;
}

bool ElsaCBaseLua::RegistLocalLibs()
{
    return true;
}

lua_State *ElsaCBaseLua::CreateLuaVM()
{
    return luaL_newstate();
}

bool ElsaCBaseLua::OpenBaseLibs()
{
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_base, nullptr))) //  打开基础库
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_string, nullptr))) //  打开基础string库
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_math, nullptr))) //  打开数学库
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_table, nullptr))) //  打开表解析库
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_io, nullptr))) //  打开基础io库
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_debug, nullptr)))
        return false;
    if (!LcCheck(lua_cpcall(m_pLua, luaopen_package, nullptr)))
        return false;
}

bool ElsaCBaseLua::Pcall(const int args, const int results, const int errfunc)
{
    bool result = true;

    int nTop = lua_gettop(m_pLua) - args - 1; //  获取除去参数和函数后剩下的栈元素个数
    result = LcCheck(lua_pcall(m_pLua, args, results, errfunc));
    int nTop2 = lua_gettop(m_pLua) - results; //  获取返回值的个数
    if (results != LUA_MULTRET && nTop != nTop2)
    {
        ShowErrorFormat("function:%s,the stack before call was:%d,the stack after call is:%d,stack difference value is:%d",
                        m_cLastFn, nTop, nTop2, nTop2 - nTop);
    }
    return result;
}

bool ElsaCBaseLua::LcCheck(int err)
{
    if (!err)
        return true;

    m_iErrno = err;

    const char *errdesc = nullptr;

    if (lua_gettop(m_pLua) > 0)
    {
        errdesc = lua_tostring(m_pLua, -1); //  获取栈顶上的错误描述
        lua_pop(m_pLua, -1);
    }
    else
    {
        errdesc = ("falt system error: lua_gettop <= 0"); //  栈顶没有错误的描述
    }

    if (errdesc == nullptr || errdesc[0] == 0)
        errdesc = ("undefined error");

    ShowErrorFormat("function:%s,error:%s", m_cLastFn, errdesc);
    return false;
}

bool ElsaCBaseLua::SetScript(const char *txt)
{
    if (m_pLua != nullptr)
    {
        // 销毁当前的LUA虚拟机
        CallFinal();
    }

    if (txt != nullptr)
    {
        // 如果txt是utf-bom编码，需要跳过前三个字节
        if ((*reinterpret_cast<int *>(const_cast<char *>(txt)) & 0x00FFFFFF) == 0xBFBBEF)
            txt += 3;
    }
    else if (m_pLua != nullptr)
    {
        lua_close(m_pLua);
        m_pLua = nullptr;
    }

    if (txt != nullptr && txt[0] != 0)
    {
        if (m_pLua == nullptr)
        {
            m_pLua = CreateLuaVM();       // 创建虚拟机
            OpenBaseLibs();               // 打开基本函数库
            LuaInitVersion(m_pLua);       // 初始化lua版本
            LuaRegisterMyRequire(m_pLua); // 注册新的require
            RegistLocalLibs();            // 注册本地函数库
            SetLastFunc("<LOADER>");      // 保存错误函数名称
        }

        LuaIncVersion(m_pLua); //  增加版本

        if (!LcCheck(luaL_loadstring(m_pLua, txt))) //  将txt作为lua代码块加载到栈顶，出错就将错误号加载到栈顶
            return false;

        // 运行刚刚加载的初始化脚本
        if (!Pcall(0, 0, 0))
            return false;

        // 初始化函数
        return CallInit();
    }
    return true;
}

int ElsaCBaseLua::StreamWriter(lua_State *L, const void *p, size_t size, void *u)
{
    UNUSED(L);
    ElsaCDataPacket *packet = static_cast<ElsaCDataPacket *>(u);

    if (packet == nullptr)
        return 1;
    packet->WriteBuf(p, size);
    return 0;
}

bool ElsaCBaseLua::CompileLua(lua_State *L, const char *content, ElsaCDataPacket &packet)
{
    if (L == nullptr || content == nullptr)
        return false;

    if (!LcCheck(luaL_loadstring(L, content)))
        return false;

    packet.SetLength(0);
    const Proto *proto = clvalue(L->top - 1)->l.p;
    luaU_dump(L, proto, ElsaCBaseLua::StreamWriter, &packet, 0); //  这里应该是将栈顶的代码段编译为二进制码存到packet中
    return true;
}

bool ElsaCBaseLua::ResetBinScript(ElsaCDataPacket &packet)
{
    if (m_pLua == nullptr)
        return false;
    CallFinal(); //  卸载原来的脚本函数
    if (!LcCheck(luaL_loadbuffer(m_pLua, packet.GetMemoryPtr(), packet.GetPosition(), "")))
        //将之前读入的数据，载入到栈顶，注意由于将数据写入到packet中，offset会移动到数据末尾，所以GetPosition相当于获取写入数据的长度
        return false;
    LuaIncVersion(m_pLua);

    // 运行加载的脚本函数
    if (!Pcall(0, 0, 0))
        return false;
    return CallInit();
}

bool ElsaCBaseLua::SetBinScript(const char *script, ElsaCDataPacket &packet, const char *name, bool compile)
{
    if (m_pLua != nullptr)
    {
        CallFinal();
    }

    if ((compile && script) || (!compile && packet.GetPosition() > 0))
    {
        if (m_pLua == nullptr)
        {
            m_pLua = CreateLuaVM();
            OpenBaseLibs();
            LuaInitVersion(m_pLua);
            LuaRegisterMyRequire(m_pLua);
            RegistLocalLibs();
            SetLastFunc("<LOADER>");
        }

        LuaIncVersion(m_pLua);

        // 将二进制脚本进行编译
        if (compile)
        {
            // 将脚本进行编译，并存放到packet中
            if (!CompileLua(m_pLua, script, packet))
                return false;
        }
        //  将二进制脚本存放到栈顶
        if (!LcCheck(luaL_loadbuffer(m_pLua, packet.GetMemoryPtr(), packet.GetPosition(), name)))
            return false;

        if (!Pcall(0, 0, 0))
            return false;

        return CallInit();
    }
    return true;
}

int ElsaCBaseLua::GetAvaliableMemorySize()
{
    if (m_pLua == nullptr)
        return 0;
    int n = lua_gc(m_pLua, LUA_GCCOUNT, 0) * 1024;
    n |= lua_gc(m_pLua, LUA_GCCOUNTB, 0);
    return 0;
}

int ElsaCBaseLua::GC()
{
    if (m_pLua == nullptr)
        return 0;

    int n = GetAvaliableMemorySize();
    lua_gc(m_pLua, LUA_GCCOLLECT, 0);
    return n - GetAvaliableMemorySize();
}

void ElsaCBaseLua::SetErrDesc(const char *err)
{
    SafeFree(m_cpErrDesc);
    size_t len = strlen(err);
    m_cpErrDesc = static_cast<char *>(malloc(len + 1));
    _STRNCPY_S(m_cpErrDesc, err, len + 1);
}

void ElsaCBaseLua::ShowError(const char *err)
{
    SetErrDesc(err);
    printf("%s", err);
}

void ElsaCBaseLua::ShowErrorFormat(const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    VSNPRINTFA(buf, ArrayCount(buf) - 1, fmt, args);
    va_end(args);

    ShowError(buf);
}
