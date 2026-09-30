
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

add_executable(demo WIN32
    main.c misc.h
)

target_include_directories(demo PRIVATE ../include)
target_link_libraries(demo featherui)
