include(FetchContent)

# ---- RenderWare SDK location ----------------------------------------------
set(_rw_sdk_root "${VICEASI_RW_SDK_ROOT}")
if (NOT _rw_sdk_root AND DEFINED ENV{RWGSDK})
    set(_rw_sdk_root "$ENV{RWGSDK}")
endif ()
if (NOT _rw_sdk_root AND DEFINED ENV{RWGDIR})
    set(_rw_sdk_root "$ENV{RWGDIR}/rwsdk")
endif ()
if (NOT _rw_sdk_root)
    message(FATAL_ERROR
        "VICEASI_USE_PLUGIN_SDK is ON but RenderWare SDK could not be located. "
        "Set RWGSDK or RWGDIR environment variable, or pass -DVICEASI_RW_SDK_ROOT=<path>.")
endif ()
file(TO_CMAKE_PATH "${_rw_sdk_root}" _rw_sdk_root)

set(VICEASI_RW_PLATFORM "d3d9" CACHE STRING "RenderWare platform variant (d3d8 / d3d9 / null / opengl)")
set(_rw_inc "${_rw_sdk_root}/include/${VICEASI_RW_PLATFORM}")
set(_rw_lib "${_rw_sdk_root}/lib/${VICEASI_RW_PLATFORM}/$<IF:$<CONFIG:Debug>,debug,release>")

if (NOT EXISTS "${_rw_inc}/rwcore.h")
    message(FATAL_ERROR "rwcore.h not found at ${_rw_inc} — check VICEASI_RW_PLATFORM or RW SDK layout.")
endif ()

# RW samples ship `skeleton.h` outside the SDK include tree.
get_filename_component(_rw_parent "${_rw_sdk_root}" DIRECTORY)
set(_rw_skel "")
foreach (_candidate "${_rw_parent}/shared/skel" "${_rw_sdk_root}/shared/skel" "${_rw_parent}/../shared/skel")
    if (EXISTS "${_candidate}/skeleton.h")
        set(_rw_skel "${_candidate}")
        break()
    endif ()
endforeach ()

message(STATUS "RenderWare SDK : ${_rw_sdk_root}  (platform: ${VICEASI_RW_PLATFORM})")
message(STATUS "RenderWare skel: ${_rw_skel}")

# ---- plugin-sdk source fetch ----------------------------------------------
FetchContent_Declare(
    plugin_sdk
    GIT_REPOSITORY https://github.com/DK22Pac/plugin-sdk.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(plugin_sdk)

# ---- Minimal core library ----
# plugin-sdk is mostly header-only inline calls into game addresses. We only
# compile the shared infrastructure (Pattern/Patch/PluginBase/...) plus the
# small set of game_sa source files we actually rely on.
set(_pssrc "${plugin_sdk_SOURCE_DIR}")
set(PLUGIN_SDK_CORE_SOURCES
    "${_pssrc}/shared/PluginBase.cpp"
    "${_pssrc}/shared/Patch.cpp"
    "${_pssrc}/shared/Pattern.cpp"
    "${_pssrc}/shared/DynAddress.cpp"
    "${_pssrc}/shared/GameVersion.cpp"
    "${_pssrc}/shared/Other.cpp"
    "${_pssrc}/shared/Color.cpp"
    "${_pssrc}/shared/StringUtils.cpp"
    "${_pssrc}/shared/Timer.cpp"
    "${_pssrc}/shared/Image.cpp"
    "${_pssrc}/shared/common_sdk.cpp"
    "${_pssrc}/shared/game/CVector.cpp"
    "${_pssrc}/shared/game/CRGBA.cpp"
    "${_pssrc}/shared/game/CompressedVector.cpp"
    "${_pssrc}/shared/game/CompressedVector2D.cpp"
    "${_pssrc}/shared/plugin.cpp"
    "${_pssrc}/plugin_sa/game_sa/common.cpp"
    "${_pssrc}/plugin_sa/game_sa/CMessages.cpp"
    "${_pssrc}/plugin_sa/game_sa/CWorld.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPlayerInfo.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPlayerPed.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPed.cpp"
    "${_pssrc}/plugin_sa/game_sa/CEntity.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPhysical.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPlaceable.cpp"
    "${_pssrc}/plugin_sa/game_sa/CVehicle.cpp"
    "${_pssrc}/plugin_sa/game_sa/CCoronas.cpp"
    "${_pssrc}/plugin_sa/game_sa/CGeneral.cpp"
    "${_pssrc}/plugin_sa/game_sa/CModelInfo.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPathFind.cpp"
    "${_pssrc}/plugin_sa/game_sa/CPools.cpp"
    "${_pssrc}/plugin_sa/game_sa/CTimer.cpp"
    "${_pssrc}/plugin_sa/game_sa/CCamera.cpp"
    "${_pssrc}/plugin_sa/game_sa/CBaseModelInfo.cpp"
)

add_library(plugin_sdk_core STATIC ${PLUGIN_SDK_CORE_SOURCES})
add_library(plugin-sdk::core ALIAS plugin_sdk_core)

target_include_directories(plugin_sdk_core SYSTEM PUBLIC
    "${_pssrc}/plugin_sa"
    "${_pssrc}/plugin_sa/game_sa"
    "${_pssrc}/plugin_sa/game_sa/enums"
    "${_pssrc}/plugin_sa/game_sa/meta"
    "${_pssrc}/plugin_sa/game_sa/rw"
    "${_pssrc}/shared"
    "${_pssrc}/shared/game"
    "${_pssrc}/shared/extensions"
    "${_pssrc}/shared/extensions/scripting"
    "${_pssrc}/shared/extender"
    "${_pssrc}/shared/rwd3d9"
    "${_pssrc}/shared/dxsdk"
    "${_pssrc}/shared/bass"
    "${_pssrc}/shared/comp"
    "${_pssrc}/shared/comp/plugins"
    "${_pssrc}/injector"
    "${_pssrc}/injector/gvm"
    "${_pssrc}/safetyhook"
    "${_pssrc}/hooking"
    "${_rw_inc}"
)
if (_rw_skel)
    target_include_directories(plugin_sdk_core SYSTEM PUBLIC "${_rw_skel}")
endif ()

target_compile_definitions(plugin_sdk_core PUBLIC
    GTASA
    RW
    _CRT_SECURE_NO_WARNINGS
    RWLIBRARYBASEVERSION=0x36003
)

set_target_properties(plugin_sdk_core PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

if (MSVC)
    target_compile_options(plugin_sdk_core PRIVATE
        /W0 /bigobj /wd4005
        /Zc:__cplusplus /std:c++latest /permissive
        "/FIplugin.h"
    )
    # Consumers compiling plugin-sdk-using sources need permissive + latest too.
    target_compile_options(plugin_sdk_core INTERFACE
        /std:c++latest /permissive
    )
endif ()

# RW link directory + the two libs every plugin needs.
target_link_directories(plugin_sdk_core PUBLIC "${_rw_lib}")
target_link_libraries(plugin_sdk_core PUBLIC rwcore rpworld)
