//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_SPRITE_HPP
#define GL2D_SPRITE_HPP

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include "Texture.hpp"
#include "Mesh.hpp"

namespace GameObjects {

    class Sprite {
    public:
        Sprite(glm::vec2 position, glm::vec2 size, glm::vec3 color);
        Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size);
        Sprite(std::shared_ptr<Texture> texture, glm::vec2 position, glm::vec2 size, int row, int column, int totalRows, int totalCols);

        virtual ~Sprite();

        // Delete copy & move constructors
        Sprite(const Sprite &other) = delete;
        Sprite &operator=(const Sprite &other) = delete;
        Sprite(Sprite &&other) = delete;
        Sprite &operator=(Sprite &&other) = delete;

        void draw() const;

        // Setters
        void setPosition(const glm::vec2& newPos);
        void setSize(const glm::vec2& newSize);
        void setUVCoords(const glm::vec4 &newUV);
        void setTexture(const std::shared_ptr<Texture> &newTexture);

        // Getters
        const glm::vec3 &getColor() const;
        const glm::vec4 &getUVCoords() const;
        const glm::vec2 &getSize() const;
        const glm::vec2 &getPosition() const;

        bool hasTexture() const;

    private:
        mutable std::shared_ptr<Mesh> m_mesh;
        std::shared_ptr<Texture> m_texture;
        glm::vec2 m_position{};
        glm::vec2 m_size{};
        glm::vec4 m_uvCoords{};
        glm::vec3 m_color{};
        mutable bool m_dirty{true};
        void ensureMesh() const;
        std::vector<Vertex> buildVertices() const;
    };

} // namespace GameObjects

#endif //GL2D_SPRITE_HPP
