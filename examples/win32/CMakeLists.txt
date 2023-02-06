add_executable(wimv WIN32
    "./wimv.cpp"
    )

target_link_libraries(wimv
    "shell32.lib"
    "Shlwapi.lib"
    "Pathcch.lib"
    "Ole32.lib"
    "windowscodecs.lib"
    "shlwapi.lib"
    "d3d11"
    "d3dcompiler"
    )

target_compile_definitions(wimv
    PUBLIC
    _CRT_SECURE_NO_WARNINGS
    )