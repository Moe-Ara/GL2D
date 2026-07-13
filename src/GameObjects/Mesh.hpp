//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_MESH_HPP
#define GL2D_MESH_HPP

#include <vector>
#include <memory>
#include <cstdint>
#include <GL/glew.h>
#include "Vertex.hpp"
#include "Texture.hpp"
namespace GameObjects {

    class Mesh {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, std::shared_ptr<Texture> texture);

        virtual ~Mesh();

        Mesh(const Mesh &other) = delete;

        Mesh &operator=(const Mesh &other) = delete;

        Mesh(Mesh &&other) = delete;

        Mesh &operator=(Mesh &&other) = delete;
        void bind()const;
        void unbind()const;
        void render()const;
        void updateVertices(const std::vector<Vertex>& vertices);
        void setTexture(std::shared_ptr<Texture> texture);
    private:
        void releaseGpuResources() noexcept;
        void createVertexBuffer(const std::vector<Vertex>& vertices);
        void createIndexBuffer(const std::vector<uint32_t>& indices);
        GLuint m_vbo{},m_ibo{},m_vao{};
        std::shared_ptr<Texture> m_texture;
        GLsizei m_indexCount{};
        GLsizeiptr m_vertexBufferSize{};
        uint32_t m_maxIndex{};

    };

} // Graphics

#endif //GL2D_MESH_HPP
