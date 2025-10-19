#pragma once

#include <string>
#include "glm/glm.hpp"

class Shader
{
public:
    Shader() : m_RendererID(0) {}
    ~Shader();

public:
    void Bind() const;
    void Unbind() const;

    void Compile(const char *vertexSource, const char *fragmentSource);

    void Set1i(const std::string &name, int value) const;
    void Set1f(const std::string &name, float value) const;
    void Set4f(const std::string &name, float v0, float v1, float v2, float v3) const;
    void Set3f(const std::string &name, float v0, float v1, float v2) const;
    void SetVec3f(const std::string &name, const glm::vec3 &vec) const;
    void SetMat4f(const std::string &name, const glm::mat4 &matrix) const;

    inline uint32_t GetID() const { return m_RendererID; }

private:
    uint32_t CompileShader(uint32_t type, const char *source) const;
    uint32_t CreateShader(const char *vertexSource, const char *fragmentSource) const;
    int GetUniformLocation(const std::string &name) const;
    void checkCompileErrors(uint32_t object, std::string type) const;
    std::string ShaderTypeToString(uint32_t type) const;

private:
    uint32_t m_RendererID;
};

Shader LoadShaderFromFile(const char *vShaderFile, const char *fShaderFile);