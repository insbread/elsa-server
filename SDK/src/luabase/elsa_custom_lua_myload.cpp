extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luabase/elsac_base_lua.h"

void LuaInitVersion(lua_State *L)
{
    lua_pushinteger(L, 0);
    lua_setfield(L, LUA_REGISTRYINDEX, "_script_version");

    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "_script_version_set");
}

void LuaIncVersion(lua_State *L)
{
    // 将当前版本号读取出来
    lua_getfield(L, LUA_REGISTRYINDEX, "_script_version");
    int version = (int)lua_tointeger(L, -1);

    lua_pop(L, 1);
    version += 1;
    lua_pushinteger(L, version);
    lua_setfield(L, LUA_REGISTRYINDEX, "_script_version");
}

static const int sentinel_ = 0;
#define sentinel ((void *)&sentinel_)

static int LuaMyRequires(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    int i;
    lua_settop(L, 1); /* _LOADED table will be at index 2 */
    lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, LUA_REGISTRYINDEX, "_script_version");     // index 3 is current script version
    lua_getfield(L, LUA_REGISTRYINDEX, "_script_version_set"); // index 4 is script version set

    lua_getfield(L, 2, name);
    if (lua_toboolean(L, -1))
    { /* is it there? */
        lua_getfield(L, 4, name);
        int isEqual = lua_equal(L, -1, 3);
        lua_pop(L, 1);
        if (isEqual)
        {
            if (lua_touserdata(L, -1) == sentinel) /* check loops */
                luaL_error(L, "loop or previous error loading module " LUA_QS, name);
            return 1; /* package is already loaded */
        }
    }
    /* else must load it; iterate over available loaders */
    lua_getfield(L, LUA_GLOBALSINDEX, "package");
    lua_getfield(L, -1, "loaders");
    if (!lua_istable(L, -1))
        luaL_error(L, LUA_QL("package.loaders") " must be a table");
    lua_pushliteral(L, ""); /* error message accumulator */
    for (i = 1;; i++)
    {
        lua_rawgeti(L, -2, i); /* get a loader */
        if (lua_isnil(L, -1))
            luaL_error(L, "module " LUA_QS " not found:%s",
                       name, lua_tostring(L, -2));
        lua_pushstring(L, name);
        lua_call(L, 1, 1);            /* call it */
        if (lua_isfunction(L, -1))    /* did it find module? */
            break;                    /* module loaded successfully */
        else if (lua_isstring(L, -1)) /* loader returned error message? */
            lua_concat(L, 2);         /* accumulate it */
        else
            lua_pop(L, 1);
    }
    lua_pushlightuserdata(L, sentinel);
    lua_setfield(L, 2, name);     /* _LOADED[name] = sentinel */
    lua_pushstring(L, name);      /* pass name as argument to module */
    lua_call(L, 1, 1);            /* run loaded module */
    if (!lua_isnil(L, -1))        /* non-nil return? */
        lua_setfield(L, 2, name); /* _LOADED[name] = returned value */
    lua_getfield(L, 2, name);
    if (lua_touserdata(L, -1) == sentinel)
    {                             /* module did not set a value? */
        lua_pushboolean(L, 1);    /* use true as result */
        lua_pushvalue(L, -1);     /* extra copy to be returned */
        lua_setfield(L, 2, name); /* _LOADED[name] = true */
    }

    lua_pushvalue(L, 3);
    lua_setfield(L, 4, name); // write current version

    return 1;
}

void LuaRegisterMyRequire(lua_State *L)
{
    lua_pushcfunction(L, LuaMyRequires);
    lua_setfield(L, LUA_GLOBALSINDEX, "require");
}