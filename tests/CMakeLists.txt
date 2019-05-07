#testing stuff
enable_testing()

# Replace with what you want
set(test_prefix "pngenc_test")

# Test creation macro
# Generates a test scaffold for the file if none exists.
# Registers this test in CTEST
macro(create_test test_name)
    set(test_file_name "${test_name}.c")
    set(test_file_path "${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.c")
    
    # Create the test scaffold if it does not exist, so after running cmake you have your test.
    if(NOT EXISTS ${test_file_path})
        set(test_scaffold "#include \"common.h\"\n\nint ${test_name}(int argc, char* argv[]) {\n    // TODO: Make the test work\n    return -1\;\n}\n\n")
        file(WRITE ${test_file_path} ${test_scaffold})
    endif()
    
    # Registering with cmake
    set(gen_test_sources ${gen_test_sources} ${test_file_name})
    add_test(NAME "${test_prefix}.${test_name}" COMMAND ${test_prefix} ${test_name})
endmacro(create_test)

# Use a single line to create a new test :)
# =========================================

# Unit tests
create_test(unit_adler)
create_test(unit_huffman)
create_test(unit_crc)
create_test(unit_matcher)
create_test(unit_deflate)

# Integration tests
create_test(integration_save_png)
create_test(integration_save_png16)
create_test(integration_pipeline)

# Performance tests
create_test(perf_huffman)
create_test(perf_adler)
create_test(perf_mt)

# Miscellaneous tests
create_test(misc_matcher)
create_test(misc_reverse)

# CTEST stuff
file(GLOB pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/*.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/*.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/main.c")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/java.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/java.c")
# remove dll interface sources and test them via dll
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/pngenc.h")
list(REMOVE_ITEM pngenc_sources "${CMAKE_CURRENT_SOURCE_DIR}/../source/pngenc/pngenc.c")

#message(STATUS "test-src: ${pngenc_sources}")
include_directories("../source")
create_test_sourcelist(test_sources test_driver.c ${gen_test_sources})
add_executable(${test_prefix} ${test_sources} ${pngenc_sources} ${system_sources}
    "${CMAKE_CURRENT_SOURCE_DIR}/common.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common.c")
target_link_libraries(${test_prefix} pngenc)
