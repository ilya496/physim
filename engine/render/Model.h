#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <filesystem>
#include <memory>

#include "Shader.h"
#include "Buffer.h"
#include "VertexArray.h"
#include "MeshPrimitive.h"
#include "asset/Asset.h"
#include "project/Project.h"
#include "asset/AssetManager.h"
#include "physics/AABB.h"

class Texture;
class Mesh;
class Material;

class TextureAsset : public Asset
{
public:
    virtual AssetType GetType() const override { return AssetType::Texture; }

    std::shared_ptr<Texture> TextureData;
};

class MeshAsset : public Asset
{
public:
    virtual AssetType GetType() const override { return AssetType::Mesh; }

    std::shared_ptr<Mesh> MeshData;
};

class MaterialAsset : public Asset
{
public:
    virtual AssetType GetType() const override { return AssetType::Material; }

    std::shared_ptr<Material> MaterialData;
};

// struct ShaderSource
// {
//     std::filesystem::path Vertex;
//     std::filesystem::path Fragment;
//     std::filesystem::path Geometry;
// };

// class ShaderAsset : public Asset
// {
// public:
//     virtual AssetType GetType() const override { return AssetType::Shader; }

//     ShaderSource Sources;
//     std::shared_ptr<Shader> ShaderData;
// };

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Texture
{
public:
    explicit Texture(const std::filesystem::path& path, bool flipVertically = true);
    ~Texture();

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetRendererID() const { return m_RendererID; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint32_t GetChannels() const { return m_Channels; }
    const std::filesystem::path& GetPath() const { return m_Path; }

    static std::shared_ptr<Texture> Create(const std::string& path, bool flipVertically = true)
    {
        return std::make_shared<Texture>(path, flipVertically);
    }

private:
    void LoadFromFile(const std::filesystem::path& path, bool flipVertically);

private:
    uint32_t m_RendererID = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Channels = 0;

    GLenum m_InternalFormat = 0;
    GLenum m_DataFormat = 0;

    std::filesystem::path m_Path;
};

struct FrameData
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::vec3 CameraPosition;
};

// class Material
// {
// public:
//     virtual ~Material() = default;

//     virtual void Bind(const Shader& shader) const = 0;
//     // virtual const Shader& GetShader() const = 0;
// };

// class PhongMaterial : public Material
// {
// public:
//     PhongMaterial()
//         // : m_ShaderHandle(shaderHandle)
//     {
//     }

//     void Bind(const Shader& shader) const override
//     {
//         // const Shader& shader = GetShader();
//         shader.Bind();

//         // Frame uniforms
//         // shader.SetMat4f("u_View", frame.View);
//         // shader.SetMat4f("u_Projection", frame.Projection);
//         // shader.SetVec3f("u_ViewPos", frame.CameraPosition);

//         // Material uniforms
//         shader.SetVec3f("material_diffuseColor", DiffuseColor);
//         shader.SetVec3f("material_specularColor", SpecularColor);
//         shader.Set1f("material_shininess", Shininess);

//         BindTextures(shader);
//     }
// private:
//     void BindTextures(const Shader& shader) const
//     {
//         auto assetManager = Project::GetActive()->GetAssetManager();
//         uint32_t slot = 0;

//         shader.Set1i("material_hasDiffuseMap", assetManager->IsAssetHandleValid(DiffuseMap));
//         // shader.Set1i("material_hasSpecularMap", assetManager->IsAssetHandleValid(SpecularMap));

//         if (assetManager->IsAssetHandleValid(DiffuseMap))
//         {
//             auto tex = GetTexture(DiffuseMap);
//             tex->Bind(slot);
//             shader.Set1i("material_diffuseMap", slot++);
//         }

//         // if (assetManager->IsAssetHandleValid(SpecularMap))
//         // {
//         //     auto tex = GetTexture(SpecularMap);
//         //     tex->Bind(slot);
//         //     shader.Set1i("material_specularMap", slot++);
//         // }
//     }

//     // const Shader& GetShader() const override
//     // {
//     //     auto shaderAsset = AssetManager::GetAsset<ShaderAsset>(m_ShaderHandle);
//     //     return *shaderAsset->ShaderData;
//     // }

//     std::shared_ptr<Texture> GetTexture(AssetHandle handle) const
//     {
//         auto texAsset = AssetManager::GetAsset<TextureAsset>(handle);
//         return texAsset->TextureData;
//     }

// public:
//     glm::vec3 DiffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
//     glm::vec3 SpecularColor = glm::vec3(1.0f, 1.0f, 1.0f);
//     float Shininess = 32.0f;

//     AssetHandle DiffuseMap;
//     // AssetHandle SpecularMap;

//     // private:
//         // AssetHandle m_ShaderHandle;
// };

class Material
{
public:
    Material() = default;

    void Bind(const Shader& shader) const
    {
        shader.Bind();
        shader.SetVec3f("material_diffuseColor", DiffuseColor);
        shader.SetVec3f("material_specularColor", SpecularColor);
        shader.Set1f("material_shininess", Shininess);

        auto assetManager = Project::GetActive()->GetAssetManager();
        uint32_t slot = 0;

        shader.Set1i("material_hasDiffuseMap", assetManager->IsAssetHandleValid(DiffuseMap));
        // shader.Set1i("material_hasSpecularMap", assetManager->IsAssetHandleValid(SpecularMap));

        if (assetManager->IsAssetHandleValid(DiffuseMap))
        {
            auto tex = GetTexture(DiffuseMap);
            tex->Bind(slot);
            shader.Set1i("material_diffuseMap", slot++);
        }

        // if (assetManager->IsAssetHandleValid(SpecularMap))
        // {
        //     auto tex = GetTexture(SpecularMap);
        //     tex->Bind(slot);
        //     shader.Set1i("material_specularMap", slot++);
        // }
    }

    void ApplyDescription(const MaterialDesc& desc)
    {
        DiffuseColor = desc.DiffuseColor;
        SpecularColor = desc.SpecularColor;
        Shininess = desc.Shininess;
        DiffuseMap = desc.DiffuseMap;
    }
private:
    std::shared_ptr<Texture> GetTexture(AssetHandle handle) const
    {
        auto texAsset = AssetManager::GetAsset<TextureAsset>(handle);
        return texAsset->TextureData;
    }

public:
    glm::vec3 DiffuseColor;
    glm::vec3 SpecularColor;
    float Shininess;

    AssetHandle DiffuseMap = 0;
    // AssetHandle SpecularMap;
};

inline BufferLayout CreateVertexBufferLayout()
{
    return {
        { ShaderDataType::Float3, "a_Position" },
        { ShaderDataType::Float3, "a_Normal" },
        { ShaderDataType::Float2, "a_TexCoords" },
    };
}

// class Geometry
// {
// public:
//     Geometry(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

// public:
//     void Bind() const;
//     void Unbind() const;
//     uint32_t GetIndexCount() const { return m_IndexBuffer->GetCount(); }
//     static std::shared_ptr<Geometry> Generate(MeshPrimitive primitive);

// private:
//     static std::shared_ptr<Geometry> GenerateCube();
//     static std::shared_ptr<Geometry> GeneratePlane();

//     std::shared_ptr<VertexArray> m_VertexArray;
//     std::shared_ptr<VertexBuffer> m_VertexBuffer;
//     std::shared_ptr<IndexBuffer> m_IndexBuffer;
// };

// class Mesh {
// public:
//     // Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material);
//     Mesh(std::shared_ptr<Geometry> geometry);

//     void Draw(const Shader& shader) const;
//     void DrawRaw() const;

//     const std::shared_ptr<Geometry>& GetGeometry() const { return m_Geometry; }
//     // const std::shared_ptr<Material>& GetMaterial() const { return m_Material; }
// private:
//     std::shared_ptr<Geometry> m_Geometry;
//     // std::shared_ptr<Material> m_Material;
// };

class Mesh
{
public:
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indicies);

    void Bind() const;
    void Unbind() const;

    void Draw() const;
    void DrawLines() const;

    const AABB& GetLocalAABB() const { return m_LocalAABB; }
    static std::shared_ptr<Mesh> Generate(MeshPrimitive primitive);

private:
    static std::shared_ptr<Mesh> GenerateCube();
    static std::shared_ptr<Mesh> GeneratePlane();

private:
    std::shared_ptr<VertexArray> m_VertexArray;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
    AABB m_LocalAABB;
};