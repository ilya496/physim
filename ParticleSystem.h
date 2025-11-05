#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include "imgui.h"

#include "Camera.h"
#include "Shader.h"
#include "stb_image.h"
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
};

class ParticleSystem {
public:
    virtual ~ParticleSystem() {}
    virtual void Update(float dt) = 0;
    virtual void Render(Camera& camera, const glm::mat4& projection) = 0;
    virtual void ResetParticle(Particle& p) = 0;
    virtual void ImGuiControls() = 0;

protected:
    std::vector<Particle> particles;
    unsigned int VAO = 0, VBO = 0;
    glm::vec3 emitterPos = glm::vec3(0.0f);
    float emissionRate = 100.0f;
    float particleLife = 3.0f;
    float particleSpeed = 2.0f;
    float gravity = -9.8f;
    unsigned int maxParticles = 1000;
    float particleSize = 0.2f;
};

class BillboardParticleSystem : public ParticleSystem {
public:
    BillboardParticleSystem() {
        particles.resize(maxParticles);

        float quadVertices[] = {
            // positions   // texcoords
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
        };
        unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        shader = new Shader("../shaders/particle_billboard.vert", "../shaders/particle_billboard.frag");

        // Load texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        stbi_set_flip_vertically_on_load(true);
        int w, h, ch;
        unsigned char* data = stbi_load("../smoke.png", &w, &h, &ch, 4);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        stbi_image_free(data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    ~BillboardParticleSystem() override {
        delete shader;
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    void Update(float dt) override {
        int newParticles = static_cast<int>(emissionRate * dt);
        for (int i = 0; i < maxParticles && newParticles > 0; i++) {
            if (particles[i].life <= 0.0f) {
                ResetParticle(particles[i]);
                newParticles--;
            }
        }

        for (auto& p : particles) {
            if (p.life > 0.0f) {
                p.life -= dt;
                p.velocity.y += gravity * dt * 0.2f;
                p.position += p.velocity * dt;
            }
        }
    }

    void Render(Camera& camera, const glm::mat4& projection) override {
        glm::mat4 view = camera.GetViewMatrix();
        shader->Bind();
        shader->SetMat4f("u_View", view);
        shader->SetMat4f("u_Projection", projection);
        shader->Set1i("u_Texture", 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        std::vector<const Particle*> aliveParticles;
        aliveParticles.reserve(particles.size());
        for (const auto& p : particles)
            if (p.life > 0.0f)
                aliveParticles.push_back(&p);

        std::sort(aliveParticles.begin(), aliveParticles.end(), [&](const Particle* a, const Particle* b) {
            float da = glm::length2(camera.Position - a->position);
            float db = glm::length2(camera.Position - b->position);
            return da > db;
            });

        glBindVertexArray(VAO);
        for (const auto* p : aliveParticles) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p->position);
            model = glm::scale(model, glm::vec3(particleSize));

            // extract camera right/up vectors from view
            glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
            glm::vec3 up = glm::vec3(view[0][1], view[1][1], view[2][1]);
            glm::mat4 billboard = glm::mat4(
                glm::vec4(right, 0.0f),
                glm::vec4(up, 0.0f),
                glm::vec4(glm::cross(right, up), 0.0f),
                glm::vec4(0, 0, 0, 1)
            );

            model *= billboard;
            shader->SetMat4f("u_Model", model);
            shader->Set1f("u_Alpha", p->life / particleLife);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }

    void ResetParticle(Particle& p) override {
        p.position = emitterPos;
        p.velocity = glm::vec3(
            ((rand() % 100) / 100.0f - 0.5f) * particleSpeed,
            (rand() % 100) / 100.0f * particleSpeed,
            ((rand() % 100) / 100.0f - 0.5f) * particleSpeed
        );
        p.life = particleLife;
    }

    void ImGuiControls() override {
        ImGui::Text("Billboard Particle System");
        ImGui::SliderFloat3("Emitter Position", glm::value_ptr(emitterPos), -5.0f, 5.0f);
        ImGui::SliderFloat("Emission Rate", &emissionRate, 0.0f, 1000.0f);
        ImGui::SliderFloat("Speed", &particleSpeed, 0.1f, 10.0f);
        ImGui::SliderFloat("Lifetime", &particleLife, 0.1f, 10.0f);
        ImGui::SliderFloat("Gravity", &gravity, -20.0f, 0.0f);
        ImGui::SliderFloat("Particle Size", &particleSize, 0.01f, 1.0f, "%.3f");
    }

private:
    Shader* shader;
    unsigned int EBO;
    unsigned int texture;
};
