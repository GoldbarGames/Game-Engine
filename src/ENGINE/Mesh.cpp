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
    unsigned int uvOffset, unsigned int normalOffset)
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

    // Normals  - offset2, every v numbers is a new vertex
    if (v == 8)
    {
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]) * v, (void*)(sizeof(vertices[0]) * normalOffset));
        glEnableVertexAttribArray(2);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Note: Do NOT unbind GL_ELEMENT_ARRAY_BUFFER while VAO is bound, as IBO binding is part of VAO state
}

void Mesh::BindMesh()
{
    glBindVertexArray(VAO);
}


void Mesh::RenderMesh(unsigned int instanceAmount)
{
    if (indexCount > 0)
    {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

        // TODO: Also we need to calculate the model matrix 

        // TODO: The error could be that during Config() it only sets up game.entities when there are other entities,
        // and so when we try to render the mesh for one of those, it doesn't know what to do.
        // glDrawElementsInstanced(mode, indexCount, GL_UNSIGNED_INT, 0, instanceAmount);

        //if (instanceAmount > 0)
        //    glDrawElementsInstanced(mode, indexCount, GL_UNSIGNED_INT, 0, instanceAmount);
        //else
        glDrawElements(mode, indexCount, GL_UNSIGNED_INT, 0);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
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

    indexCount = 0;
}