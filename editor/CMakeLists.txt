add_executable(Physim
    main.cpp
    EditorLayer.cpp
    EditorSettings.cpp
    LauncherPanel.cpp
)

target_link_libraries(Physim PRIVATE engine)