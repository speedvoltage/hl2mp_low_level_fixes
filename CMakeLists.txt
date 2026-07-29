cmake_minimum_required(VERSION 3.21)
if(POLICY CMP0169)
	cmake_policy(SET CMP0169 OLD)
endif()
project(hl2mp_lowlevel_fixes VERSION 0.1.0 LANGUAGES C CXX ASM)

set(MMSOURCE_ROOT "" CACHE PATH "Path to Metamod:Source 2.0 source tree")
set(HL2SDK_ROOT "" CACHE PATH "Path to the HL2DM SDK root or Valve Source SDK src directory")

if(NOT MMSOURCE_ROOT OR NOT EXISTS "${MMSOURCE_ROOT}/core/ISmmPlugin.h")
	message(FATAL_ERROR "Set MMSOURCE_ROOT to the Metamod:Source source tree")
endif()

if(EXISTS "${HL2SDK_ROOT}/src/public/eiface.h")
	set(HL2SDK_ROOT "${HL2SDK_ROOT}/src")
endif()
if(NOT HL2SDK_ROOT OR NOT EXISTS "${HL2SDK_ROOT}/public/eiface.h")
	message(FATAL_ERROR "Set HL2SDK_ROOT to alliedmodders/hl2sdk hl2dm or Valve's Source SDK src directory")
endif()

if(CMAKE_SIZEOF_VOID_P AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "This release targets x86. Configure Visual Studio with -A Win32 or Linux with a 32-bit toolchain.")
endif()

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
	set(CAPSTONE_BUILD_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
endif()

set(FUNCHOOK_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(FUNCHOOK_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(FUNCHOOK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FUNCHOOK_INSTALL OFF CACHE BOOL "" FORCE)
set(FUNCHOOK_DISASM capstone CACHE STRING "" FORCE)
include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/compat/include")
if(UNIX AND EXISTS "/usr/include/x86_64-linux-gnu")
	include_directories(BEFORE SYSTEM "/usr/include/x86_64-linux-gnu")
	if(EXISTS "/usr/include/c++/14")
		include_directories(BEFORE SYSTEM "/usr/include/c++/14" "/usr/include/x86_64-linux-gnu/c++/14" "/usr/include/c++/14/backward")
	endif()
endif()
set(FETCHCONTENT_SOURCE_DIR_CAPSTONE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/capstone" CACHE PATH "" FORCE)
add_subdirectory(third_party/funchook EXCLUDE_FROM_ALL)

if(TARGET funchook-static)
	set_property(TARGET funchook-static PROPERTY INTERFACE_LINK_LIBRARIES capstone)
endif()

add_library(hl2mp_lowlevel_fixes SHARED
	src/plugin.cpp
	src/plugin.h
)

if(NOT MSVC)
	target_sources(hl2mp_lowlevel_fixes PRIVATE compat/src/cxx_new_delete_shim.cpp)
endif()

target_compile_features(hl2mp_lowlevel_fixes PRIVATE cxx_std_11)
target_include_directories(hl2mp_lowlevel_fixes PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${MMSOURCE_ROOT}/core"
	"${MMSOURCE_ROOT}/core/sourcehook"
	"${HL2SDK_ROOT}/public"
	"${HL2SDK_ROOT}/public/engine"
	"${HL2SDK_ROOT}/public/mathlib"
	"${HL2SDK_ROOT}/public/vstdlib"
	"${HL2SDK_ROOT}/public/tier0"
	"${HL2SDK_ROOT}/public/tier1"
	"${HL2SDK_ROOT}/public/game/server"
	"${CMAKE_CURRENT_SOURCE_DIR}/compat/include"
)

target_compile_definitions(hl2mp_lowlevel_fixes PRIVATE
	META_NO_HL2SDK
	SOURCE_ENGINE=7
	SE_EPISODEONE=1
	SE_DARKMESSIAH=2
	SE_ORANGEBOX=3
	SE_BLOODYGOODTIME=4
	SE_EYE=5
	SE_CSS=6
	SE_HL2DM=7
	SE_DODS=8
	SE_SDK2013=9
	SE_BMS=10
	SE_TF2=11
	SE_LEFT4DEAD=12
	SE_CONTAGION=14
	SE_LEFT4DEAD2=15
	SE_ALIENSWARM=16
	SE_PORTAL2=17
	SE_BLADE=18
	SE_INSURGENCY=19
	SE_DOI=20
	SE_CSGO=21
	SE_DOTA=22
	HL2_DLL
	HL2MP
	GAME_DLL
	NO_HOOK_MALLOC
	NO_MALLOC_OVERRIDE
)

target_link_libraries(hl2mp_lowlevel_fixes PRIVATE funchook-static capstone)
set_target_properties(hl2mp_lowlevel_fixes PROPERTIES
	PREFIX ""
	OUTPUT_NAME "hl2mp_lowlevel_fixes"
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN YES
)

if(MSVC)
	set_property(TARGET hl2mp_lowlevel_fixes PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	target_compile_definitions(hl2mp_lowlevel_fixes PRIVATE COMPILER_MSVC COMPILER_MSVC32 WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
	target_compile_options(hl2mp_lowlevel_fixes PRIVATE /W4 /GR- /EHs-c- /Zc:__cplusplus)
	target_link_options(hl2mp_lowlevel_fixes PRIVATE /INCREMENTAL:NO)
else()
	target_compile_definitions(hl2mp_lowlevel_fixes PRIVATE
		COMPILER_GCC GNUC POSIX LINUX _LINUX PLATFORM_32BITS
	)
	target_compile_options(hl2mp_lowlevel_fixes PRIVATE
		-m32 -fPIC -fno-exceptions -fno-rtti -fno-strict-aliasing -fno-sized-deallocation
		-fvisibility=hidden -fvisibility-inlines-hidden -msse -Wall -Wextra
		-Wno-unused-parameter -Wno-non-virtual-dtor -Wno-overloaded-virtual
		-Wno-unknown-pragmas -Wno-dangling-else -Wno-deprecated-copy-with-user-provided-copy
		-Wno-unused-variable -Wno-macro-redefined
	)
	target_link_options(hl2mp_lowlevel_fixes PRIVATE
		-m32 -fuse-ld=lld -nostdlib -Wl,--unresolved-symbols=ignore-all
		-Wl,--exclude-libs,ALL
	)
	set_target_properties(hl2mp_lowlevel_fixes PROPERTIES SUFFIX ".so")
endif()

install(TARGETS hl2mp_lowlevel_fixes
	RUNTIME DESTINATION addons/hl2mp_lowlevel_fixes/bin
	LIBRARY DESTINATION addons/hl2mp_lowlevel_fixes/bin
)
install(FILES package/addons/hl2mp_lowlevel_fixes.vdf DESTINATION addons)
