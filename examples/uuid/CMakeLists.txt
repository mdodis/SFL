add_executable(uuid_example "./uuid_example.cpp")

if (WIN32)
    target_link_libraries(uuid_example
        "shell32.lib"
        "Shlwapi.lib"
        "Pathcch.lib"
        )
endif()