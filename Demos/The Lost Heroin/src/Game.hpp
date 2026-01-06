//
// Created by Mohamad on 04/12/2025.
//

#ifndef GL2D_GAME_HPP
#define GL2D_GAME_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

#include "Engine/Scene.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "InputSystem/InputService.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "Graphics/Window.hpp"
class GLFWwindow;

namespace Rendering {
class Renderer;
}

class Player;
class InputController;
class SceneBuilder;

class Game {
public:
    Game();
    ~Game();

    Game(const Game &other) = delete;
    Game &operator=(const Game &other) = delete;
    Game(Game &&other) = delete;
    Game &operator=(Game &&other) = delete;

    void setup();
    void update();
    void exit();
    void setPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    void updateCameraViewport(int width, int height);
    void queueFramebufferSize(int width, int height);

private:
    struct BackgroundLayerInstance {
        std::vector<TransformComponent*> transforms;
        glm::vec2 basePosition{0.0f};
        float parallax{0.0f};
        glm::vec2 baseCenter{0.0f};
        float tileWidth{0.0f};
        int tileCount{0};
    };

    void updateProjection();
    void updateCameraTargetOffset();
    void processPendingFramebufferResize();
    void loadBackgroundChapter(size_t index);
    void clearBackgroundChapter();
    void updateBackgroundParallax();
    glm::vec2 cameraCenter() const;

    std::unique_ptr<Graphics::Window> m_window;
    std::unique_ptr<Rendering::Renderer> m_renderer;
    Player *m_player{nullptr};
    std::unique_ptr<InputService> m_inputService;
    std::unique_ptr<InputController> m_inputController;
    std::unique_ptr<SceneBuilder> m_sceneBuilder;
    std::unique_ptr<Camera> m_camera;
    Scene m_scene;
    std::shared_ptr<GameObjects::Sprite> m_groundSprite;

    static constexpr float WORLD_WIDTH = 1920.0f;
    static constexpr float WORLD_HEIGHT = 1080.0f;
    glm::vec2 m_spriteSize{400.0f, 400.0f};
    glm::vec4 m_clearColor{0.05f, 0.05f, 0.1f, 1.0f};
    glm::mat4 m_projection{1.0f};
    float m_playbackSpeed{0.5f};
    glm::ivec2 m_pendingFramebufferSize{0, 0};
    bool m_hasPendingFramebufferSize{false};
    bool m_needsBackgroundReload{false};
    std::vector<Entity*> m_backgroundEntities;
    std::vector<BackgroundLayerInstance> m_backgroundLayers;
    size_t m_currentBackgroundChapter{0};
};

#endif //GL2D_GAME_HPP
