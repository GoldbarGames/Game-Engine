#include "Mesh.h"
#include "TransientBuffer.h"
#include "ModelMaterials.h"
#include "render/MeshPool.h"
#include "render/RenderDevice.h"
#include <glm/ext/matrix_float4x4.hpp>

// VAO / VBO / IBO / instanceVBO hold render-device handle ids (the members are
// plain unsigned ints so Mesh's layout is unchanged for games).

Mesh::Mesh()
{
    VAO = 0;
    VBO = 0;
    IBO = 0;
    indexCount = 0;
}

Mesh::~Mesh()
{
    ClearMesh();
    ForgetMeshMaterial(this);   // a material imported with it (glTF), if any
    MeshPoolForget(this);       // and its copy in the shared mesh pool
}

void Mesh::CreateMesh(float* vertices, unsigned int* indices,
    unsigned int numOfVertices, unsigned int numOfIndices, unsigned int v,
    unsigned int uvOffset, unsigned int normalOffset, int tangentOffset)
{
    RenderDevice& device = Device();
    indexCount = numOfIndices;

    const VertexArrayHandle vao = device.CreateVertexArray();
    const BufferHandle ibo = device.CreateIndexBuffer(sizeof(indices[0]) * numOfIndices, indices);
    const BufferHandle vbo = device.CreateBuffer(sizeof(vertices[0]) * numOfVertices, vertices, BufferUsage::Static);
    device.SetIndexBuffer(vao, ibo);

    // Every v floats is a new vertex: position (3) at 0, UV (2) at uvOffset.
    const size_t stride = sizeof(vertices[0]) * v;
    device.SetVertexAttribute(vao, 0, vbo, 3, stride, 0);
    device.SetVertexAttribute(vao, 1, vbo, 2, stride, sizeof(vertices[0]) * uvOffset);

    // Normals - present whenever a normal offset is given (stride 8 or 11)
    if (normalOffset > 0)
        device.SetVertexAttribute(vao, 2, vbo, 3, stride, sizeof(vertices[0]) * normalOffset);

    // Tangent (location 7, avoids the instancing mat4 slots 3-6)
    if (tangentOffset >= 0)
        device.SetVertexAttribute(vao, 7, vbo, 3, stride, sizeof(vertices[0]) * tangentOffset);

    VAO = vao.id;
    VBO = vbo.id;
    IBO = ibo.id;
}

void Mesh::UpdateVertices(const float* vertices, unsigned int floatCount)
{
    if (VBO == 0 || vertices == nullptr || floatCount == 0)
        return;
    Device().UpdateBuffer(BufferHandle(VBO), 0, sizeof(float) * floatCount, vertices);
}

void Mesh::BindMesh()
{
    Device().BindVertexArray(VertexArrayHandle(VAO));
}

namespace
{
    // A mat4 attribute occupies four consecutive vec4 locations (3-6),
    // advancing once per instance.
    void PointInstanceMatrices(VertexArrayHandle vao, BufferHandle buffer, size_t offset)
    {
        for (unsigned int i = 0; i < 4; i++)
            Device().SetVertexAttribute(vao, 3 + i, buffer, 4, sizeof(glm::mat4),
                offset + i * sizeof(glm::vec4), 1);
    }
}

void Mesh::SetInstances(const glm::mat4* matrices, unsigned int count, bool dynamic)
{
    instanceCount = count;
    if (count == 0)
        return;

    RenderDevice& device = Device();
    const BufferUsage usage = dynamic ? BufferUsage::Dynamic : BufferUsage::Static;
    if (instanceVBO == 0)
        instanceVBO = device.CreateBuffer(count * sizeof(glm::mat4), matrices, usage).id;
    else
        device.ReplaceBuffer(BufferHandle(instanceVBO), count * sizeof(glm::mat4), matrices, usage);

    // (Re)enable the instance attributes every call: ClearInstances() disables
    // them again after the draw, so a mesh that is also drawn non-instanced
    // (shadow pass) never keeps the per-instance divisors on locations 3-6.
    PointInstanceMatrices(VertexArrayHandle(VAO), BufferHandle(instanceVBO), 0);
}

void Mesh::SetInstancesTransient(const glm::mat4* matrices, unsigned int count)
{
    const TransientAlloc a = TransientUpload(matrices, count * sizeof(glm::mat4));
    if (!a.Valid())
    {
        SetInstances(matrices, count, true);   // ring unavailable: the mesh's own buffer
        return;
    }

    instanceCount = count;
    PointInstanceMatrices(VertexArrayHandle(VAO), BufferHandle(a.buffer), a.offset);
}

void Mesh::ClearInstances()
{
    instanceCount = 0;
    // Disable the instance attributes even when the matrices came from the
    // transient ring (no instanceVBO of our own); disabling unused ones is harmless.
    if (VAO == 0)
        return;
    for (unsigned int i = 0; i < 4; i++)
        Device().DisableVertexAttribute(VertexArrayHandle(VAO), 3 + i);
}

void Mesh::RenderMesh(unsigned int instanceAmount)
{
    if (indexCount > 0)
    {
        // Meshes with instance matrices (SetInstances) draw all instances in
        // one call; instanceAmount can override with a smaller count
        const unsigned int instances = (instanceAmount > 0) ? instanceAmount : instanceCount;
        Device().DrawIndexed(VertexArrayHandle(VAO), Primitive::Triangles, indexCount, (int)instances);
    }
}

void Mesh::ClearMesh()
{
    RenderDevice& device = Device();

    BufferHandle vbo(VBO), ibo(IBO), instances(instanceVBO);
    VertexArrayHandle vao(VAO);
    device.DestroyBuffer(vbo);
    device.DestroyVertexArray(vao);
    device.DestroyBuffer(ibo);
    device.DestroyBuffer(instances);
    VBO = 0;
    VAO = 0;
    IBO = 0;
    instanceVBO = 0;
    instanceCount = 0;
    indexCount = 0;
}
