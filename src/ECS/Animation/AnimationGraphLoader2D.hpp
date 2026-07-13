#pragma once

#include "ECS/Animation/AnimationGraph2D.hpp"

#include <functional>
#include <memory>
#include <string>

namespace GameObjects {
class Texture;
}

namespace ECS {

class AnimationGraphLoader2D {
public:
    using TextureResolver = std::function<std::shared_ptr<GameObjects::Texture>(const std::string&)>;

    // Loads the established GL2D animation metadata schema into an immutable ECS graph.
    // Relative asset-path policy is owned by the supplied resolver.
    static std::shared_ptr<const AnimationGraph2D> loadFromFile(
        const std::string& metadataPath, const TextureResolver& textureResolver);
};

} // namespace ECS
