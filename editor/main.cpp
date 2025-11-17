#include <Entrypoint.h>

class Editor : public Application
{
public:
    Editor() : Application() {}

    virtual void Shutdown() override {
        Application::Shutdown();
    }

    ~Editor() {}
};

Application* CreateApplication()
{
    return new Editor();
}