add_executable(watchdir
    "./watchdir.c"
    )

if (WIN32)
    target_link_libraries(watchdir
        "shell32.lib"
        "Shlwapi.lib"
        "Pathcch.lib"
        )
endif()

set(DEBUG_FLAGS "")

message("Debug flags = ${DEBUG_FLAGS}")

target_compile_options(watchdir
    PUBLIC
    $<$<CONFIG:Debug>:${DEBUG_FLAGS}>)