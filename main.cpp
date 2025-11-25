#include "Engine/Scene.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/IComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/Animation.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "Graphics/Animation/Frame.hpp"
#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "InputSystem/InputService.hpp"
#include "Managers/TextureManager.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "ParticleSystem/ParticleSystem.hpp"
#include "ParticleSystem/ParticleEffectLoader.hpp"
#include "AudioSystem/AudioManager.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
Camera* gActiveCamera = nullptr;

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height) {
    const int safeWidth = std::max(width, 1);
    const int safeHeight = std::max(height, 1);
    glViewport(0, 0, safeWidth, safeHeight);
    if (gActiveCamera) {
        gActiveCamera->setViewportSize(static_cast<float>(safeWidth),
                                       static_cast<float>(safeHeight));
    }
}
} // namespace

struct AnimationLoadResult {
    std::map<std::string, std::shared_ptr<Graphics::Animation>> animations;
    std::string initialState;
};

AnimationLoadResult loadAnimationsFromMetadata(const std::string &metadataPath) {
    AnimationLoadResult result{};
    auto metadata = Loaders::AnimationMetadataLoader::loadFromFile(metadataPath);
    result.initialState = metadata.initialState;
    std::unordered_map<std::string, std::shared_ptr<GameObjects::Texture>> textureCache;
    auto fetchTexture = [&](const std::string &path) -> std::shared_ptr<GameObjects::Texture> {
        if (path.empty())
            return nullptr;
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            return it->second;
        }
        auto texture = Managers::TextureManager::loadTexture(path);
        textureCache[path] = texture;
        return texture;
    };

    auto atlasTexture = fetchTexture(metadata.atlas.texturePath);

    for (const auto &entry : metadata.animations) {
        if (entry.frames.empty()) {
            continue;
        }

        const float animationFrameDuration = entry.defaultFrameDuration > 0.0f
                                                 ? entry.defaultFrameDuration
                                                 : metadata.defaultFrameDuration;

        auto animation = std::make_shared<Graphics::Animation>(
                metadata.atlas.rows, metadata.atlas.cols, animationFrameDuration,
                entry.loop, entry.playbackMode);
        animation->setName(entry.name);
        animation->setSharedTexture(atlasTexture);
        animation->setFrameDuration(animationFrameDuration);
        for (const auto &transition : entry.transitions) {
            animation->addTransition(Graphics::AnimationTransition{
                    transition.target, transition.condition});
        }

        for (const auto &frameMeta : entry.frames) {
            Graphics::Frame frame{};
            frame.row = frameMeta.row;
            frame.column = frameMeta.column;
            frame.useCustomUV = frameMeta.useCustomUV;
            frame.uvRect = frameMeta.uvRect;
            frame.duration = frameMeta.duration > 0.0f ? frameMeta.duration
                                                       : animationFrameDuration;
            frame.eventName = frameMeta.eventName;

            if (!frameMeta.texturePath.empty()) {
                frame.texture = fetchTexture(frameMeta.texturePath);
            } else if (atlasTexture) {
                frame.texture = atlasTexture;
            } else {
                frame.texture = nullptr;
            }

            animation->addFrame(frame);
        }
        result.animations[entry.name] = animation;
    }

    return result;
}

class PlayerMover : public IUpdatableComponent {
public:
    PlayerMover(InputService &input, RigidBodyComponent *rigidBody, float moveSpeed, float jumpImpulse, float groundY)
            : m_input(input), m_rigidBody(rigidBody), m_moveSpeed(moveSpeed), m_jumpImpulse(jumpImpulse), m_groundY(groundY) {}

    void update(Entity & /*owner*/, double dt) override {
        if (!m_rigidBody || !m_rigidBody->body()) {
            return;
        }

        bool jumpPressed = false;
        for (const auto &evt : m_input.getActionEvents()) {
            const bool pressed = evt.eventType == InputEventType::ButtonPressed;
            const bool released = evt.eventType == InputEventType::ButtonReleased;
            if (evt.actionName == "MoveLeft") {
                if (pressed) m_moveLeft = true;
                else if (released) m_moveLeft = false;
            }
            if (evt.actionName == "MoveRight") {
                if (pressed) m_moveRight = true;
                else if (released) m_moveRight = false;
            }
            if (evt.actionName == "Jump" && pressed) {
                jumpPressed = true;
            }
        }

        float desiredDir = (m_moveRight ? 1.0f : 0.0f) - (m_moveLeft ? 1.0f : 0.0f);

        auto *body = m_rigidBody->body();
        auto velocity = body->getVelocity();
        velocity.x = desiredDir * m_moveSpeed;

        const float groundedPosEpsilon = PhysicsUnits::toUnits(0.05f);
        const float groundedVelEpsilon = PhysicsUnits::toUnits(0.015f);
        const bool grounded = body->getPosition().y <= m_groundY + groundedPosEpsilon &&
                              std::abs(velocity.y) <= groundedVelEpsilon;
        if (jumpPressed && grounded) {
            velocity.y = m_jumpImpulse;
        } else if (grounded && velocity.y < 0.0f) {
            // Snap small downward drift to zero when on ground.
            velocity.y = 0.0f;
        }

        body->setVelocity(velocity);
    }

private:
    InputService &m_input;
    RigidBodyComponent *m_rigidBody{nullptr};
    float m_moveSpeed{PhysicsUnits::toUnits(1.5f)};
    float m_jumpImpulse{PhysicsUnits::toUnits(4.8f)};
    float m_groundY{0.0f};
    bool m_moveLeft{false};
    bool m_moveRight{false};
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    GLFWwindow *window = glfwCreateWindow(1280, 720, "GL2D Prototype", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    Rendering::Renderer renderer("Shaders/vertex.vert", "Shaders/fragment.frag");
//    Rendering::ParticleRenderer particleRenderer;
    Camera camera(1280.0f, 720.0f);
    gActiveCamera = &camera;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);
    camera.setFollowMode(CameraFollowMode::HardLock);
    camera.setDamping(8.0f);
    Audio::AudioManager audio;
    try {
        audio.init();
        // Register a couple of example triggers (ensure these files exist in assets/audio).
//        Audio::PlayParams jumpParams;
//        jumpParams.volume = 0.9f;
//        jumpParams.pitch = 1.0f;
//        jumpParams.spatial = true;
//        jumpParams.minDistance = 1.0f;
//        jumpParams.maxDistance = 35.0f;
//        audio.registerSfxTrigger("sfx_jump", "assets/audio/jump.ogg", jumpParams, 2);

//        Audio::PlayParams landParams;
//        landParams.volume = 0.8f;
//        landParams.spatial = true;
//        landParams.minDistance = 1.0f;
//        landParams.maxDistance = 35.0f;
//        audio.registerSfxTrigger("sfx_land", "assets/audio/land.ogg", landParams, 2);

        // Background music (streaming). Ensure this file exists.
        if (audio.playMusic("assets/audio/bgm1.mp3", false, 500)) {
            std::cout << "[Audio] Started BGM assets/audio/bgm1.mp3" << std::endl;
            audio.queueNextMusic("assets/audio/bgm.mp3", true, 2000);
        } else {
            std::cerr << "[Audio] Could not start BGM assets/audio/bgm1.mp3" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Audio init failed: " << e.what() << std::endl;
    }

    InputService inputService;
    inputService.initialize(window);
    inputService.loadBindingsFromFile("assets/config/input_bindings.json", "keyboard_mouse");

    Scene scene;

    const auto animResult = loadAnimationsFromMetadata("assets/character/animations.json");
    std::shared_ptr<Graphics::Animation> startAnimation;
    if (!animResult.initialState.empty()) {
        auto it = animResult.animations.find(animResult.initialState);
        if (it != animResult.animations.end()) {
            startAnimation = it->second;
        }
    }
    if (!startAnimation && !animResult.animations.empty()) {
        startAnimation = animResult.animations.begin()->second;
    }

    // Ground entity (static)
    Entity &ground = scene.createEntity();
    auto &groundTransform = ground.addComponent<TransformComponent>();
    groundTransform.setPosition(glm::vec2{-800.0f, 0.0f});
    auto groundSprite = std::make_shared<GameObjects::Sprite>(groundTransform.getTransform().Position,
                                                              glm::vec2{2000.0f, 80.0f},
                                                              glm::vec3{0.30f, 0.55f, 0.32f});
    ground.addComponent<SpriteComponent>(groundSprite.get(), -1);
    ground.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, 0.0f);
    auto groundBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
    groundBody->setTransform(&groundTransform.getTransform());
    ground.addComponent<RigidBodyComponent>(std::move(groundBody));

    // Player entity
    Entity &player = scene.createEntity();
    auto &playerTransform = player.addComponent<TransformComponent>();
    playerTransform.setPosition(glm::vec2{100.0f, 150.0f});
    auto playerSprite = std::make_shared<GameObjects::Sprite>(nullptr, playerTransform.getTransform().Position, glm::vec2{128.0f, 128.0f});
    auto playerAnimator = std::make_shared<Graphics::Animator>(playerSprite);
    if (startAnimation) {
        playerAnimator->play(startAnimation);
    }
    player.addComponent<SpriteComponent>(playerSprite.get(), 0);
    player.addComponent<AnimatorComponent>(playerAnimator.get());
    auto &playerCollider = player.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, -12.0f);
    auto playerBody = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
    playerBody->setLinearDamping(6.0f);
    playerBody->setTransform(&playerTransform.getTransform());
    auto &playerRb = player.addComponent<RigidBodyComponent>(std::move(playerBody));
    player.addComponent<PlayerMover>(
        inputService,
        &playerRb,
        PhysicsUnits::toUnits(2.5f),
        PhysicsUnits::toUnits(4.8f),
        groundTransform.getTransform().Position.y + PhysicsUnits::toUnits(0.8f));
    playerCollider.ensureCollider(player);

    camera.setTarget(&playerTransform.getTransform());
    camera.setWorldBounds(glm::vec4{-1000.0f, -200.0f, 2000.0f, 800.0f});
//    particleRenderer.setBorder({0.0f, 0.0f, 0.0f, 1.0f}, 0.00f); // black, thin outline

    // Particle effects setup
    ParticleSystem particleSystem;
    ParticleEmitter* electricEmitter = nullptr;
    try {
        auto effects = ParticleEffectLoader::loadFromFile("assets/config/particles.json");
        auto it = std::find_if(effects.begin(), effects.end(), [](const auto& e){ return e.name == "blue_fire"; });
        if (it == effects.end()) {
            it = std::find_if(effects.begin(), effects.end(), [](const auto& e){ return e.name == "blue_electron"; });
        }
        if (it != effects.end()) {
            electricEmitter = particleSystem.createEmitter(it->maxParticles, it->config);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load particle effects: " << ex.what() << std::endl;
    }
    if (!electricEmitter) {
        // Fallback in case loading failed; keeps effect alive while player exists.
        ParticleEmitterConfig fallback{};
        fallback.spawnRate = 180.0f;
        fallback.burstCount = 0;
        fallback.minLifeTime = 0.6f;
        fallback.maxLifeTime = 1.6f;
        fallback.minSpeed = 40.0f;
        fallback.maxSpeed = 110.0f;
        fallback.direction = 1.57f;
        fallback.spread = 2.6f;
        fallback.minSize = 14.0f;
        fallback.maxSize = 32.0f;
        fallback.startColor = {0.35f, 0.9f, 1.8f, 0.9f};
        fallback.endColor = {0.05f, 0.35f, 1.0f, 0.0f};
        fallback.gravity = {0.0f, 80.0f};
        fallback.drag = 0.35f;
        electricEmitter = particleSystem.createEmitter(260, fallback);
    }
        if (electricEmitter) {
            electricEmitter->setPosition(playerTransform.getTransform().Position);
            electricEmitter->setTarget(playerTransform.getTransform().Position);
            electricEmitter->burst(electricEmitter->getConfig().burstCount);
        }

    // Background layered parallax to make camera motion obvious.
    Entity &background = scene.createEntity();
    auto &bgTransform = background.addComponent<TransformComponent>();
    bgTransform.setPosition(glm::vec2{-1600.0f, -400.0f});
    auto bgTex = Managers::TextureManager::loadTexture("assets/BG/bg_sky.png");
    auto bgSprite = std::make_shared<GameObjects::Sprite>(bgTex,
                                                          bgTransform.getTransform().Position,
                                                          glm::vec2{3200.0f, 1800.0f});
    background.addComponent<SpriteComponent>(bgSprite.get(), -10);

    // Midground band for parallax contrast (solid color).
    Entity &midground = scene.createEntity();
    auto &midTransform = midground.addComponent<TransformComponent>();
    midTransform.setPosition(glm::vec2{-1600.0f, -100.0f});
    auto midTex = Managers::TextureManager::loadTexture("assets/BG/bg_forest.png");
    auto midSprite = std::make_shared<GameObjects::Sprite>(midTex,
                                                           midTransform.getTransform().Position,
                                                           glm::vec2{3200.0f, 1200.0f});
    midground.addComponent<SpriteComponent>(midSprite.get(), -9);

    double lastTime = glfwGetTime();
    std::size_t frameCount = 0;
    auto lastStatTime = std::chrono::steady_clock::now();
    double fps = 0.0;
    double avgMs = 0.0;
#ifdef _WIN32
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    const auto cpuCount = static_cast<double>(sysInfo.dwNumberOfProcessors == 0 ? 1 : sysInfo.dwNumberOfProcessors);
    auto fileTimeToSec = [](const FILETIME& ft) {
        ULARGE_INTEGER uli{};
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return static_cast<double>(uli.QuadPart) * 1e-7;
    };
    FILETIME prevKernel{}, prevUser{}, prevDummy{};
    GetProcessTimes(GetCurrentProcess(), &prevDummy, &prevDummy, &prevKernel, &prevUser);
#endif
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        inputService.update();

        double currentTime = glfwGetTime();
        auto deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Pull and resolve input for this frame so components can read getActionEvents().
        inputService.pollEvents();

        // Update listener at player position.
        {
            const auto& pos2d = playerTransform.getTransform().Position;
            audio.setListener(glm::vec3{pos2d.x, pos2d.y, 0.0f}, glm::vec3{0.0f, 0.0f, -1.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
        }

        // Fire audio triggers based on input events.
//        for (const auto& evt : inputService.getActionEvents()) {
//            const bool pressed = evt.eventType == InputEventType::ButtonPressed;
//            if (pressed && evt.actionName == "Jump") {
//                const auto& pos2d = playerTransform.getTransform().Position;
//                glm::vec3 pos3d{pos2d.x, pos2d.y, 0.0f};
//                audio.triggerSfx("sfx_jump", &pos3d);
//            }
//        }
        audio.update();

        scene.updateWorld(deltaTime, camera, renderer);

        // Update particle emitter to follow player
        if (electricEmitter) {
            electricEmitter->setPosition(playerTransform.getTransform().Position);
            electricEmitter->setTarget(playerTransform.getTransform().Position);
        }
        particleSystem.update(deltaTime);

        // Draw particles after scene render
//        particleRenderer.begin(camera.getViewProjection());
//        particleSystem.render(particleRenderer);
//        particleRenderer.end();

#ifdef _WIN32
        ++frameCount;
        auto now = std::chrono::steady_clock::now();
        const double statElapsed = std::chrono::duration<double>(now - lastStatTime).count();
        if (statElapsed >= 1.0) {
            fps = static_cast<double>(frameCount) / statElapsed;
            avgMs = (statElapsed * 1000.0) / static_cast<double>(frameCount > 0 ? frameCount : 1);
            FILETIME curKernel{}, curUser{}, dummy{};
            GetProcessTimes(GetCurrentProcess(), &dummy, &dummy, &curKernel, &curUser);
            const double kernelDelta = fileTimeToSec(curKernel) - fileTimeToSec(prevKernel);
            const double userDelta = fileTimeToSec(curUser) - fileTimeToSec(prevUser);
            const double cpuPercent = ((kernelDelta + userDelta) / statElapsed) * (100.0 / cpuCount);
            prevKernel = curKernel;
            prevUser = curUser;
            std::string title = "GL2D | FPS: " + std::to_string(static_cast<int>(fps))
                                + " | avg ms: " + std::to_string(static_cast<int>(avgMs))
                                + " | CPU: " + std::to_string(static_cast<int>(cpuPercent)) + "% | GPU: N/A";
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastStatTime = now;
        }
#endif

        glfwSwapBuffers(window);
    }

    audio.shutdown();
    glfwTerminate();
    return 0;
}
