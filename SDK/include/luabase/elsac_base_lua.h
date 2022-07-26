/***
 * @Author: insbread
 * @Date: 2022-07-24 15:45:37
 * @LastEditTime: 2022-07-24 15:45:37
 * @LastEditors: insbread
 * @Description: lua脚本基础对象
 * @FilePath: /elsa-server/SDK/include/luabase/elsac_base_lua.h
 * @版权声明
 */
#pragma once

extern "C"
{
#include "lua.h"
}

#include "memory/elsac_buffer_allocator.h"
#include "datapacker/elsac_data_packet.hpp"

class ElsaCBaseLua
{
protected:
    lua_State *m_pLua;   // lua虚拟机
    int m_iErrno;        // 上一个错误号
    char *m_cpErrDesc;   // 上一个错误的文字描述
    char m_cLastFn[128]; // 上一次调用的函数名称
public:
    ElsaCBaseLua();
    virtual ~ElsaCBaseLua();

protected:
    // 创建lua虚拟机
    virtual lua_State *CreateLuaVM();
    // 打开基础库。默认会打开base、string、math以及table库。返回true表示成功。
    virtual bool OpenBaseLibs();
    // 注册本地函数库，返回true表示成功
    virtual bool RegistLocalLibs();
    // 调用脚本的初始化函数。函数返回true表示没有错误发生。本类未提供调用初始化函数的实际功能。
    virtual bool CallInit();
    // 调用脚本的卸载函数。函数返回true表示没有错误发生。本类未提供调用卸载函数的实际功能。
    virtual bool CallFinal();
    //显示脚本错误内容
    virtual void ShowError(const char *err);

    void SetErrDesc(const char *err);
    // 格式化错误内容并显示，格式化后错误内容字符串的长度被限制为1024个字符。
    void ShowErrorFormat(const char *fmt, ...);
    /*	脚本调用函数
        在将函数以及参数名称入栈后调用从函数用于替代直接调用lua_pcall的行为以便集中错误处理。
        函数中包含对调用函数前后的堆栈平衡检查。
        函数返回true表示没有错误。
    */
    bool Pcall(const int args, const int results, const int errfunc);
    //检查脚本调用返回值，如果nError不为成功值则会输出错误内容并返回false，且nError的值被保存在m_nLastError中。
    inline bool LcCheck(int err);
    /*** 编译脚本为字节码
     * @param {lua_State} *L lua虚拟机
     * @param {char} *content 文本形式的lua脚本内存
     * @param {ElsaCDataPacket} &packet 二进制输出流
     * @return {bool} 成功返回true，失败返回false
     */
    bool CompileLua(lua_State *L, const char *content, ElsaCDataPacket &packet);
    /*** 二进制输出流Writer
     * @param {lua_State} *L lua虚拟机
     * @param {void} *p 待写入的二进制内存
     * @param {size_t} size 二进制内容长度
     * @param {void} *u 这里是ElsaCDataPacket(本来是用户数据)
     * @return {int} 成功返回0，失败返回非0
     */
    static int StreamWriter(lua_State *L, const void *p, size_t size, void *u);

public:
    /* 设置脚本内容，会完成如下核心操作：
       1、调用当前脚本的卸载函数
       2、删除当前的虚拟机对象
       3、重新创建虚拟机
       4、打开基础函数库
       5、注册本地函数库
       6、调用初始化函数
       如果参数sText为NULL或为空字符串则会销毁当前虚拟机且不创建新虚拟机。
       函数返回true表示设置脚本成功，其他值表示发生错误。
    */
    bool SetScript(const char *txt);
    /*** 设置二进制脚本内容，功能基本上和SetScript相似，只不过脚本内容是二进制内容而已
     * @param {char} *script 脚本
     * @param {ElsaCDataPacket} &packet 如果bCompile为true，它是编译后的二进制输出流；否则是二进制输入流
     * @param {char} *name 代码块名称（一般指脚本文件名）
     * @param {bool} compile 为true会先编译脚本为二进制，然后加载二进制；否则直接加载二进制数据
     * @return {bool} 成功返回true；失败返回false
     */
    bool SetBinScript(const char *script, ElsaCDataPacket &packet, const char *name = nullptr, bool compile = false);
    /*** 重置二进制脚本
     * @param {ElsaCDataPacket} &packet 脚本二进制数据
     * @return {bool}
     */
    bool ResetBinScript(ElsaCDataPacket &packet);

    //获取虚拟机的内存使用量，单位是字节
    int GetAvaliableMemorySize();
    // 进行垃圾回收，释放内存。返回回收了多少字节的内存
    int GC();

    // 获取错误号
    inline int GetLastErrnoNo()
    {
        return m_iErrno;
    }
    // 获取错误描述
    inline const char *GetLastErrorDesc() const
    {
        return m_cpErrDesc;
    }
    // 设置上一次调用的函数名称
    void SetLastFunc(const char *name)
    {
        STATIC_ASSERT(sizeof(m_cLastFn) > 8);
        if (name)
        {
            _STRNCPY_A(m_cLastFn, name);
        }
        else
        {
            m_cLastFn[0] = 0;
        }
    }
    // 判断脚本中是否存在fn函数
    bool FunctionExists(const char *fn);
};