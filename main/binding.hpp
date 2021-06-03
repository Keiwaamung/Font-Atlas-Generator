#pragma once
#include "common.hpp"
#include "builder.hpp"
#include "lua.hpp"
#include <cassert>
#include <cstdio>

namespace fontatlas
{
    constexpr char lua_module_fontatlas[] = "fontatlas";
    constexpr char lua_class_fontatlas_Builder[] = "fontatlas.Builder";
    
    struct BuilderWrapper
    {
        Builder* builder = nullptr;
        
        static Builder* luaCast(lua_State* L, int n)
        {
            BuilderWrapper* udata = (BuilderWrapper*)luaL_checkudata(L, n, lua_class_fontatlas_Builder);
            assert(udata->builder);
            return udata->builder;
        }
        static int luaRegister(lua_State* L)
        {
            const luaL_Reg cls_lib[] = {
                {"addFont", &addFont},
                {"addCode", &addCode},
                {"addRange", &addRange},
                {"addText", &addText},
                {"build", &build},
                {NULL, NULL},
            };
            const luaL_Reg mt_lib[] = {
                {"__tostring", &__tostring},
                {"__gc", &__gc},
                {NULL, NULL},
            };
            
            luaL_newmetatable(L, lua_class_fontatlas_Builder);  // ? M mt
            luaL_setfuncs(L, mt_lib, 0);                        // ? M mt
            lua_newtable(L);                                    // ? M mt cls
            luaL_setfuncs(L, cls_lib, 0);                       // ? M mt cls
            lua_setfield(L, -2, "__index");                     // ? M mt
            lua_pop(L, 1);                                      // ? M
            
            const luaL_Reg M_lib[] = {
                {"Builder", &__create},
                {NULL, NULL},
            };
            
            luaL_setfuncs(L, M_lib, 0);                         // ? M
            
            return 0;
        }
        static Builder* luaCreate(lua_State* L)
        {
            BuilderWrapper* udata = (BuilderWrapper*)lua_newuserdata(L, sizeof(BuilderWrapper));
            udata->builder = new Builder;
            luaL_getmetatable(L, lua_class_fontatlas_Builder);
            lua_setmetatable(L, -2);
            return udata->builder;
        }
        
        static int addFont(lua_State* L)
        {
            Builder* self = luaCast(L, 1);
            const char* name = luaL_checkstring(L, 2);
            const char* path = luaL_checkstring(L, 3);
            const uint32_t face = (uint32_t)luaL_checkinteger(L, 4);
            const uint32_t size = (uint32_t)luaL_checkinteger(L, 5);
            const bool ret = self->addFont(name, path, face, size);
            lua_pushboolean(L, ret);
            return 1;
        }
        static int addCode(lua_State* L)
        {
            Builder* self = luaCast(L, 1);
            const char* name = luaL_checkstring(L, 2);
            const uint32_t c = (uint32_t)luaL_checkinteger(L, 3);
            const bool ret = self->addCode(name, c);
            lua_pushboolean(L, ret);
            return 1;
        }
        static int addRange(lua_State* L)
        {
            Builder* self = luaCast(L, 1);
            const char* name = luaL_checkstring(L, 2);
            const uint32_t a = (uint32_t)luaL_checkinteger(L, 3);
            const uint32_t b = (uint32_t)luaL_checkinteger(L, 4);
            const bool ret = self->addRange(name, a, b);
            lua_pushboolean(L, ret);
            return 1;
        }
        static int addText(lua_State* L)
        {
            Builder* self = luaCast(L, 1);
            const char* name = luaL_checkstring(L, 2);
            size_t text_length = 0;
            const char* text = luaL_checklstring(L, 3, &text_length);
            const bool ret = self->addText(name, std::string_view(text, text_length));
            lua_pushboolean(L, ret);
            return 1;
        }
        static int build(lua_State* L)
        {
            Builder* self = luaCast(L, 1);
            const char* path = luaL_checkstring(L, 2);
            const bool is_multi_channel = lua_toboolean(L, 3);
            const uint32_t texture_width = (uint32_t)luaL_checkinteger(L, 4);
            const uint32_t texture_height = (uint32_t)luaL_checkinteger(L, 5);
            const uint32_t texture_edge = (uint32_t)luaL_checkinteger(L, 6);
            const uint32_t glyph_edge = (uint32_t)luaL_checkinteger(L, 7);
            const bool ret = self->build(path,
                is_multi_channel, texture_width, texture_height, texture_edge,
                glyph_edge);
            lua_pushboolean(L, ret);
            return 1;
        }
        
        static int __tostring(lua_State* L)
        {
            lua_pushstring(L, lua_class_fontatlas_Builder);
            return 1;
        };
        static int __gc(lua_State* L)
        {
            BuilderWrapper* udata = (BuilderWrapper*)luaL_checkudata(L, 1, lua_class_fontatlas_Builder);
            if (udata->builder)
            {
                delete udata->builder;
                udata->builder = nullptr;
            }
            return 0;
        }
        
        static int __create(lua_State* L)
        {
            luaCreate(L);
            return 1;
        }
    };
    
    int lua_fontatlas_open(lua_State* L)
    {
        struct Wrapper
        {
            static int __require(lua_State* L)
            {
                lua_newtable(L);
                return 1;
            }
        };
        luaL_requiref(L, lua_module_fontatlas, &Wrapper::__require, true);
        BuilderWrapper::luaRegister(L);
        return 1;
    }
    
    static lua_State* GL = NULL;
    static int lua_stack_traceback(lua_State *L)
    {
        // errmsg
        luaL_traceback(GL, L, lua_tostring(L, 1), 1);
        return 1;
    }
    
    void lua_safe_dofile(lua_State* L, const std::string_view path)
    {
        GL = L;
        lua_pushcfunction(L, &lua_stack_traceback);
        const int funindex = lua_gettop(L);
        Buffer data = std::move(readFile(path));
        const int loadret = luaL_loadbuffer(L, (char*)data.data(), data.size(), path.data());
        if (loadret != LUA_OK)
        {
            const std::string_view errmsg = lua_tostring(L, -1);
            std::printf("%s\n", errmsg.data());
            lua_pop(L, 1);
            return;
        }
        const int callret = lua_pcall(L, 0, 0, funindex);
        if (callret != LUA_OK)
        {
            const std::string_view errmsg = lua_tostring(L, -1);
            std::printf("%s\n", errmsg.data());
            lua_pop(L, 1);
            return;
        }
    }
}
