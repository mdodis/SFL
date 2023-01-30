add_executable(sfl_bmp_test
    "./sfl_bmp.test.c")

if (UNIX)
    target_link_libraries(sfl_bmp_test
    "m")
endif()

set_property(TARGET sfl_bmp_test PROPERTY C_STANDARD 99)
