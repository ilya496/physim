#pragma once

#include <cstdint>
#include <memory>

#include "Buffer.h"

class VertexArray
{
public:
    ~VertexArray() = default;

public:
    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(const VertexBuffer& vertexBuffer);
    void SetIndexBuffer(const IndexBuffer& IndexBuffer);

    const std::vector<VertexBuffer>& GetVertexBuffers() const;
    const IndexBuffer& GetIndexBuffer() const;

    static VertexArray Create();
};