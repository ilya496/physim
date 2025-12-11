#include "Model.h"

#include "stb_image.h"
#include <iostream>

Texture::Texture(const std::string& path, bool flipVertically)
    : m_Path(path)
{
    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, 0);
    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return;
    }

    GLenum internalFormat = 0;
    GLenum dataFormat = 0;
    if (m_Channels == 1)
    {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    }
    else if (m_Channels == 3)
    {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }
    else if (m_Channels == 4)
    {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    }

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}

Texture::~Texture()
{
    glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(uint32_t slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}

void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

Geometry::Geometry(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    m_VertexArray = VertexArray::Create();
    m_VertexArray->Bind();

    m_VertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex)));
    m_VertexBuffer->SetLayout(CreateVertexBufferLayout());
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    m_IndexBuffer = IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);

    m_VertexArray->Unbind();
}

void Geometry::Bind() const
{
    m_VertexArray->Bind();
}

void Geometry::Unbind() const
{
    m_VertexArray->Unbind();
}

std::shared_ptr<Geometry> Geometry::Generate(MeshPrimitive primitive)
{
    switch (primitive)
    {
    case MeshPrimitive::CUBE: return GenerateCube();
    case MeshPrimitive::PLANE: return GeneratePlane();
    default:
        return nullptr;
    }
}

std::shared_ptr<Geometry> Geometry::GenerateCube()
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

    ComputeTangents(vertices, indices);
    return std::make_shared<Geometry>(vertices, indices);
}

std::shared_ptr<Geometry> Geometry::GeneratePlane()
{
    std::vector<Vertex> vertices = {
        {{-1.0f, 0.0f,  1.0f}, {0, 1, 0}, {0, 0}},
        {{ 1.0f, 0.0f,  1.0f}, {0, 1, 0}, {1, 0}},
        {{ 1.0f, 0.0f, -1.0f}, {0, 1, 0}, {1, 1}},
        {{-1.0f, 0.0f, -1.0f}, {0, 1, 0}, {0, 1}},
    };

    std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };

    ComputeTangents(vertices, indices);
    return std::make_shared<Geometry>(vertices, indices);
}

void Geometry::ComputeTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    for (auto& v : vertices) {
        v.Tangent = glm::vec3(0.0f);
        v.Bitangent = glm::vec3(0.0f);
    }

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Vertex& v0 = vertices[indices[i + 0]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        glm::vec3 e1 = v1.Position - v0.Position;
        glm::vec3 e2 = v2.Position - v0.Position;

        glm::vec2 dUV1 = v1.TexCoords - v0.TexCoords;
        glm::vec2 dUV2 = v2.TexCoords - v0.TexCoords;

        float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y + 1e-8f);

        glm::vec3 tangent;
        tangent.x = f * (dUV2.y * e1.x - dUV1.y * e2.x);
        tangent.y = f * (dUV2.y * e1.y - dUV1.y * e2.y);
        tangent.z = f * (dUV2.y * e1.z - dUV1.y * e2.z);

        glm::vec3 bitangent;
        bitangent.x = f * (-dUV2.x * e1.x + dUV1.x * e2.x);
        bitangent.y = f * (-dUV2.x * e1.y + dUV1.x * e2.y);
        bitangent.z = f * (-dUV2.x * e1.z + dUV1.x * e2.z);

        v0.Tangent += tangent;
        v1.Tangent += tangent;
        v2.Tangent += tangent;

        v0.Bitangent += bitangent;
        v1.Bitangent += bitangent;
        v2.Bitangent += bitangent;
    }

    for (auto& v : vertices) {
        v.Tangent = glm::normalize(v.Tangent);
        v.Bitangent = glm::normalize(v.Bitangent);
    }
}


Mesh::Mesh(std::shared_ptr<Geometry> geometry,
    std::shared_ptr<Material> material)
    : m_Geometry(std::move(geometry))
    , m_Material(std::move(material))
{
}


void Mesh::Draw(const Shader& shader) const
{
    if (m_Material)
    {
        m_Material->Apply(shader);
    }

    m_Geometry->Bind();
    glDrawElements(GL_TRIANGLES, m_Geometry->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
    m_Geometry->Unbind();
}