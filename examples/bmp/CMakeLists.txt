add_executable(bmp_probe
    "./bmp_probe.c"
    )
target_compile_definitions(bmp_probe PUBLIC "_CRT_SECURE_NO_WARNINGS")

add_executable(bmp_convert
    "./bmp_convert.c"
    )

target_compile_definitions(bmp_convert PUBLIC "_CRT_SECURE_NO_WARNINGS")