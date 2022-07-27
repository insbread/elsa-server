#pragma once

extern "C"
{
#include "lua.h"
}

// 初始化版本，在全局注册表中添加_script_version字段表示整数的版本号，以及插入_script_version_set的空表
void LuaInitVersion(lua_State *L);

// 增加版本，将注册表中的_script_version字段代表的整数值+1
void LuaIncVersion(lua_State *L);

// 注册新的requires
void LuaRegisterMyRequire(lua_State *L);