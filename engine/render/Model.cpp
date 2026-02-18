#include "Model.h"

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

Texture::Texture(const std::filesystem::path& path, bool flipVertically)
    : m_Path(path)
{
    LoadFromFile(path, flipVertically);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_RendererID);
}

void Texture::LoadFromFile(const std::filesystem::path& path, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically);

    int width, height, channels;
    stbi_uc* data = stbi_load(
        path.string().c_str(),
        &width,
        &height,
        &channels,
        0
    );

    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << '\n';
        return;
    }

    m_Width = width;
    m_Height = height;
    m_Channels = channels;

    if (channels == 3)
    {
        m_InternalFormat = GL_RGB8;
        m_DataFormat = GL_RGB;
    }
    else if (channels == 4)
    {
        m_InternalFormat = GL_RGBA8;
        m_DataFormat = GL_RGBA;
    }
    else
    {
        std::cerr << "Unsupported texture format! (channels=" << channels << ")\n";
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
    glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

    glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTextureSubImage2D(
        m_RendererID,
        0,
        0,
        0,
        m_Width,
        m_Height,
        m_DataFormat,
        GL_UNSIGNED_BYTE,
        data
    );

    // glGenTextures(1, &m_RendererID);
    // glBindTexture(GL_TEXTURE_2D, m_RendererID);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    // glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}


void Texture::Bind(uint32_t slot) const
{
    // glActiveTexture(GL_TEXTURE0 + slot);
    // glBindTexture(GL_TEXTURE_2D, m_RendererID);
    glBindTextureUnit(slot, m_RendererID);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Geometry::Geometry(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
// {
//     m_VertexArray = VertexArray::Create();
//     m_VertexArray->Bind();

//     m_VertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));
//     m_VertexBuffer->SetLayout(CreateVertexBufferLayout());
//     m_VertexArray->AddVertexBuffer(m_VertexBuffer);

//     m_IndexBuffer = IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
//     m_VertexArray->SetIndexBuffer(m_IndexBuffer);

//     m_VertexArray->Unbind();
// }

// void Geometry::Bind() const
// {
//     m_VertexArray->Bind();
// }

// void Geometry::Unbind() const
// {
//     m_VertexArray->Unbind();
// }

// std::shared_ptr<Geometry> Geometry::Generate(MeshPrimitive primitive)
// {
//     switch (primitive)
//     {
//     case MeshPrimitive::CUBE: return GenerateCube();
//     case MeshPrimitive::PLANE: return GeneratePlane();
//     default:
//         return nullptr;
//     }
// }

// std::shared_ptr<Geometry> Geometry::GenerateCube()
// {
//     std::vector<Vertex> vertices = {
//         // Back
//         {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0}},
//         {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0}},
//         {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1}},
//         {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1}},

//         // Front
//         {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}},
//         {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}},
//         {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}},
//         {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}},

//         // Left
//         {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0}},
//         {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0}},
//         {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1}},
//         {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1}},

//         // Right
//         {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0}},
//         {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0}},
//         {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1}},
//         {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1}},

//         // Bottom
//         {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}},
//         {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}},
//         {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 0}},
//         {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 0}},

//         // Top
//         {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1}},
//         {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1}},
//         {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0}},
//         {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0}},
//     };

//     std::vector<uint32_t> indices = {
//         0, 1, 2, 0, 2, 3,     // back
//         4, 5, 6, 4, 6, 7,     // front
//         8, 9,10, 8,10,11,     // left
//         12,13,14,12,14,15,    // right
//         16,17,18,16,18,19,    // bottom
//         20,21,22,20,22,23     // top
//     };

//     return std::make_shared<Geometry>(vertices, indices);
// }

// std::shared_ptr<Geometry> Geometry::GeneratePlane()
// {
//     std::vector<Vertex> vertices = {
//         {{-1.0f, 0.0f,  1.0f}, {0, 1, 0}, {0, 0}},
//         {{ 1.0f, 0.0f,  1.0f}, {0, 1, 0}, {1, 0}},
//         {{ 1.0f, 0.0f, -1.0f}, {0, 1, 0}, {1, 1}},
//         {{-1.0f, 0.0f, -1.0f}, {0, 1, 0}, {0, 1}},
//     };

//     std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

//     return std::make_shared<Geometry>(vertices, indices);
// }

// Mesh::Mesh(std::shared_ptr<Geometry> geometry,
//     std::shared_ptr<Material> material)
//     : m_Geometry(std::move(geometry))
//     , m_Material(std::move(material))
// {
// }

// Mesh::Mesh(std::shared_ptr<Geometry> geometry)
//     : m_Geometry(std::move(geometry))
// {
// }


// void Mesh::Draw(const Shader& shader) const
// {
//     // if (m_Material)
//     // {
//         // m_Material->Bind(shader);
//     // }

//     m_Geometry->Bind();
//     glDrawElements(GL_TRIANGLES, m_Geometry->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
//     m_Geometry->Unbind();
// }

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_VertexArray = VertexArray::Create();
    m_VertexArray->Bind();

    m_VertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));
    m_VertexBuffer->SetLayout(CreateVertexBufferLayout());
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    m_IndexBuffer = IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);

    m_VertexArray->Unbind();

    AABB aabb;
    aabb.Min = glm::vec3(-FLT_MAX);
    aabb.Max = glm::vec3(FLT_MAX);

    for (const auto& v : vertices)
    {
        aabb.Min = glm::min(aabb.Min, v.Position);
        aabb.Max = glm::max(aabb.Max, v.Position);
    }

    m_LocalAABB = aabb;
}

void Mesh::Bind() const
{
    m_VertexArray->Bind();
}

void Mesh::Unbind() const
{
    m_VertexArray->Unbind();
}

void Mesh::Draw() const
{
    m_VertexArray->Bind();
    glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
    m_VertexArray->Unbind();
}

void Mesh::DrawLines() const
{
    m_VertexArray->Bind();
    glDrawElements(GL_LINES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
    m_VertexArray->Unbind();
}

std::shared_ptr<Mesh> Mesh::Generate(MeshPrimitive primitive)
{
    switch (primitive)
    {
    case MeshPrimitive::CUBE: return GenerateCube();
    case MeshPrimitive::PLANE: return GeneratePlane();
    default:
        return nullptr;
    }
}

std::shared_ptr<Mesh> Mesh::GenerateCube()
{
    std::vector<Vertex> vertices = {
        // Back
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1}},

        // Front
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}},

        // Left
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1}},

        // Right
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1}},

        // Bottom
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 0}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 0}},

        // Top
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0}},
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0}},
    };

    std::vector<uint32_t> indices = {
        0, 1, 2, 0, 2, 3,     // back
        4, 5, 6, 4, 6, 7,     // front
        8, 9,10, 8,10,11,     // left
        12,13,14,12,14,15,    // right
        16,17,18,16,18,19,    // bottom
        20,21,22,20,22,23     // top
    };

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<Mesh> Mesh::GeneratePlane()
{
    std::vector<Vertex> vertices = {
        {{-1.0f, 0.0f,  1.0f}, {0, 1, 0}, {0, 0}},
        {{ 1.0f, 0.0f,  1.0f}, {0, 1, 0}, {1, 0}},
        {{ 1.0f, 0.0f, -1.0f}, {0, 1, 0}, {1, 1}},
        {{-1.0f, 0.0f, -1.0f}, {0, 1, 0}, {0, 1}},
    };

    std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

    return std::make_shared<Mesh>(vertices, indices);
}