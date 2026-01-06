//
// Created by Mohamad on 06/01/2026.
//

#ifndef THE_LOST_HEROIN_SCENE_BUILDER_HPP
#define THE_LOST_HEROIN_SCENE_BUILDER_HPP

#include <memory>
#include <glm/vec2.hpp>

class Player;
class InputService;
class Scene;

namespace GameObjects {
class Sprite;
}

class SceneBuilder {
public:
    struct BuildResult {
        Player* player{nullptr};
        std::shared_ptr<GameObjects::Sprite> groundSprite;
    };

    BuildResult build(Scene& scene,
                      InputService* inputService,
                      const glm::vec2& worldSize,
                      const glm::vec2& playerSize) const;

    static void applyPlayerTransform(Player& player,
                                     const glm::vec2& worldSize,
                                     const glm::vec2& playerSize);
};

#endif // THE_LOST_HEROIN_SCENE_BUILDER_HPP
