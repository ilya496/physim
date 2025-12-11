#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
    ~Shader();

    void Bind() const;
    void Unbind() const;

    void Set1i(const std::string& name, int value) const;
    void Set1f(const std::string& name, float value) const;
    void Set3f(const std::string& name, float v0, float v1, float v2) const;
    void Set4f(const std::string& name, float v0, float v1, float v2, float v3) const;
    void SetVec3f(const std::string& name, const glm::vec3& vec) const;
    void SetVec4f(const std::string& name, const glm::vec4& vec) const;
    void SetMat3f(const std::string& name, const glm::mat3& mat) const;
    void SetMat4f(const std::string& name, const glm::mat4& mat) const;

private:
    uint32_t m_RendererID = 0;

    std::string LoadShaderFile(const char* path);
    uint32_t CompileShader(uint32_t type, const char* source);
    void CheckCompileErrors(uint32_t object, const std::string& type) const;
    int GetUniformLocation(const std::string& name) const;

    uint32_t CreateProgram(const char* vertexSrc, const char* fragmentSrc, const char* geometrySrc = nullptr);
};
