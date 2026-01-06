#include "Engine/Scene.hpp"
#include "GameObjects/Components/AnimatorComponent.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/GroundSensorComponent.hpp"
#include "GameObjects/Components/SpriteComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/WaterStateComponent.hpp"
#include "GameObjects/Components/WaterVolumeComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "GameObjects/IComponent.hpp"
#include "GameObjects/Sprite.hpp"
#include "GameObjects/Prefabs/PrefabDefinitions.hpp"
#include "GameObjects/Prefabs/RopePrefab.hpp"
#include "Graphics/Animation/Animation.hpp"
#include "Graphics/Animation/Frame.hpp"
#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"
#include "Debug/DebugOverlay.hpp"
#include "Managers/TextureManager.hpp"
#include "Physics/PhysicsUnits.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/ParticleRenderer.hpp"
#include "GameObjects/Components/ControllerComponent.hpp"
#include "Engine/PlayerController.hpp"
#include "Engine/RopeHangComponent.hpp"
#include "Engine/VehicleController.hpp"
#include "Engine/VehicleMountComponent.hpp"
#include "Engine/SwimmingComponent.hpp"
#include "ParticleSystem/ParticleSystem.hpp"
#include "ParticleSystem/ParticleEffectLoader.hpp"
#include "GameObjects/Components/LightingComponent.hpp"
#include "AudioSystem/AudioManager.hpp"
#include "UI/UILoader.hpp"
#include "UI/UIElements.hpp"
#include "UI/UIRenderer.hpp"
#include "FeelingsSystem/FeelingsController.hpp"
#include "FeelingsSystem/FeelingsSystem.hpp"
#include "FeelingsSystem/FeelingsLoader.hpp"
#include "Engine/DialogueSystem.hpp"
#include "Exceptions/Gl2DException.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <cstdint>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
Camera* gActiveCamera = nullptr;
bool gMouseWasDown = false;
bool gEscWasDown = false;

std::filesystem::path resolveMetadataDirectory(const std::string &metadataPath) {
    std::filesystem::path dir = std::filesystem::path(metadataPath).parent_path();
    if (dir.empty()) {
        dir = std::filesystem::current_path();
    }
    if (!dir.is_absolute()) {
        dir = std::filesystem::absolute(dir);
    }
    return dir;
}

std::string resolveAssetPath(const std::filesystem::path &metadataDir, const std::string &assetPath) {
    if (assetPath.empty()) {
        return {};
    }
    std::filesystem::path candidate(assetPath);
    if (candidate.is_absolute()) {
        return candidate.string();
    }
    auto resolved = (metadataDir / candidate).lexically_normal();
    return resolved.string();
}

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
    const auto metadataDir = resolveMetadataDirectory(metadataPath);
    auto fetchTexture = [&](const std::string &path) -> std::shared_ptr<GameObjects::Texture> {
        if (path.empty())
            return nullptr;
        const auto resolvedPath = resolveAssetPath(metadataDir, path);
        auto it = textureCache.find(resolvedPath);
        if (it != textureCache.end()) {
            return it->second;
        }
        auto texture = Managers::TextureManager::loadTexture(resolvedPath);
        textureCache[resolvedPath] = texture;
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

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    GLFWwindow *window = nullptr;
    Audio::AudioManager audio;
    try {
        window = glfwCreateWindow(1280, 720, "GL2D Prototype", nullptr, nullptr);
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
    Prefabs::registerGamePrefabs();
    FeelingsSystem::FeelingsController feelingsController(scene.feelings());
    DialogueSystem dialogue;
    // Load feelings config (if present) and apply a default feeling.
    try {
        const std::string feelingsPath = "assets/config/feelings.json";
        if (std::filesystem::exists(feelingsPath)) {
            auto feelings = FeelingsSystem::FeelingsLoader::loadMap(feelingsPath);
            feelingsController.setDefinitions(std::move(feelings), "default");
        }
    } catch (const std::exception& e) {
        std::cerr << "Feelings load failed: " << e.what() << std::endl;
    }

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

    // Example line to verify dialogue overlay/voice; replace with your own content or remove.
    dialogue.enqueue(DialogueSystem::Line{
        .speaker = "Guide",
        .text = "Welcome! Use A/D or the stick to move, and Space or gamepad A to jump.",
        .audioTriggerId = "",
        .audioPath = "",
        .duckDb = -8.0f,
        .duration = 0.0f});

    // Ground entity (static)
    Entity &ground = scene.createEntity();
    auto &groundTransform = ground.addComponent<TransformComponent>();
    groundTransform.setPosition(glm::vec2{-800.0f, 0.0f});
    auto groundSprite = std::make_shared<GameObjects::Sprite>(groundTransform.getTransform().Position,
                                                              glm::vec2{2000.0f, 80.0f},
                                                              glm::vec3{0.30f, 0.55f, 0.32f});
    const std::string groundNormalPath = "assets/ground_normal.png";
    if (std::filesystem::exists(groundNormalPath)) {
        groundSprite->setNormalTexture(Managers::TextureManager::loadTexture(groundNormalPath));
    }
    ground.addComponent<SpriteComponent>(groundSprite.get(), -1);
    ground.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, 0.0f);
    auto groundBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
    groundBody->setTransform(&groundTransform.getTransform());
    ground.addComponent<RigidBodyComponent>(std::move(groundBody));

    // Simple rectangular water volume with mild current.
    Entity &water = scene.createEntity();
    auto &waterTransform = water.addComponent<TransformComponent>();
    waterTransform.setPosition(glm::vec2{-400.0f, -120.0f});
    auto waterSprite = std::make_shared<GameObjects::Sprite>(waterTransform.getTransform().Position,
                                                             glm::vec2{1200.0f, 220.0f},
                                                             glm::vec3{0.10f, 0.35f, 0.70f});
    water.addComponent<SpriteComponent>(waterSprite.get(), -2);
    auto &waterCollider = water.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, 0.0f);
    waterCollider.setTrigger(true);
    auto &waterVolume = water.addComponent<WaterVolumeComponent>();
    waterVolume.setDensity(1.05f);
    waterVolume.setLinearDrag(10.0f);
    waterVolume.setFlowVelocity(glm::vec2{PhysicsUnits::toUnits(0.6f), 0.0f});

    constexpr uint32_t kMovingPlatformLayer = 1;

    // Player entity
    Entity &player = scene.createEntity();
    auto &playerTransform = player.addComponent<TransformComponent>();
    playerTransform.setPosition(glm::vec2{100.0f, 150.0f});
    auto playerSprite = std::make_shared<GameObjects::Sprite>(nullptr, playerTransform.getTransform().Position,
                                                              glm::vec2{128.0f, 128.0f});
    auto &playerSpriteComp = player.addComponent<SpriteComponent>(playerSprite, 0);
    auto &playerAnimatorComp = player.addComponent<AnimatorComponent>();
    AnimatorComponent* playerAnimatorCompPtr = &playerAnimatorComp;
    playerAnimatorComp.setSprite(playerSpriteComp.sharedSprite());
    if (startAnimation) {
        playerAnimatorComp.play(startAnimation);
    }
    auto &playerCollider = player.addComponent<ColliderComponent>(nullptr, ColliderType::CAPSULE, -12.0f);
    auto &playerSensor = player.addComponent<GroundSensorComponent>();
    playerSensor.setWorldEntities(&scene.getEntities());
    playerSensor.setPlatformLayerMask(1u << kMovingPlatformLayer);
    auto playerBody = std::make_unique<RigidBody>(1.0f, RigidBodyType::DYNAMIC);
    playerBody->setLinearDamping(6.0f);
    playerBody->setTransform(&playerTransform.getTransform());
    auto &playerRb = player.addComponent<RigidBodyComponent>(std::move(playerBody));
    auto controller = std::make_unique<PlayerController>(inputService);
    controller->setMaxMoveSpeed(PhysicsUnits::toUnits(2.0f));
    auto& playerControllerComp = player.addComponent<ControllerComponent>(std::move(controller));
    player.addComponent<RopeHangComponent>(inputService, playerControllerComp, &scene.getEntities());
    player.addComponent<WaterStateComponent>();
    player.addComponent<SwimmingComponent>(inputService);
    playerCollider.ensureCollider(player);

    camera.setTarget(&playerTransform.getTransform());
    camera.setWorldBounds(glm::vec4{-1000.0f, -200.0f, 2000.0f, 800.0f});
//    particleRenderer.setBorder({0.0f, 0.0f, 0.0f, 1.0f}, 0.00f); // black, thin outline

    // Boat entity to exercise buoyancy and boat controller.
    Entity &boat = scene.createEntity();
    auto &boatTransform = boat.addComponent<TransformComponent>();
    boatTransform.setPosition(glm::vec2{-220.0f, 20.0f});
    auto boatSprite = std::make_shared<GameObjects::Sprite>(boatTransform.getTransform().Position,
                                                            glm::vec2{220.0f, 90.0f},
                                                            glm::vec3{0.45f, 0.25f, 0.12f});
    boat.addComponent<SpriteComponent>(boatSprite.get(), -1);
    auto &boatCollider = boat.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, -6.0f);
    boatCollider.setLayer(kMovingPlatformLayer);
    auto boatBody = std::make_unique<RigidBody>(2.6f, RigidBodyType::DYNAMIC);
    boatBody->setLinearDamping(3.0f);
    boatBody->setTransform(&boatTransform.getTransform());
    boat.addComponent<RigidBodyComponent>(std::move(boatBody));
    boat.addComponent<WaterStateComponent>();
    auto boatController = std::make_unique<VehicleController>(inputService);
    boatController->setSeatOffset(glm::vec2{0.0f, boatSprite->getSize().y * 0.25f});
    boat.addComponent<ControllerComponent>(std::move(boatController));
    auto& mountComp = boat.addComponent<VehicleMountComponent>(inputService, &player);
    mountComp.setMountRadius(PhysicsUnits::toUnits(1.2f));
    mountComp.setSeatOffset(glm::vec2{0.0f, boatSprite->getSize().y * 0.25f});
    mountComp.setDebugCamera(&camera);
    auto& boatSensor = boat.addComponent<GroundSensorComponent>();
    boatSensor.setWorldEntities(&scene.getEntities());
    boatCollider.ensureCollider(boat);

    Entity &tree = scene.createEntity();
    auto &treeTransform = tree.addComponent<TransformComponent>();
    treeTransform.setPosition(glm::vec2{-80.0f, 120.0f});
    auto treeSprite = std::make_shared<GameObjects::Sprite>(treeTransform.getTransform().Position,
                                                             glm::vec2{40.0f, 220.0f},
                                                             glm::vec3{0.25f, 0.45f, 0.18f});
    tree.addComponent<SpriteComponent>(treeSprite.get(), -1);
    auto &treeCollider = tree.addComponent<ColliderComponent>(nullptr, ColliderType::AABB, -4.0f);
    auto treeBody = std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC);
    treeBody->setTransform(&treeTransform.getTransform());
    tree.addComponent<RigidBodyComponent>(std::move(treeBody));

    Prefabs::RopePrefabConfig ropeConfig{};
    ropeConfig.anchorPosition =
            treeTransform.getTransform().Position + glm::vec2{0.0f, treeSprite->getSize().y * 0.5f};
    ropeConfig.direction = glm::vec2{0.0f, -1.0f};
    ropeConfig.segmentCount = 14;
    ropeConfig.segmentLength = 18.0f;
    ropeConfig.segmentThickness = 3.5f;
    ropeConfig.segmentSpacing = 0.5f;
    ropeConfig.startAnchor = &tree;
    ropeConfig.startAnchorOffset = glm::vec2{0.0f, treeSprite->getSize().y * 0.5f};
    ropeConfig.lowerLimit = -1.2f;
    ropeConfig.upperLimit = 1.2f;
    ropeConfig.limitStiffness = 13.0f;
    ropeConfig.limitDamping = 2.5f;
    ropeConfig.maxLimitTorque = 28.0f;
    ropeConfig.segmentMass = 0.18f;
    ropeConfig.segmentLinearDamping = 5.0f;
    ropeConfig.segmentAngularDamping = 5.5f;
    ropeConfig.useAnchorEntity = false;
    ropeConfig.maxDrop = ropeConfig.anchorPosition.y;
    ropeConfig.clampDrop = true;
    ropeConfig.endAnchor = &ground;
    ropeConfig.endAnchorOffset = glm::vec2{0.0f, 20.0f};
    Prefabs::RopePrefab::instantiate(scene, ropeConfig);

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
    const std::string bgNormalPath = "assets/BG/bg_sky_normal.png";
    if (std::filesystem::exists(bgNormalPath)) {
        bgSprite->setNormalTexture(Managers::TextureManager::loadTexture(bgNormalPath));
    }
    background.addComponent<SpriteComponent>(bgSprite.get(), -10);

    // Midground band for parallax contrast (solid color).
    Entity &midground = scene.createEntity();
    auto &midTransform = midground.addComponent<TransformComponent>();
    midTransform.setPosition(glm::vec2{-1600.0f, -100.0f});
    auto midTex = Managers::TextureManager::loadTexture("assets/BG/bg_forest.png");
    auto midSprite = std::make_shared<GameObjects::Sprite>(midTex,
                                                           midTransform.getTransform().Position,
                                                           glm::vec2{3200.0f, 1200.0f});
    const std::string midNormalPath = "assets/BG/bg_forest_normal.png";
    if (std::filesystem::exists(midNormalPath)) {
        midSprite->setNormalTexture(Managers::TextureManager::loadTexture(midNormalPath));
    }
    midground.addComponent<SpriteComponent>(midSprite.get(), -9);

    // Sample lights removed; rely on level-defined lights or fallback fills.

    // UI: pause menu screen loaded from JSON; textures resolved lazily.
    std::unordered_map<std::string, std::shared_ptr<GameObjects::Texture>> uiTextures;
    auto uiResolver = [&uiTextures](const std::string& texPath) -> GameObjects::Texture* {
        auto it = uiTextures.find(texPath);
        if (it != uiTextures.end()) {
            return it->second.get();
        }
        auto tex = Managers::TextureManager::loadTexture(texPath);
        uiTextures[texPath] = tex;
        return tex ? tex.get() : nullptr;
    };
    UI::UIScreen pauseScreen{};
    try {
        pauseScreen = UI::UILoader::loadFromFile("assets/config/ui_pause.json", uiResolver);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load pause UI: " << e.what() << std::endl;
    }
    auto findElement = [](const std::vector<std::shared_ptr<UI::UIElement>>& roots,
                          const std::string& targetId) -> std::shared_ptr<UI::UIElement> {
        std::function<std::shared_ptr<UI::UIElement>(const std::shared_ptr<UI::UIElement>&)> dfs;
        dfs = [&](const std::shared_ptr<UI::UIElement>& node) -> std::shared_ptr<UI::UIElement> {
            if (!node) return nullptr;
            if (node->id() == targetId) return node;
            for (const auto& child : node->children()) {
                if (auto found = dfs(child)) return found;
            }
            return nullptr;
        };
        for (const auto& root : roots) {
            if (auto found = dfs(root)) return found;
        }
        return nullptr;
    };
    bool pauseOverlayActive = false;
    if (auto resumeEl = findElement(pauseScreen.roots, "btn_resume")) {
        if (auto resumeBtn = std::dynamic_pointer_cast<UI::Button<>>(resumeEl)) {
            resumeBtn->setCallback([&]() {
                pauseOverlayActive = false;
                scene.setPaused(false);
            });
        }
    }
    if (auto quitEl = findElement(pauseScreen.roots, "btn_quit")) {
        if (auto quitBtn = std::dynamic_pointer_cast<UI::Button<>>(quitEl)) {
            quitBtn->setCallback([&]() { glfwSetWindowShouldClose(window, GLFW_TRUE); });
        }
    }

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

        // Update feelings controller (blending, timers) and apply to subsystems.
//        feelingsController.setTargets(&camera, &renderer, nullptr /*particleRenderer*/, &audio);
//        feelingsController.update(deltaTime * 1000.0f);
        const float effectiveDt = deltaTime * feelingsController.timeScale();

        // Pull and resolve input for this frame so components can read getActionEvents().
        inputService.pollEvents();
        for (const auto& evt : inputService.getActionEvents()) {
            if (evt.eventType == InputEventType::ButtonPressed && evt.actionName == "ToggleDebugGizmos") {
                DebugOverlay::toggle();
            }
        }

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
        dialogue.update(effectiveDt, audio);

        // Toggle pause overlay on ESC press.
        const bool escDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escDown && !gEscWasDown) {
            pauseOverlayActive = !pauseOverlayActive;
            scene.setPaused(pauseOverlayActive);
        }
        gEscWasDown = escDown;

        // Mouse pointer state for UI.
        double cursorX = 0.0, cursorY = 0.0;
        glfwGetCursorPos(window, &cursorX, &cursorY);
        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        UI::UIPointerState pointer{};
        pointer.position = {static_cast<float>(cursorX), static_cast<float>(fbH - cursorY)};
        const bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        pointer.down = mouseDown;
        pointer.pressed = mouseDown && !gMouseWasDown;
        pointer.released = !mouseDown && gMouseWasDown;
        gMouseWasDown = mouseDown;

        if (pauseOverlayActive) {
            pauseScreen.canvasSize = {static_cast<float>(fbW), static_cast<float>(fbH)};
            pauseScreen.update(deltaTime, pointer);
        }

        // Switch player animation based on horizontal speed.
        {
            static std::string currentAnim = startAnimation ? startAnimation->getName() : "";
            constexpr float kPlayerAnimationCrossfade = 0.25f;
            const float vx = playerRb.body() ? playerRb.body()->getVelocity().x : 0.0f;
            const float speed = std::abs(vx);
            const std::string desiredAnim =
                    speed < 5.0f ? "Idle" :
                    speed < 80.0f ? "Walk" : "Run";

            if (desiredAnim != currentAnim) {
                if (auto it = animResult.animations.find(desiredAnim);
                    it != animResult.animations.end() && playerAnimatorCompPtr) {
                    playerAnimatorCompPtr->play(it->second, kPlayerAnimationCrossfade);
                    currentAnim = desiredAnim;
                }
            }
        }

        scene.updateWorld(effectiveDt, camera, renderer);

        // Update particle emitter to follow player
        if (electricEmitter) {
            electricEmitter->setPosition(playerTransform.getTransform().Position);
            electricEmitter->setTarget(playerTransform.getTransform().Position);
        }
        particleSystem.update(effectiveDt);

        // Draw particles after scene render
//        particleRenderer.begin(camera.getViewProjection());
//        particleSystem.render(particleRenderer);
//        particleRenderer.end();

        // Render UI overlays (pause, dialogue) without clearing the scene.
        std::vector<UI::UIRenderCommand> uiCommands;
        if (pauseOverlayActive) {
            const auto commands = pauseScreen.collectRenderCommands();
            uiCommands.insert(uiCommands.end(), commands.begin(), commands.end());
        }
        dialogue.appendCommands(fbW, fbH, uiCommands);
        if (!uiCommands.empty()) {
            UI::UIRenderer::render(uiCommands, fbW, fbH);
        }

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
    } catch (const Engine::GL2DException&) {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return 3;
    } catch (const std::exception& ex) {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return 3;
    } catch (...) {
        if (window) {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
        return 3;
    }
}
