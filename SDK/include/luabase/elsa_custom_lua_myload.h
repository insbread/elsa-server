#pragma once

extern "C"
{
#include "lua.h"
}

// 初始化版本
void LuaInitVersion(lua_State *L);

// 增加版本
void LuaIncVersion(lua_State *L);

// 注册新的requires
void LuaRegisterMyRequire(lua_State *L);