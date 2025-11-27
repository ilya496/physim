#include <Entrypoint.h>

#include "EditorLayer.h"

class Editor : public Application
{
public:
    Editor() : Application()
    {
        m_LayerStack.PushLayer(std::make_unique<EditorLayer>(m_Window.get()));
    }

    virtual void Shutdown() override
    {
        Application::Shutdown();
    }

    ~Editor() {}
};

Application* CreateApplication()
{
    return new Editor();
}