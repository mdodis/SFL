add_executable(watchdir
    "./watchdir.c"
    )

target_link_libraries(watchdir
    "shell32.lib"
    "Shlwapi.lib"
    "Pathcch.lib"
    )