
add_executable(fontatlas)
set_target_properties(fontatlas PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    C_STANDARD 11
    CXX_STANDARD 20
)
target_compile_options(fontatlas PRIVATE
    "/utf-8"
)
target_include_directories(fontatlas PRIVATE
    ./
)
target_sources(fontatlas PRIVATE
    common.hpp
    common.cpp
    logger.hpp
    logger.cpp
    texture.hpp
    texture.cpp
    utf.hpp
    builder.hpp
    builder.cpp
    binding.hpp
    main.cpp
    fontatlas.manifest
)
target_link_libraries(fontatlas PRIVATE
    windowscodecs.lib
    freetype
    lua
)

add_custom_command(TARGET fontatlas POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/data
    
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/bin/"$<TARGET_FILE_NAME:freetype>"
    COMMAND ${CMAKE_COMMAND} -E copy  "$<TARGET_FILE:freetype>"  ${CMAKE_SOURCE_DIR}/bin
    
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/bin/"$<TARGET_FILE_NAME:lua>"
    COMMAND ${CMAKE_COMMAND} -E copy  "$<TARGET_FILE:lua>"  ${CMAKE_SOURCE_DIR}/bin
    
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_SOURCE_DIR}/bin/"$<TARGET_FILE_NAME:fontatlas>"
    COMMAND ${CMAKE_COMMAND} -E copy  "$<TARGET_FILE:fontatlas>"  ${CMAKE_SOURCE_DIR}/bin
)
