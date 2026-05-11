add_library(vice-roleplay SHARED)

target_sources(vice-roleplay PRIVATE
    "src/dllmain.cpp"
    "src/plugin.cpp"
    "src/hooks.cpp"
    "src/actions.cpp"
)

target_include_directories(vice-roleplay PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(vice-roleplay PRIVATE rakhook cyanide::cyanide)

set_target_properties(vice-roleplay PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "vice-roleplay"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

if (MSVC)
    target_compile_options(vice-roleplay PRIVATE /W3 /permissive-)
endif ()
