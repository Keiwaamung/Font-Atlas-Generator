
add_library(lua SHARED IMPORTED GLOBAL)
target_include_directories(lua INTERFACE
    install/include
)
set_target_properties(lua PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/install/bin/lua54.dll
    IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/lua54.lib
)
