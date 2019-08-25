if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DPNGENC_EXPORT=1")
endif()

file(GLOB pngenc_sources "*.c" "*.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/java.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/java.c")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/iacaMarks.h")

# set flags to activate dllexport on win
add_library(pngenc SHARED ${pngenc_sources})
add_executable(example "main.c" "pngenc.h")
target_link_libraries(example pngenc)

#make include dir during cmake configure
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/pngenc)

#copy header during build
add_custom_command(
        TARGET pngenc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/source/pngenc/pngenc.h
                ${CMAKE_BINARY_DIR}/include/pngenc/pngenc.h)

# build native lib for java
if(BUILD_JAVA)
    message(STATUS "OPTIONAL JAVA BINDINGS ENABLED (REQUIRES JDK8)")
    message(STATUS "-- You may need to adjust the jdk path in the build script")
    INCLUDE_DIRECTORIES("C:/Program Files/Java/jdk-10.0.1/include")
    INCLUDE_DIRECTORIES("C:/Program Files/Java/jdk-10.0.1/include/win32")
    add_library(pngenc4j SHARED ${pngenc_sources} "java.h" "java.c")
endif()
