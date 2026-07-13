#include "TextureManager.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace Managers {
    std::unordered_map<GLFWwindow*, TextureManager::ContextCache>
        TextureManager::m_textureCaches;

    std::shared_ptr<GameObjects::Texture> TextureManager::loadTexture(const std::string &filepath, bool useSRGB) {
        GLFWwindow* context = glfwGetCurrentContext();
        if (!context) {
            throw std::logic_error(
                "TextureManager::loadTexture requires a current GLFW OpenGL context");
        }
        auto& cache = m_textureCaches[context];
        const std::size_t variant = useSRGB ? 1U : 0U;
        auto it = cache.find(filepath);
        if (it != cache.end()) {
            if (auto existingTexture = it->second[variant].lock()) {
                return existingTexture;
            }
        }

        auto newTexture = std::shared_ptr<GameObjects::Texture>(new GameObjects::Texture(filepath, useSRGB));
        cache[filepath][variant] = newTexture;
        return newTexture;
    }

    void TextureManager::cleanUnusedTextures() {
        for (auto contextIt = m_textureCaches.begin();
             contextIt != m_textureCaches.end();) {
            auto& cache = contextIt->second;
            for (auto textureIt = cache.begin(); textureIt != cache.end();) {
                if (textureIt->second[0].expired() &&
                    textureIt->second[1].expired()) {
                    textureIt = cache.erase(textureIt);
                } else {
                    ++textureIt;
                }
            }
            contextIt = cache.empty() ? m_textureCaches.erase(contextIt)
                                      : std::next(contextIt);
        }
    }
} // namespace Managers
