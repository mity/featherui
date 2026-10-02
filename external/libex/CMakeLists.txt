
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")


set(SOURCES
    cmdline.c       cmdline.h
                    defs.h
                    list.h
)

# The main libex library:
add_library(ex STATIC ${SOURCES})
target_include_directories(ex PUBLIC src)
