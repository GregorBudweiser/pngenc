find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIRS})

file(GLOB timing_sources "${CMAKE_CURRENT_SOURCE_DIR}/../system/*.cpp"
                         "${CMAKE_CURRENT_SOURCE_DIR}/../system/*.h")


add_executable(timing_pngenc "timing_pngenc.cpp" "../pngenc/pngenc.h" ${timing_sources})
target_link_libraries(timing_pngenc pngenc)

add_executable(timing_libpng "timing_libpng.cpp" ${timing_sources})
target_link_libraries(timing_libpng ${PNG_LIBRARY})


