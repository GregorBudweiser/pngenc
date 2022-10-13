add_executable(timing_pngenc "timing_pngenc.c" "../../tests/common.c")
target_link_libraries(timing_pngenc pngenc)

find_package(PNG)

if(${PNG_FOUND})  
  if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN23")
  endif()

  include_directories(${PNG_INCLUDE_DIRS})
  add_executable(timing_libpng "timing_libpng.c" "../../tests/common.c")
  target_link_libraries(timing_libpng ${PNG_LIBRARY})
endif()
