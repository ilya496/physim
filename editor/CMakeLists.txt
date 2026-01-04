add_executable(Physim
    main.cpp
    EditorLayer.cpp
    EditorSettings.cpp
    LauncherPanel.cpp
    InspectorPanel.cpp
    AssetPanel.cpp
    SceneHierarchyPanel.cpp
)

target_link_libraries(Physim PRIVATE engine)