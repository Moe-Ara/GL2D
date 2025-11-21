//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_TEXTUREMANAGER_HPP
#define GL2D_TEXTUREMANAGER_HPP
#include "GameObjects/Texture.hpp"
#include <unordered_map>
#include <memory>
#include <string>
namespace Managers {

    class TextureManager {
    public:
        static std::shared_ptr<GameObjects::Texture> loadTexture(const std::string& filepath, bool useSRGB = false);
        static void cleanUnusedTextures();

    private:
        static std::unordered_map<std::string, std::weak_ptr<GameObjects::Texture>> m_textureCache;
    };

} // namespace Managers
#endif //GL2D_TEXTUREMANAGER_HPP
