#include "Shader.h"
#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
{
    std::string vertexCode = LoadShaderFile(vertexPath);
    std::string fragmentCode = LoadShaderFile(fragmentPath);
    std::string geometryCode;

    const char* geomSrc = nullptr;
    if (geometryPath != nullptr)
    {
        geometryCode = LoadShaderFile(geometryPath);
        if (!geometryCode.empty())
            geomSrc = geometryCode.c_str();
    }

    m_RendererID = CreateProgram(vertexCode.c_str(), fragmentCode.c_str(), geomSrc);
}

Shader::~Shader()
{
    glDeleteProgram(m_RendererID);
}

void Shader::Bind() const
{
    glUseProgram(m_RendererID);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

std::string Shader::LoadShaderFile(const char* path)
{
    std::ifstream file(path);
    if (!file)
    {
        std::cerr << "ERROR::SHADER::Failed to open file: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

uint32_t Shader::CompileShader(uint32_t type, const char* source)
{
    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    std::string typeStr =
        (type == GL_VERTEX_SHADER) ? "VERTEX" :
        (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" :
        (type == GL_GEOMETRY_SHADER) ? "GEOMETRY" : "UNKNOWN";

    CheckCompileErrors(shader, typeStr);
    return shader;
}

uint32_t Shader::CreateProgram(const char* vertexSrc, const char* fragmentSrc, const char* geometrySrc)
{
    uint32_t program = glCreateProgram();

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    uint32_t gs = 0;
    if (geometrySrc)
    {
        gs = CompileShader(GL_GEOMETRY_SHADER, geometrySrc);
        glAttachShader(program, gs);
    }

    glLinkProgram(program);
    CheckCompileErrors(program, "PROGRAM");

    // Clean up
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (gs)
        glDeleteShader(gs);

    return program;
}

int Shader::GetUniformLocation(const std::string& name) const
{
    int location = glGetUniformLocation(m_RendererID, name.c_str());
    if (location == -1)
        std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
    return location;
}

void Shader::CheckCompileErrors(uint32_t object, const std::string& type) const
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                << infoLog << "\n----------------------------------------" << std::endl;
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n"
                << infoLog << "\n----------------------------------------" << std::endl;
        }
    }
}

void Shader::Set3f(const std::string& name, float v0, float v1, float v2) const
{
    glUniform3f(GetUniformLocation(name), v0, v1, v2);
}

void Shader::Set4f(const std::string& name, float v0, float v1, float v2, float v3) const
{
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::Set1i(const std::string& name, int value) const
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::Set1f(const std::string& name, float value) const
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec3f(const std::string& name, const glm::vec3& vec) const
{
    glUniform3fv(GetUniformLocation(name), 1, &vec[0]);
}

void Shader::SetVec4f(const std::string& name, const glm::vec4& vec) const
{
    glUniform4fv(GetUniformLocation(name), 1, &vec[0]);
}

void Shader::SetMat3f(const std::string& name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::SetMat4f(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}