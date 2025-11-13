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

enum class MeshPrimitive { CUBE, PLANE, UV_SPHERE, ICO_SPHERE, CYLINDER };

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

class Texture
{
public:
    Texture(const std::string& path, bool flipVertically = true);
    ~Texture();

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetID() const { return m_RendererID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    const std::string& GetPath() const { return m_Path; }

    static std::shared_ptr<Texture> Create(const std::string& path, bool flipVertically = true)
    {
        return std::make_shared<Texture>(path, flipVertically);
    }

private:
    uint32_t m_RendererID = 0;
    std::string m_Path;
    int m_Width = 0;
    int m_Height = 0;
    int m_Channels = 0;
};

class Material
{
public:
    virtual ~Material() = default;
    virtual void Apply(const Shader& shader) const = 0;
};

class PhongMaterial : public Material
{
public:
    void Apply(const Shader& shader) const override
    {
        shader.Bind();
        shader.SetVec3f("material.diffuseColor", DiffuseColor);
        shader.SetVec3f("material.specularColor", SpecularColor);
        shader.Set1f("material.shininess", Shininess);
        shader.Set1i("material.hasDiffuseMap", DiffuseMap ? 1 : 0);
        shader.Set1i("material.hasSpecularMap", SpecularMap ? 1 : 0);
        shader.Set1i("material.hasNormalMap", NormalMap ? 1 : 0);

        uint32_t textureSlot = 0;

        if (DiffuseMap) {
            DiffuseMap->Bind(textureSlot);
            shader.Set1i("material.diffuseMap", textureSlot++);
        }

        if (SpecularMap) {
            SpecularMap->Bind(textureSlot);
            shader.Set1i("material.specularMap", textureSlot++);
        }

        if (NormalMap) {
            NormalMap->Bind(textureSlot);
            shader.Set1i("material.normalMap", textureSlot++);
        }
    }
public:
    glm::vec3 DiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 SpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float Shininess = 32.0f;

    std::shared_ptr<Texture> DiffuseMap;
    std::shared_ptr<Texture> SpecularMap;
    std::shared_ptr<Texture> NormalMap;
};

inline BufferLayout CreateVertexBufferLayout()
{
    return {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float3, "a_Normal" },
        { ShaderDataType::Float2, "a_TexCoords" },
        { ShaderDataType::Float3, "a_Tangent" },
        { ShaderDataType::Float3, "a_Bitangent" }
    };
}

class Geometry
{
public:
    Geometry(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

public:
    void Bind() const;
    void Unbind() const;
    uint32_t GetIndexCount() const { return m_IndexBuffer->GetCount(); }
    static std::shared_ptr<Geometry> Generate(MeshPrimitive primitive);

private:
    static std::shared_ptr<Geometry> GenerateCube();
    static std::shared_ptr<Geometry> GeneratePlane();
    // static std::shared_ptr<Geometry> GenerateUVSphere();
    // static std::shared_ptr<Geometry> GenerateIcoSphere();
    // static std::shared_ptr<Geometry> GenerateCylinder();

    static void ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    std::shared_ptr<VertexArray> m_VertexArray;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

class Mesh {
public:
    Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material);

    void Draw(const Shader& shader) const;

    const std::shared_ptr<Geometry>& GetGeometry() const { return m_Geometry; }
    const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
private:
    std::shared_ptr<Geometry> m_Geometry;
    std::shared_ptr<Material> m_Material;
};

// class Model {
// public:
//     std::vector<Mesh> meshes;
//     std::string directory;

//     Model(const std::string& path);
//     void Draw(const Shader& shader) const;

// private:
//     void loadModel(const std::string& path);
//     void processNode(aiNode* node, const aiScene* scene);
//     Mesh processMesh(aiMesh* mesh, const aiScene* scene);
//     std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName);
//     uint32_t TextureFromFile(const std::string& path, const std::string& directory);
// };
