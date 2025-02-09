#ifndef GL2D_TEXTURE_HPP
#define GL2D_TEXTURE_HPP

#include <string>
#include <GL/glew.h>
#include "Managers/TextureManager.hpp"
namespace Managers {
    class TextureManager;
}
namespace GameObjects {

    class Texture {
        friend class Managers::TextureManager;
    public:
        virtual ~Texture();

        // Delete copy semantics
        Texture(const Texture &other) = delete;
        Texture &operator=(const Texture &other) = delete;

        // Enable move semantics
        Texture(Texture &&other) noexcept;
        Texture &operator=(Texture &&other) noexcept;

        // Binding with optional texture unit
        void bind(GLenum textureUnit = GL_TEXTURE0) const;
        void unbind() const;

        // Getters
        GLuint getID() const { return m_textureID; }
        int getWidth() const { return m_width; }
        int getHeight() const { return m_height; }

    private:
        explicit Texture(const std::string& filepath, bool useSRGB);

        GLuint m_textureID{};
        int m_width{}, m_height{}, m_numChannels{};
    };

} // namespace GameObjects
#endif