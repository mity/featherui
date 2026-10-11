
configure_file(project.h.in "${PROJECT_BINARY_DIR}/include/project.h" @ONLY)

set(SOURCES
    # Generated header:
    ${PROJECT_BINARY_DIR}/include/project.h

    # *.c:          # *.h:                  # public *.h:
    # =========================================================================
    appwindow.c     appwindow.h             ../include/feather/appwindow.h
    gfx.c           gfx.h gfx_internal.h
                    hstring_ref.h
    mainloop.c      mainloop.h
    misc.c          misc.h
    misc_alloc.c
    monitor.c       monitor.h
    system.c        system.h
    text.c          text.h
    uimetrics.c     uimetrics.h
    uitheme.c       uitheme.h
    version.c                               ../include/feather/version.h
    xtrace.c        xtrace.h
                                            ../include/feather.h
)
set(DLL_SOURCES
    ${SOURCES}
    _exports.def
)
set(STATIC_SOURCES
    ${SOURCES}
)

set(LIBS
    c_d2d_dwrite ex                         # ../external
    dwrite dxguid dxgi d2d1 d3d11 shcore    # system/toolchain
)

set(PUBLIC_INCDIRS
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
set(PRIVATE_INCDIRS
    ${CMAKE_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

add_definitions(-DFEATHER_BUILD)
add_definitions(-DWIN32_LEAN_AND_MEAN)

# Enable C interfaces for COM stuff we need. */
add_definitions(-DCINTERFACE)
add_definitions(-DD2D_USE_C_DEFINITIONS)
add_definitions(-DCOBJMACROS)


if(FEATHER_BUILD_DLL)
    add_library(feather SHARED ${DLL_SOURCES})
    target_compile_definitions(feather PRIVATE
            -DMALLOC_FUNC=fe_malloc
            -DREALLOC_FUNC=fe_realloc
            -DFREE_FUNC=fe_free
    )
    target_compile_definitions(feather PRIVATE -DFEATHER_BUILD_DLL)
    target_link_libraries(feather PRIVATE ${LIBS})
    target_include_directories(feather PUBLIC ${PUBLIC_INCDIRS} PRIVATE ${PRIVATE_INCDIRS})
    add_library(featherui ALIAS feather)
endif()

if(FEATHER_BUILD_STATIC)
    add_library(feather_static STATIC ${STATIC_SOURCES})
    target_compile_definitions(feather_static PRIVATE
            -DMALLOC_FUNC=fe_malloc
            -DREALLOC_FUNC=fe_realloc
            -DFREE_FUNC=fe_free
    )
    target_compile_definitions(feather_static PRIVATE -DFEATHER_BUILD_STATIC)
    target_link_libraries(feather_static PUBLIC ${LIBS})
    target_include_directories(feather_static PUBLIC ${PUBLIC_INCDIRS} PRIVATE ${PRIVATE_INCDIRS})
    add_library(featherui_static ALIAS feather_static)
endif()
