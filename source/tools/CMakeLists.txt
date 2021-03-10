find_package(PNG REQUIRED)

include_directories(${PNG_INCLUDE_DIRS})

add_executable(timing_pngenc "timing_pngenc.c" "../../tests/common.c")
target_link_libraries(timing_pngenc pngenc)

add_executable(timing_libpng "timing_libpng.c" "../../tests/common.c")
target_link_libraries(timing_libpng ${PNG_LIBRARY})
