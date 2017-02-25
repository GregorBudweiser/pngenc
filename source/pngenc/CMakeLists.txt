set(pngenc_sources
    "adler32.h"
    "adler32.c"
    "callback.h"
    "callback.c"
    "crc32.h"
    "crc32.c"
    "huffman.h"
    "huffman.c"
    "matcher.h"
    "matcher.c"
    "image_descriptor.h"
    "image_descriptor.c"
    "node.h"
    "node.c"
    "node_custom.h"
    "node_data_gen.h"
    "node_data_gen.c"
    "node_deflate.h"
    "node_deflate.c"
    "node_idat.h"
    "node_idat.c"
    "png_pipeline.h"
    "png_pipeline.c"
    "pngenc.h"
    "pngenc.c"
    "utils.h"
    "utils.c"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DPNGENC_EXPORT=1 -Wall -Werror=return-type -mtune=native -funroll-loops")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DPNGENC_EXPORT=1")
endif()

# set flags to activate dllexport on win
add_library(pngenc SHARED ${pngenc_sources})
# add libm on gnu/linux
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_libraries(pngenc m)
endif()

add_executable(example "main.c" "pngenc.h")
target_link_libraries(example pngenc)

# build native lib for java
if(BUILD_JAVA)
    message(STATUS "OPTIONAL JAVA BINDINGS ENABLED (REQUIRES JDK8)")
    message(STATUS "-- You may need to adjust the jdk path in the build script")
    INCLUDE_DIRECTORIES("~/Downloads/jdk1.8.0_102/include/")
    INCLUDE_DIRECTORIES("~/Downloads/jdk1.8.0_102/include/linux")
    add_library(pngenc4j SHARED ${pngenc_sources} "java.h" "java.c")
endif()
