#include "TextureManager.hpp"

namespace Managers {
    std::unordered_map<std::string, std::weak_ptr<GameObjects::Texture>> TextureManager::m_textureCache;

    std::shared_ptr<GameObjects::Texture> TextureManager::loadTexture(const std::string &filepath, bool useSRGB) {
        auto it = m_textureCache.find(filepath);
        if (it != m_textureCache.end()) {
            if (auto existingTexture = it->second.lock()) {
                return existingTexture; // Return cached texture if still alive
            }
        }

        // Correct: Using 'new' with std::shared_ptr
        auto newTexture = std::shared_ptr<GameObjects::Texture>(new GameObjects::Texture(filepath, useSRGB));
        m_textureCache[filepath] = newTexture;
        return newTexture;
    }

    void TextureManager::cleanUnusedTextures() {
        for (auto it = m_textureCache.begin(); it != m_textureCache.end();) {
            if (it->second.expired()) {
                it = m_textureCache.erase(it); // Remove expired textures
            } else {
                ++it;
            }
        }
    }
} // namespace Managers
