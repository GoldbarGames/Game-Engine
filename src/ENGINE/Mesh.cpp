#include "Mesh.h"
#include <glm/ext/matrix_float4x4.hpp>

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
}

void Mesh::CreateMesh(GLfloat* vertices, unsigned int* indices,
    unsigned int numOfVertices, unsigned int numOfIndices, unsigned int v,
    unsigned int uvOffset, unsigned int normalOffset, int tangentOffset)
{
    indexCount = numOfIndices;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    // Check size of element of array * number of elements in array
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * numOfIndices, indices, GL_STATIC_DRAW);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Check size of element of array * number of elements in array
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * numOfVertices, vertices, GL_STATIC_DRAW);

    // Vertices - no offset, every v numbers is a new vertex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]) * v, 0);
    glEnableVertexAttribArray(0);

    // UVs - offset1, every v numbers is a new vertex
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]) * v, (void*)(sizeof(vertices[0]) * uvOffset));
    glEnableVertexAttribArray(1);

    // Normals - present whenever a normal offset is given (stride 8 or 11)
    if (normalOffset > 0)
    {
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]) * v, (void*)(sizeof(vertices[0]) * normalOffset));
        glEnableVertexAttribArray(2);
    }

    // Tangent (location 7, avoids the instancing mat4 slots 3-6)
    if (tangentOffset >= 0)
    {
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]) * v, (void*)(sizeof(vertices[0]) * tangentOffset));
        glEnableVertexAttribArray(7);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Note: Do NOT unbind GL_ELEMENT_ARRAY_BUFFER while VAO is bound, as IBO binding is part of VAO state
}

void Mesh::BindMesh()
{
    glBindVertexArray(VAO);
}

void Mesh::SetInstances(const glm::mat4* matrices, unsigned int count, bool dynamic)
{
    instanceCount = count;
    if (count == 0)
        return;

    glBindVertexArray(VAO);

    bool firstUpload = (instanceVBO == 0);
    if (firstUpload)
        glGenBuffers(1, &instanceVBO);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(glm::mat4), matrices,
        dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    if (firstUpload)
    {
        // A mat4 attribute occupies four consecutive vec4 locations (3-6),
        // advancing once per instance
        for (int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(3 + i);
            glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                (void*)(i * sizeof(glm::vec4)));
            glVertexAttribDivisor(3 + i, 1);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void Mesh::RenderMesh(unsigned int instanceAmount)
{
    if (indexCount > 0)
    {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // Meshes with instance matrices (SetInstances) draw all instances in
        // one call; instanceAmount can override with a smaller count
        unsigned int instances = (instanceAmount > 0) ? instanceAmount : instanceCount;
        if (instances > 0)
            glDrawElementsInstanced(mode, indexCount, GL_UNSIGNED_INT, 0, instances);
        else
            glDrawElements(mode, indexCount, GL_UNSIGNED_INT, 0);

        // Unbind the VAO BEFORE the element buffer: unbinding
        // GL_ELEMENT_ARRAY_BUFFER while the VAO is still bound would strip
        // the IBO association from the VAO state itself
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

void Mesh::ClearMesh()
{
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    if (IBO != 0)
    {
        glDeleteBuffers(1, &IBO);
        IBO = 0;
    }

    if (instanceVBO != 0)
    {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }

    instanceCount = 0;
    indexCount = 0;
}