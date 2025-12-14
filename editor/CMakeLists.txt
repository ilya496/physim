add_executable(Physim
    main.cpp
    EditorLayer.cpp
    EditorSettings.cpp
)

target_link_libraries(Physim PRIVATE engine)