# pngenc sources
add_subdirectory(pngenc)

# optional development tools (requires libpng)
if(BUILD_TOOLS)
    message(STATUS "OPTIONAL DEV TOOLS ENABLED (REQUIRES libpng)")
    if(WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN23")
    endif()
    add_subdirectory(tools)
endif()

