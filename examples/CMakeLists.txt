if (WIN32)
    add_executable(d3d_viewer WIN32
        "./d3d_viewer.cpp"
        )

    target_link_libraries(d3d_viewer
        "d3d11"
        "d3dcompiler"
        )
endif()

add_subdirectory("./fs_watch")
