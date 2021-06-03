
add_library(freetype SHARED IMPORTED GLOBAL)
target_include_directories(freetype INTERFACE
    install/include
    install/include/freetype2
)
set_target_properties(freetype PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/install/bin/freetype.dll
    IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/freetype.lib
)
