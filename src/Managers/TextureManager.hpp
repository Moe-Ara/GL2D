//
// Created by Mohamad on 05/02/2025.
//

#ifndef GL2D_TEXTUREMANAGER_HPP
#define GL2D_TEXTUREMANAGER_HPP
#include "GameObjects/Texture.hpp"
#include <array>
#include <unordered_map>
#include <memory>
#include <string>
struct GLFWwindow;
namespace Managers {

    class TextureManager {
    public:
        static std::shared_ptr<GameObjects::Texture> loadTexture(const std::string& filepath, bool useSRGB = false);
        static void cleanUnusedTextures();

    private:
        using TextureVariants = std::array<std::weak_ptr<GameObjects::Texture>, 2>;
        using ContextCache = std::unordered_map<std::string, TextureVariants>;
        static std::unordered_map<GLFWwindow*, ContextCache> m_textureCaches;
    };

} // namespace Managers
#endif //GL2D_TEXTUREMANAGER_HPP
