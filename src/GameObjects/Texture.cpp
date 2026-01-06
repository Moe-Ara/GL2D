#include <iostream>
#include "Texture.hpp"
#include "third_party/stb/stb_image.h"
#include "Exceptions/TextureException.hpp"

namespace GameObjects {

    Texture::Texture(const std::string &filepath, bool useSRGB) {
        glGenTextures(1, &m_textureID);
        bind();

        // Set texture wrapping and filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Enable Anisotropic Filtering if supported
        if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
        }

        // Load image data
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_numChannels, 0);
        if (!data) {
            std::cerr << "ERROR: Failed to load texture: " << filepath
                      << " (" << stbi_failure_reason() << ")" << std::endl;
            glDeleteTextures(1, &m_textureID);
            throw TextureException("Failed to load texture: " + filepath);
        }

        // Determine image format dynamically
        GLenum format, internalFormat;
        if (m_numChannels == 1) {
            format = GL_RED;
            internalFormat = GL_RED;
        } else if (m_numChannels == 3) {
            format = GL_RGB;
            internalFormat = useSRGB ? GL_SRGB : GL_RGB;
        } else if (m_numChannels == 4) {
            format = GL_RGBA;
            internalFormat = useSRGB ? GL_SRGB_ALPHA : GL_RGBA;
        } else {
            stbi_image_free(data);
            glDeleteTextures(1, &m_textureID);
            throw TextureException("Unsupported texture format: " + filepath);
        }

        // Upload texture data to OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        unbind();
    }

    Texture::~Texture() {
        unbind();
        glDeleteTextures(1, &m_textureID);
    }

    void Texture::bind(GLenum textureUnit) const {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
    }

    void Texture::unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
} // namespace GameObjects
