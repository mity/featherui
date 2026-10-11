
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

add_executable(demo WIN32
    main.c
    demo.rc
)

target_include_directories(demo PRIVATE ../include ${CMAKE_SOURCE_DIR}/external)
target_link_libraries(demo ex featherui)
