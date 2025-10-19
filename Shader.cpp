#include "Shader.h"
#include "glad/glad.h"

#include <iostream>
#include <fstream>
#include <sstream>

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

void Shader::Compile(const char *vertexSource, const char *fragmentSource)
{
    m_RendererID = CreateShader(vertexSource, fragmentSource);
}

void Shader::Set4f(const std::string &name, float v0, float v1, float v2, float v3) const
{
    glUniform4f(GetUniformLocation(name), v0, v1, v2, v3);
}

void Shader::Set3f(const std::string &name, float v0, float v1, float v2) const
{
    glUniform3f(GetUniformLocation(name), v0, v1, v2);
}

void Shader::Set1i(const std::string &name, int value) const
{
    glUniform1i(GetUniformLocation(name), value);
}

void Shader::Set1f(const std::string &name, float value) const
{
    glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetVec3f(const std::string &name, const glm::vec3 &vec) const
{
    glUniform3fv(GetUniformLocation(name), 1, &vec[0]);
}

void Shader::SetMat4f(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

uint32_t Shader::CompileShader(uint32_t type, const char *source) const
{
    uint32_t id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);
    checkCompileErrors(id, ShaderTypeToString(type));
    return id;
}

uint32_t Shader::CreateShader(const char *vertexSource, const char *fragmentSource) const
{
    uint32_t program = glCreateProgram();

    uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glValidateProgram(program);
    checkCompileErrors(program, "PROGRAM");

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int Shader::GetUniformLocation(const std::string &name) const
{
    int location = glGetUniformLocation(m_RendererID, name.c_str());

    if (location == -1)
        std::cout << "Warning: uniform: '" << name << "' doesn't exist!" << std::endl;

    return location;
}

void Shader::checkCompileErrors(uint32_t object, std::string type) const
{
    int success;
    char infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(object, 1024, NULL, infoLog);
            std::cout << "| ERROR::SHADER: Compile-time error: Type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    }
    else
    {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(object, 1024, NULL, infoLog);
            std::cout << "| ERROR::Shader: Link-time error: Type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- "
                      << std::endl;
        }
    }
}

std::string Shader::ShaderTypeToString(uint32_t type) const
{
    switch (type)
    {
    case GL_VERTEX_SHADER:
        return "VERTEX";
    case GL_FRAGMENT_SHADER:
        return "FRAGMENT";
    default:
        return "UNKNOWN";
    }
}

Shader LoadShaderFromFile(const char *vShaderFile, const char *fShaderFile)
{
    std::string vertexSource;
    std::string fragmentSource;
    try
    {
        std::ifstream vertexShaderFile(vShaderFile);
        std::ifstream fragmentShaderFile(fShaderFile);
        std::stringstream vShaderStream, fShaderStream;

        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();

        vertexShaderFile.close();
        fragmentShaderFile.close();

        vertexSource = vShaderStream.str();
        fragmentSource = fShaderStream.str();
    }
    catch (std::exception e)
    {
        std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
    }
    const char *vShaderSource = vertexSource.c_str();
    const char *fShaderSource = fragmentSource.c_str();
    Shader shader;
    shader.Compile(vShaderSource, fShaderSource);
    return shader;
}