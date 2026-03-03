add_executable(Physim
    main.cpp
    EditorLayer.cpp
    EditorSettings.cpp
    LauncherPanel.cpp
    InspectorPanel.cpp
    AssetPanel.cpp
    SceneHierarchyPanel.cpp
)

target_compile_definitions(Physim PRIVATE
    ICON_DIR="${CMAKE_SOURCE_DIR}/editor/icons/"
)

target_link_libraries(Physim PRIVATE engine)