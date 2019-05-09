cmake_minimum_required(VERSION 3.0)

# Project properties
project(pngenc)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS                "${CMAKE_C_FLAGS}                -march=native -std=gnu99 -DPNGENC_EXPORT=1 -Wall -Wcast-align -Werror=return-type -fopenmp")
    set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}        -mtune=native -O3")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -mtune=native -O3")

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -lgomp")

    # Enable arm neon on RPi 2 (see arm_neon.h)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpu=neon")
    endif()
endif()

# Add sources
add_subdirectory(source)
add_subdirectory(tests)
