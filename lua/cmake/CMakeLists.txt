cmake_minimum_required(VERSION 3.20)
project(lua54)

set(LUA_ROOT "../lua-5.4.3/src")

set(LUA_HEADER
    ${LUA_ROOT}/lua.h
    ${LUA_ROOT}/luaconf.h
    ${LUA_ROOT}/lualib.h
    ${LUA_ROOT}/lauxlib.h
    ${LUA_ROOT}/lua.hpp
)

set(LUA_CORE
    ${LUA_ROOT}/lapi.c
    ${LUA_ROOT}/lapi.h
    ${LUA_ROOT}/lcode.c
    ${LUA_ROOT}/lcode.h
    ${LUA_ROOT}/lctype.c
    ${LUA_ROOT}/lctype.h
    ${LUA_ROOT}/ldebug.c
    ${LUA_ROOT}/ldebug.h
    ${LUA_ROOT}/ldo.c
    ${LUA_ROOT}/ldo.h
    ${LUA_ROOT}/ldump.c
    ${LUA_ROOT}/lfunc.c
    ${LUA_ROOT}/lfunc.h
    ${LUA_ROOT}/lgc.c
    ${LUA_ROOT}/lgc.h
    ${LUA_ROOT}/ljumptab.h
    ${LUA_ROOT}/llex.c
    ${LUA_ROOT}/llex.h
    ${LUA_ROOT}/llimits.h
    ${LUA_ROOT}/lmem.c
    ${LUA_ROOT}/lmem.h
    ${LUA_ROOT}/lobject.c
    ${LUA_ROOT}/lobject.h
    ${LUA_ROOT}/lopcodes.c
    ${LUA_ROOT}/lopcodes.h
    ${LUA_ROOT}/lopnames.h
    ${LUA_ROOT}/lparser.c
    ${LUA_ROOT}/lparser.h
    ${LUA_ROOT}/lprefix.h
    ${LUA_ROOT}/lstate.c
    ${LUA_ROOT}/lstate.h
    ${LUA_ROOT}/lstring.c
    ${LUA_ROOT}/lstring.h
    ${LUA_ROOT}/ltable.c
    ${LUA_ROOT}/ltable.h
    ${LUA_ROOT}/ltm.c
    ${LUA_ROOT}/ltm.h
    ${LUA_ROOT}/lua.h
    ${LUA_ROOT}/luaconf.h
    ${LUA_ROOT}/lundump.c
    ${LUA_ROOT}/lundump.h
    ${LUA_ROOT}/lvm.c
    ${LUA_ROOT}/lvm.h
    ${LUA_ROOT}/lzio.c
    ${LUA_ROOT}/lzio.h
)

set(LUA_AUX
    ${LUA_ROOT}/lauxlib.c
    ${LUA_ROOT}/lauxlib.h
    ${LUA_ROOT}/lbaselib.c
    ${LUA_ROOT}/lcorolib.c
    ${LUA_ROOT}/ldblib.c
    ${LUA_ROOT}/linit.c
    ${LUA_ROOT}/liolib.c
    ${LUA_ROOT}/lmathlib.c
    ${LUA_ROOT}/loadlib.c
    ${LUA_ROOT}/loslib.c
    ${LUA_ROOT}/lstrlib.c
    ${LUA_ROOT}/ltablib.c
    ${LUA_ROOT}/lualib.h
    ${LUA_ROOT}/lutf8lib.c
)

include_directories(${LUA_ROOT})

add_library(lua54 SHARED ${LUA_CORE} ${LUA_AUX})
target_compile_definitions(lua54 PUBLIC LUA_BUILD_AS_DLL)

add_executable(lua ${LUA_ROOT}/lua.c)
target_compile_definitions(lua PUBLIC LUA_BUILD_AS_DLL)
target_link_libraries(lua lua54)

add_executable(luac ${LUA_CORE} ${LUA_AUX} ${LUA_ROOT}/luac.c)

set_target_properties(lua54 lua luac PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

include(GNUInstallDirs)
install(TARGETS lua54 lua luac
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ${LUA_HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
