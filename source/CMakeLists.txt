# pngenc sources
add_subdirectory(pngenc)

# optional development tools (requires c++)
if(BUILD_TOOLS)
    message(STATUS "OPTIONAL DEV TOOLS ENABLED (REQUIRES C++)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    if(WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN23")
    endif()
    add_subdirectory(tools)
endif()

