//
// Loads UI screens from JSON into runtime UI elements.
//

#ifndef GL2D_UILOADER_HPP
#define GL2D_UILOADER_HPP

#include <functional>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "UIElements.hpp"

namespace UI {

class UILoader {
public:
    using TextureResolver = std::function<GameObjects::Texture*(const std::string& id)>;

    // Loads a UIScreen from a JSON file. TextureResolver can map string ids/paths to textures (nullable).
    static UIScreen loadFromFile(const std::string& path, TextureResolver resolver = {});
};

} // namespace UI

#endif //GL2D_UILOADER_HPP
