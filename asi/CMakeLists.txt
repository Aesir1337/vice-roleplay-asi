add_library(vice-roleplay SHARED)

target_sources(vice-roleplay PRIVATE
    "src/dllmain.cpp"
    "src/plugin.cpp"
    "src/hooks.cpp"
    "src/actions.cpp"
    "src/game.cpp"
)

if (TARGET plugin-sdk::core)
    target_sources(vice-roleplay PRIVATE
        "src/features/turnlights.cpp"
    )
endif ()

target_include_directories(vice-roleplay PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(vice-roleplay PRIVATE rakhook cyanide::cyanide)

if (TARGET plugin-sdk::core)
    target_link_libraries(vice-roleplay PRIVATE plugin-sdk::core)
    target_compile_definitions(vice-roleplay PRIVATE VICEASI_HAS_PLUGIN_SDK)
endif ()

set_target_properties(vice-roleplay PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "vice-roleplay"
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

if (MSVC)
    target_compile_options(vice-roleplay PRIVATE /W3 /permissive-)
endif ()
