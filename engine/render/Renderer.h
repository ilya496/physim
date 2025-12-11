#pragma once

#include "Model.h"
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Forward-declare engine GPU resource handles. Replace with your actual types.
struct MeshHandle { uint64_t id = 0; };      // e.g. index into AssetManager
struct MaterialHandle { uint64_t id = 0; };  // e.g. index into AssetManager

// Per-object render information (CPU-side, lightweight)
struct RenderObject
{
    glm::mat4 Model = glm::mat4(1.0f);
    MeshHandle Mesh;
    MaterialHandle Material;
    uint32_t LayerMask = 0xFFFFFFFF;
};

// Camera / frame globals used by renderer
struct CameraData
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::vec3 ViewPos;
};

// Frame snapshot passed to the renderer
struct RenderState
{
    CameraData Camera;
    std::vector<RenderObject> Opaque;
    std::vector<RenderObject> Transparent;

    // Optional: UI draw commands or other debug lists.
    // Add other small data you need the render thread to know.
};

class RenderCommandQueue
{
public:
    using Command = std::function<void()>;

    // Push a command from the main thread. Thread-safe.
    static void Push(Command cmd)
    {
        {
            std::lock_guard lock(s_Mutex);
            s_Queue.push(std::move(cmd));
        }
        s_CV.notify_one();
    }


    // Pop a command on the render thread. Blocks until a command is available or shutdown is requested.
    // Returns empty optional if shutdown and no command left.
    static std::optional<Command> PopBlocking()
    {
        std::unique_lock lock(s_Mutex);
        s_CV.wait(lock, [] { return s_ShutdownRequested || !s_Queue.empty(); });

        if (s_ShutdownRequested && s_Queue.empty())
            return std::nullopt;

        auto cmd = std::move(s_Queue.front());
        s_Queue.pop();
        return cmd;
    }


    // Try pop without blocking; returns nullopt when empty.
    static std::optional<Command> TryPop()
    {
        std::lock_guard lock(s_Mutex);
        if (s_Queue.empty()) return std::nullopt;
        auto cmd = std::move(s_Queue.front());
        s_Queue.pop();
        return cmd;
    }


    // Wake render thread waiting on PopBlocking (e.g. during shutdown).
    static void NotifyAll()
    {
        s_CV.notify_all();
    }

    // Request shutdown so PopBlocking can return empty.
    static void RequestShutdown()
    {
        {
            std::lock_guard lock(s_Mutex);
            s_ShutdownRequested = true;
        }
        s_CV.notify_all();
    }


    // Clear shutdown request (for re-use tests). Not necessary normally.
    static void ClearShutdownRequest()
    {
        std::lock_guard lock(s_Mutex);
        s_ShutdownRequested = false;
    }

private:
    static inline std::mutex s_Mutex;
    static inline std::condition_variable s_CV;
    static inline std::queue<Command> s_Queue;
    static inline bool s_ShutdownRequested = false;
};

class Renderer
{
public:
    // Called once on the render thread to initialize GL, ImGui, etc.
    // nativeWindow is whatever your Window::GetNativeWindow() returns (GLFWwindow*)
    static void InitForRenderThread(void* nativeWindow)
    {
        // Make the context current on this thread (nativeWindow is GLFWwindow*)
        glfwMakeContextCurrent(static_cast<GLFWwindow*>(nativeWindow));

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cerr << "Renderer: Failed to initialize GLAD on render thread\n";
            return;
        }

        // Basic GL state you can change
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Setup ImGui context on render thread
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        // Add or remove flags as needed
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(nativeWindow), true);
        ImGui_ImplOpenGL3_Init("#version 460");

        s_Initialized = true;
    }

    // Called to request a clean shutdown of the renderer and render loop.
    static void Shutdown()
    {
        // Stop the render thread loop
        StopRenderThread();

        // Shutdown ImGui and detach GL context (only if we initialized)
        if (s_Initialized)
        {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            s_Initialized = false;
        }
    }

    // Enqueue a GPU command (create buffers, upload textures, destroy, etc.)
    // Must be called from main thread (or any thread). Commands execute on render thread.
    static void EnqueueCommand(RenderCommandQueue::Command cmd)
    {
        RenderCommandQueue::Push(std::move(cmd));
    }

    // Submit a complete frame snapshot to be consumed by the render thread.
    // Ownership of the snapshot can be transferred; takes unique_ptr for explicitness.
    // Call from main thread after building the RenderState for the frame.
    static void SubmitRenderState(std::unique_ptr<RenderState> state)
    {
        {
            std::lock_guard lock(s_StateMutex);
            // Replace any previously queued state (we keep only last snapshot)
            s_CurrentState = std::move(state);
        }

        // notify render thread
        s_NewFrameAvailable = true;
        s_FrameCV.notify_one();
    }

    // Start render loop on a new thread. The function will spawn the render thread and return immediately.
    // windowNative is your native window pointer (GLFWwindow*). Renderer will make context current in the render thread.
    static void StartRenderThread(void* windowNative)
    {
        if (s_ShouldRun)
            return; // already running

        s_ShouldRun = true;
        RenderCommandQueue::ClearShutdownRequest();

        s_RenderThread = std::thread([windowNative]() {
            Renderer::RenderThreadMain(windowNative);
            });
    }

    // Call from application destructor or shutdown path to stop the render thread.
    static void StopRenderThread()
    {
        if (!s_ShouldRun)
            return;

        s_ShouldRun = false;
        // wake up render thread
        s_FrameCV.notify_one();
        RenderCommandQueue::RequestShutdown();
        RenderCommandQueue::NotifyAll();

        if (s_RenderThread.joinable())
            s_RenderThread.join();

        // Detach GL context from this thread
        // (If your Window expects context detached on stop, do it here or in Window)
        glfwMakeContextCurrent(nullptr);
    }

    // NOTE: You may also add helper API like CreateMeshAsync(...), but the basic enqueue+command pattern is generic.

private:
    // Private implementation details below.
    static void RenderThreadMain(void* nativeWindow)
    {
        // Initialize GL/ImGui on this thread
        InitForRenderThread(nativeWindow);

        // Make sure window has context current; user must have detached context on main thread before calling StartRenderThread
        // Main render loop
        while (s_ShouldRun)
        {
            // 1) Execute any queued GPU/resource commands first (upload mesh, textures etc.)
            //    We will drain the RenderCommandQueue (blocking-pop until empty)
            while (true)
            {
                auto optCmd = RenderCommandQueue::TryPop();
                if (!optCmd.has_value()) break;
                try {
                    optCmd.value()();
                }
                catch (const std::exception& ex) {
                    std::cerr << "Renderer: Render command threw: " << ex.what() << "\n";
                }
                catch (...) {
                    std::cerr << "Renderer: Render command threw unknown exception\n";
                }
            }

            // 2) Wait for next frame state or a small timeout to keep UI responsive
            {
                std::unique_lock lock(s_FrameMutex);
                s_FrameCV.wait_for(lock, std::chrono::milliseconds(2), [] { return s_NewFrameAvailable.load() == true || !s_ShouldRun.load(); });
            }

            if (!s_ShouldRun) break;

            // 3) Consume snapshot
            std::unique_ptr<RenderState> stateCopy;
            {
                std::lock_guard lock(s_StateMutex);
                if (s_CurrentState)
                {
                    // move ownership for this frame; keep s_CurrentState empty to indicate consumed
                    stateCopy = std::move(s_CurrentState);
                    s_CurrentState.reset();
                    s_NewFrameAvailable = false;
                }
                else
                {
                    // no frame available; continue to next loop iteration (allows processing commands)
                    continue;
                }
            }

            // 4) Begin ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // 5) Draw the scene based on stateCopy
            //    Call DrawFrame which will bind materials, VAOs, issue draw calls
            //    You should implement DrawFrame to use your Mesh/Material wrappers.
            //    For now we implement a minimal loop with placeholders.
            if (stateCopy)
            {
                // Clear screen
                glViewport(0, 0, /* width */ 1920, /* height */ 1080); // you should query framebuffer size
                glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // TODO: set camera UBOs / uniforms from stateCopy->Camera
                // Example: Shader::BindGlobalCamera(stateCopy->Camera);

                // Draw opaque
                for (auto& obj : stateCopy->Opaque)
                {
                    // TODO: lookup mesh/material by handle and draw
                    // Example:
                    // Mesh* mesh = AssetManager::GetMesh(obj.Mesh.id);
                    // Material* mat = AssetManager::GetMaterial(obj.Material.id);
                    // mat->Bind();
                    // mat->SetUniform("u_Model", obj.Model);
                    // mesh->Draw();
                }

                // Draw transparent (sorted) - sorting omitted for brevity
                for (auto& obj : stateCopy->Transparent)
                {
                    // same as above
                }
            }

            // 6) Render ImGui (draw data already prepared by layers that ran OnRender on main thread or here)
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // 7) Swap buffers - using GLFW
            glfwSwapBuffers(static_cast<GLFWwindow*>(nativeWindow));
        }

        // Flush remaining commands
        while (true)
        {
            auto optCmd = RenderCommandQueue::TryPop();
            if (!optCmd.has_value()) break;
            try { optCmd.value()(); }
            catch (...) {}
        }

        // IMGUI shutdown will be done by Renderer::Shutdown() called on the main thread during application teardown
    }
    static void ExecutePendingCommands(); // drains command queue
    static void DrawFrame();               // draws the current RenderState

    // Current frame snapshot (consumed by render thread). Protected by mutex.
    static inline std::unique_ptr<RenderState> s_CurrentState;
    static inline std::mutex s_StateMutex;

    // Atomic flag used to tell render thread when a new frame is available.
    static inline std::atomic<bool> s_NewFrameAvailable{ false };
    static inline std::condition_variable s_FrameCV;
    static inline std::mutex s_FrameMutex;

    // Render thread
    static inline std::thread s_RenderThread;
    static inline std::atomic<bool> s_ShouldRun{ false };
    static inline std::atomic<bool> s_Initialized{ false };
};