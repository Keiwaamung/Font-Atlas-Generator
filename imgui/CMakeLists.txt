
add_executable(test ${IMGUI_SRC})
target_compile_definitions(test PRIVATE
    UNICODE
    _UNICODE
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    IMGUI_USE_BGRA_PACKED_COLOR
    IMGUI_USE_WCHAR32
)
target_sources(test PRIVATE
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    
    imconfig.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui.cpp
    imgui.h
    
    imgui_stdlib.cpp
    imgui_stdlib.h
    
    imgui_impl_dx9.cpp
    imgui_impl_dx9.h
    
    imgui_impl_win32.cpp
    imgui_impl_win32.h
    
    WICTextureLoader9.h
    WICTextureLoader9.cpp
    
    main.cpp
)
target_link_libraries(test PRIVATE
    d3d9.lib
)

add_custom_command(TARGET test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/data
    
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/bin/"$<TARGET_FILE_NAME:test>"
    COMMAND ${CMAKE_COMMAND} -E copy  "$<TARGET_FILE:test>"  ${CMAKE_SOURCE_DIR}/bin
)
