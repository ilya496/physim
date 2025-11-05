#pragma once

#include <vector>
#include <glm/glm.hpp>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
};

class ParticleSystem {
public:
    virtual ~ParticleSystem() {}
    virtual void Update(float dt) = 0;
    virtual void Render(const glm::mat4& view, const glm::mat4& projection) = 0;
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