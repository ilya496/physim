#pragma once

#include <string>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include <glad/glad.h>
#include "Buffer.h"
#include "VertexArray.h"

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

enum class TextureType
{
    DIFFUSE, SPECULAR, NORMAL
};

struct Texture {
    unsigned int id;
    TextureType type;
    std::string path;
};


class Geometry
{
public:
    Geometry(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

public:
    void Bind() const;
    void Unbind() const;

    uint32_t GetIndexCount() const;
private:
    std::shared_ptr<VertexArray> m_VertexArray;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(const Shader& shader) const;

private:
    unsigned int VBO, EBO;
    void setupMesh();
};

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    Model(const std::string& path);
    void Draw(const Shader& shader) const;

private:
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
    unsigned int TextureFromFile(const char* path, const std::string& directory);
};
