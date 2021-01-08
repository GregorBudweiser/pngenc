# set flags to activate dllexport on win
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DPNGENC_EXPORT=1")
endif()

# define pngenc library
file(GLOB pngenc_sources "*.c" "*.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/iacaMarks.h")
add_library(pngenc SHARED ${pngenc_sources})
add_executable(example "main.c" "pngenc.h")
target_link_libraries(example pngenc)

# make include dir during cmake configure
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/pngenc)

# copy header during build
add_custom_command(
        TARGET pngenc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/source/pngenc/pngenc.h
                ${CMAKE_BINARY_DIR}/include/pngenc/pngenc.h)
