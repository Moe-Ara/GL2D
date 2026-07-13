#include <algorithm>
#include <memory>
#include <utility>
#include "Texture.hpp"
#include "third_party/stb/stb_image.h"
#include "Exceptions/TextureException.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace GameObjects {

    Texture::Texture(const std::string &filepath, bool useSRGB) {
        if (filepath.empty()) {
            throw TextureException("Texture path cannot be empty");
        }
        m_ownerContext = glfwGetCurrentContext();
        if (!m_ownerContext) {
            throw TextureException(
                "Texture creation requires a current GLFW OpenGL context: '" +
                filepath + "'");
        }

        stbi_set_flip_vertically_on_load_thread(1);
        unsigned char* decoded = stbi_load(
            filepath.c_str(), &m_width, &m_height, &m_numChannels, 0);
        const char* failureReason = decoded ? nullptr : stbi_failure_reason();
        stbi_set_flip_vertically_on_load_thread(0);
        std::unique_ptr<unsigned char, decltype(&stbi_image_free)> data(
            decoded, &stbi_image_free);
        if (!data) {
            throw TextureException("Failed to decode '" + filepath + "': " +
                                   (failureReason ? failureReason : "unknown image error"));
        }
        if (m_width <= 0 || m_height <= 0) {
            throw TextureException("Decoded texture has invalid dimensions: " + filepath);
        }

        GLenum format = 0;
        GLint internalFormat = 0;
        switch (m_numChannels) {
            case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
            case 2:
                format = GL_RG;
                internalFormat = GL_RG8;
                break;
            case 3:
                format = GL_RGB;
                internalFormat = useSRGB ? GL_SRGB8 : GL_RGB8;
                break;
            case 4:
                format = GL_RGBA;
                internalFormat = useSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
                break;
            default:
                throw TextureException("Unsupported channel count " +
                                       std::to_string(m_numChannels) + " in '" + filepath + "'");
        }

        GLint previousActiveTexture = 0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        GLint previousTexture = 0;
        GLint previousUnpackAlignment = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

        glGenTextures(1, &m_textureID);
        if (!m_textureID) {
            glActiveTexture(static_cast<GLenum>(previousActiveTexture));
            throw TextureException("OpenGL failed to allocate texture storage for '" + filepath + "'");
        }
        glBindTexture(GL_TEXTURE_2D, m_textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (m_numChannels == 1) {
            constexpr GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        } else if (m_numChannels == 2) {
            constexpr GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
            GLfloat maxAnisotropy = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                            std::max(1.0f, maxAnisotropy));
        }

        // Byte-packed rows avoid corruption for RGB widths that are not a
        // multiple of OpenGL's default four-byte unpack alignment.
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0,
                     format, GL_UNSIGNED_BYTE, data.get());
        glGenerateMipmap(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    }

    Texture::~Texture() {
        if (m_textureID && glfwGetCurrentContext() == m_ownerContext) {
            glDeleteTextures(1, &m_textureID);
        }
    }

    Texture::Texture(Texture&& other) noexcept
        : m_textureID(std::exchange(other.m_textureID, 0)),
          m_width(std::exchange(other.m_width, 0)),
          m_height(std::exchange(other.m_height, 0)),
          m_numChannels(std::exchange(other.m_numChannels, 0)),
          m_ownerContext(std::exchange(other.m_ownerContext, nullptr)) {}

    Texture& Texture::operator=(Texture&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (m_textureID && glfwGetCurrentContext() == m_ownerContext) {
            glDeleteTextures(1, &m_textureID);
        }
        m_textureID = std::exchange(other.m_textureID, 0);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_numChannels = std::exchange(other.m_numChannels, 0);
        m_ownerContext = std::exchange(other.m_ownerContext, nullptr);
        return *this;
    }

    void Texture::bind(GLenum textureUnit) const {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_textureID);
    }

    void Texture::unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
} // namespace GameObjects
