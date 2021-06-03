#include "common.hpp"
#include "builder.hpp"
#include "binding.hpp"
#include "lua.hpp"

int main(int, char**)
{
    fontatlas::ScopeCoInitialize co;
    
    lua_State* L = luaL_newstate();
    if (L)
    {
        luaL_openlibs(L);
        fontatlas::lua_fontatlas_open(L);
        lua_settop(L, 0);
        fontatlas::lua_safe_dofile(L, "config.lua");
        lua_close(L);
        L = NULL;
    }
    
    return 0;
}
