
cmake_minimum_required(VERSION 3.16)
project(FeatherUI
    VERSION "0.1.0"
    HOMEPAGE_URL https://github.com/mity/featherui
    LANGUAGES C
)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})

    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()


add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-D_WIN32_IE=0x0a00 -D_WIN32_WINNT=0xa000 -DWINVER=_WIN32_WINNT)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}")
set(CMAKE_SHARED_LIBRARY_PREFIX "")

OPTION(FEATHER_BUILD_DLL "Enable/disable build of FeatherUI as a shared lib (feather.dll)" ON)
OPTION(FEATHER_BUILD_STATIC "Enable/disable build of FeatherUI as a static lib (feather_static.lib or libfeather_static.a)" OFF)

if(CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
    add_definitions(-DDEBUG)
endif()

if("${CMAKE_C_COMPILER_ID}" MATCHES "GNU|Clang")
    # Enable warnings:
    add_compile_options(-Wall -Wextra -Wshadow -Wdeclaration-after-statement)

    # Use Unicode (so we may avoid using -W suffixed symbols):
    add_compile_options(-municode)
    add_link_options(-municode)

    # Use faster but less precise float math; it suffices for our purposes:
    add_compile_options(-ffast-math)

    # By default, mingw-w64 uses its own swprintf() implementation, for the
    # sake of standard compliance. We don't need it, so lets enforce the one
    # from System runtime:
    add_compile_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

    # We don't want to depend on LIBGCC.DLL:
    add_link_options(-static-libgcc)

    # Ensure our exported symbols don't have undesired symbol decorations:
    add_link_options(-Wl,--enable-stdcall-fixup -Wl,--kill-at)

    # In non-debug builds, enable DEP (data execution prevention) and
    # ASLR (address space layout randomization).
    # See https://lists.gnu.org/archive/html/bug-binutils/2015-09/msg00204.html
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_link_options(-Wl,--nxcompat)
        add_link_options(-Wl,--disable-auto-image-base,--dynamicbase)
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            add_link_options(-Wl,--high-entropy-va)
            add_link_options(-Wl,--image-base,0x180000000)
        endif()
    endif()

    # Use LTO in non-debug builds:
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-flto -fno-fat-lto-objects)
        add_link_options(-flto)
    endif()


    # Use -O3 rather than -O2 in Release builds.
    #string(REGEX REPLACE "(^| )-O[0-9a-z]+" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    #string(REGEX REPLACE "(^| )-O[0-9a-z]+" "" CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE}")
    #set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${OPTIMIZE_LEVEL}")
    #set(CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE} ${OPTIMIZE_LEVEL}")
endif()

if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    # Disable warnings about the so-called unsecured functions:
    add_compile_definitions(/D_CRT_SECURE_NO_WARNINGS)

    # Use faster but less precise float math; it suffices for our purposes:
    add_compile_options(/fp:fast)

    # We provide our own manifest, so tell the linker to not generate it:
    add_link_options(/MANIFEST:NO)
endif()

add_subdirectory(src)
add_subdirectory(demo)
add_subdirectory(external)

