#include "Game.hpp"
#include "InputController.hpp"
#include "Player.hpp"
#include "SceneBuilder.hpp"
#include "InputSystem/InputService.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/RenderSystem.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "Managers/TextureManager.hpp"
#include "Exceptions/Gl2DException.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Components/SpriteRender.hpp"
#include "ECS/Components/ParallaxLayer2D.hpp"
#include "FeelingsSystem/FeelingsLoader.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifndef GL2D_ENGINE_SHADER_DIR
#define GL2D_ENGINE_SHADER_DIR "Shaders"
#endif

#ifndef DEMO_ASSETS_DIR
#define DEMO_ASSETS_DIR "assets"
#endif

namespace {

void handleFramebufferSize(GLFWwindow *window, int width, int height) {
    if (auto *game = static_cast<Game *>(glfwGetWindowUserPointer(window))) {
        game->queueFramebufferSize(width, height);
    }
}

struct BackgroundLayerDefinition {
    std::string file;
    float parallax{0.0f};
    int depth{0};
    glm::vec2 offset{0.0f};
    Rendering::RenderLayer layer{Rendering::RenderLayer::BackgroundMid};
};

struct BackgroundChapterDefinition {
    std::string folder;
    std::string baseFile;
    glm::vec2 baseOffset{0.0f};
    float baseParallax{0.0f};
    int baseDepth{0};
    Rendering::RenderLayer baseLayer{Rendering::RenderLayer::BackgroundFar};
    std::vector<BackgroundLayerDefinition> layers;
};

const std::vector<BackgroundChapterDefinition> kBackgroundChapters{
    {
        "BG_01",
        "BG_01.png",
        glm::vec2{0.0f, -20.0f},
        0.1f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.1f, -49, {0.0f, -40.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.35f, -48, {0.0f, -30.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.6f, -47, {0.0f, -20.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.85f, -46, {0.0f, -10.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 1.0f, -45, {0.0f, -5.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.2f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_02",
        "BG_02.png",
        glm::vec2{0.0f, -18.0f},
        0.1f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.1f, -49, {0.0f, -40.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.35f, -48, {0.0f, -28.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.6f, -47, {0.0f, -18.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.85f, -46, {0.0f, -8.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 1.0f, -45, {0.0f, -4.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.2f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_03",
        "BG_03.png",
        glm::vec2{0.0f, -22.0f},
        0.1f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.1f, -49, {0.0f, -38.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.35f, -48, {0.0f, -27.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.6f, -47, {0.0f, -17.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.85f, -46, {0.0f, -9.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 1.0f, -45, {0.0f, -5.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.2f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_04",
        "BG_04.png",
        glm::vec2{0.0f, -15.0f},
        0.1f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.1f, -49, {0.0f, -35.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.35f, -48, {0.0f, -25.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.6f, -47, {0.0f, -15.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.85f, -46, {0.0f, -7.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 1.0f, -45, {0.0f, -3.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.2f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    }
};

// Each backdrop chapter blends the scene into one of the authored emotional
// states (see design/00-Overview.md): grade, camera, lighting, and movement
// shift together through the FeelingsSystem.
constexpr std::array<const char*, 4> kChapterMoods{
    "still", "wonder", "grief", "fear"};

float computeBottomAnchorOffsetY(const Camera& camera) {
    const glm::vec4 viewBounds = camera.getViewBounds(/*paddingFactor=*/0.0f);
    const float halfViewY = (viewBounds.w - viewBounds.y) * 0.5f;
    const float padding = std::min(120.0f, halfViewY * 0.8f);
    return halfViewY - padding;
}

} // namespace

Game::Game() = default;

Game::~Game() {
    exit();
}

glm::vec2 Game::cameraCenter() const {
    if (m_camera) {
        return m_camera->getTransfrom().Position;
    }
    return glm::vec2{0.0f};
}

void Game::clearBackgroundChapter() {
    auto& registry = m_scene.registry();
    for (ECS::Entity entity : m_backgroundEntities) {
        registry.destroy(entity);
    }
    m_backgroundEntities.clear();
}

void Game::loadBackgroundChapter(size_t index) {
    if (kBackgroundChapters.empty()) {
        return;
    }
    m_currentBackgroundChapter = index % kBackgroundChapters.size();
    const auto& chapter = kBackgroundChapters[m_currentBackgroundChapter];
    clearBackgroundChapter();
    const glm::vec2 centerPos = cameraCenter();
    const glm::vec4 viewBounds = m_camera ? m_camera->getViewBounds(/*paddingFactor=*/0.0f)
                                          : glm::vec4{0.0f, 0.0f, WORLD_WIDTH, WORLD_HEIGHT};
    const glm::vec2 viewSize{viewBounds.z - viewBounds.x, viewBounds.w - viewBounds.y};
#ifdef DEMO_BACKGROUND_DIR
    std::filesystem::path baseDir(DEMO_BACKGROUND_DIR);
    if (baseDir.empty()) {
        baseDir = std::filesystem::path("Demos/The Lost Heroin/assets/PNG");
    }
#else
    std::filesystem::path baseDir("Demos/The Lost Heroin/assets/PNG");
#endif
    if (!baseDir.is_absolute()) {
        baseDir = std::filesystem::absolute(baseDir);
    }
    baseDir /= chapter.folder;
    if (!std::filesystem::exists(baseDir)) {
        throw Engine::GL2DException("Background assets missing: " + baseDir.string());
    }
    const auto spawnLayer = [&](const std::filesystem::path& relPath,
                                 float parallax,
                                 int depth,
                                 const glm::vec2& offset,
                                 Rendering::RenderLayer renderLayer) {
        const auto fullPath = (baseDir / relPath).lexically_normal();
        if (!std::filesystem::exists(fullPath)) {
            throw Engine::GL2DException("Background layer missing: " + fullPath.string());
        }
        std::shared_ptr<GameObjects::Texture> texture =
            Managers::TextureManager::loadTexture(fullPath.string());
        if (!texture) {
            throw Engine::GL2DException("Failed to load background texture: " + fullPath.string());
        }
        const glm::vec2 size{static_cast<float>(texture->getWidth()),
                             static_cast<float>(texture->getHeight())};
        auto sprite = std::make_shared<GameObjects::Sprite>(texture, glm::vec2{0.0f}, size);
        // Foreground layers read as an atmospheric scrim; tint alpha keeps the
        // shared sprite resource clean instead of mutating its color.
        const glm::vec4 tint = (renderLayer == Rendering::RenderLayer::Foreground)
            ? glm::vec4{1.0f, 1.0f, 1.0f, 0.4f} : glm::vec4{1.0f};
        const float scaleX = size.x > 0.0f ? (viewSize.x / size.x) : 1.0f;
        const float scaleY = size.y > 0.0f ? (viewSize.y / size.y) : 1.0f;
        const float scale = std::max(scaleX, scaleY);
        const float tileWidth = size.x * scale;
        const int tileCount = std::max(1, static_cast<int>(std::ceil(viewSize.x / std::max(tileWidth, 1.0f)))) + 2;
        const glm::vec2 basePos{viewBounds.x + offset.x,
                                viewBounds.y + offset.y};

        auto& registry = m_scene.registry();
        for (int i = 0; i < tileCount; ++i) {
            const ECS::Entity entity = registry.create();
            m_backgroundEntities.push_back(entity);
            auto& transform = registry.emplace<ECS::Transform2D>(entity);
            transform.scale = glm::vec2{scale, scale};
            transform.position = basePos + glm::vec2{tileWidth * static_cast<float>(i), 0.0f};
            auto& renderable = registry.emplace<ECS::SpriteRender>(
                entity, sprite, static_cast<int>(renderLayer), depth);
            renderable.tint = tint;
            auto& layerComponent = registry.emplace<ECS::ParallaxLayer2D>(entity);
            layerComponent.factor = glm::vec2{parallax};
            layerComponent.basePosition = basePos;
            layerComponent.baseCameraCenter = centerPos;
            layerComponent.tileWidth = tileWidth;
            layerComponent.tileIndex = i;
            layerComponent.tileCount = tileCount;
        }
    };
    if (!chapter.baseFile.empty()) {
        spawnLayer(chapter.baseFile,
                   chapter.baseParallax,
                   chapter.baseDepth,
                   chapter.baseOffset,
                   chapter.baseLayer);
    }

    for (const auto& layer : chapter.layers) {
        spawnLayer(std::filesystem::path("Layers") / layer.file,
                   layer.parallax,
                   layer.depth,
                   layer.offset,
                   layer.layer);
    }
    applyChapterMood(m_currentBackgroundChapter);
}

void Game::loadFeelings() {
    const auto path =
        std::filesystem::path(DEMO_ASSETS_DIR) / "config/feelings.json";
    m_feelingsController =
        std::make_unique<FeelingsSystem::FeelingsController>(m_scene.feelings());
    m_feelingsController->setDefinitions(
        FeelingsSystem::FeelingsLoader::loadMap(path.string()), "still");
}

void Game::applyChapterMood(size_t chapterIndex) {
    if (!m_feelingsController) {
        return;
    }
    const char* mood = kChapterMoods[chapterIndex % kChapterMoods.size()];
    if (!m_feelingsController->setFeeling(mood)) {
        std::cerr << "Chapter mood is not defined in feelings.json: " << mood
                  << std::endl;
    }
}

void Game::requestChapter(size_t index) {
    if (kBackgroundChapters.empty()) {
        return;
    }
    const size_t resolved = index % kBackgroundChapters.size();
    if (resolved == m_currentBackgroundChapter && !m_backgroundEntities.empty()) {
        return;
    }
    m_currentBackgroundChapter = resolved;
    m_needsBackgroundReload = true;
}

void Game::spawnChapterTriggers() {
    if (m_chapterTriggersSpawned || kBackgroundChapters.empty()) {
        return;
    }
    m_chapterTriggersSpawned = true;
    // Full-height gate volumes spaced along the world. Crossing one advances the
    // environmental-storytelling chapter, cycling through the authored set. This
    // exercises the engine's trigger pipeline rather than polling the player's
    // position each frame.
    constexpr float kGateSpacing = 2500.0f;
    constexpr float kWorldReach = 10000.0f;
    constexpr float kGateHalfWidth = 40.0f;
    size_t gateOrdinal = 0;
    for (float x = -kWorldReach + kGateSpacing; x < kWorldReach; x += kGateSpacing) {
        // x = 0 is the starting chapter; gates to either side step forward.
        if (std::abs(x) < 1.0f) {
            continue;
        }
        const size_t chapter =
            (++gateOrdinal) % kBackgroundChapters.size();
        Entity& gate = m_scene.createEntity();
        auto& transform = gate.addComponent<TransformComponent>();
        transform.setPosition(glm::vec2{x, 0.0f});
        auto& collider = gate.addComponent<ColliderComponent>(
            std::make_unique<AABBCollider>(
                glm::vec2{-kGateHalfWidth, 0.0f},
                glm::vec2{kGateHalfWidth, WORLD_HEIGHT}));
        collider.setTrigger(true);
        collider.setOnTriggerEnter(
            [this, chapter](Entity&, Entity&) { requestChapter(chapter); });

        // Chapter thresholds get a slower, heavier camera: crossing into a
        // new mood should feel like walking through a doorway.
        CameraFramingRegion threshold{};
        threshold.bounds = {x - 600.0f, 0.0f, x + 600.0f, WORLD_HEIGHT};
        threshold.blendMargin = 250.0f;
        threshold.damping = 3.5f;
        m_cameraDirector.addRegion(threshold);
    }
}
void Game::setup() {
    m_window = std::make_unique<Graphics::Window>(1280, 720, "The Lost Heroin");
    glfwSetWindowUserPointer(m_window->getNativeHandle(), this);
    m_window->setResizeCallback(handleFramebufferSize);

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_window->getNativeHandle(), &fbWidth, &fbHeight);
    m_camera = std::make_unique<Camera>(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    m_camera->setFollowMode(CameraFollowMode::DeadZone);
    m_camera->setDeadZoneSize(glm::vec2{WORLD_WIDTH * 0.15f, WORLD_HEIGHT * 0.35f});
    // Matches the tiled ground extent in SceneBuilder so the camera never
    // frames space beyond the walkable world.
    m_camera->setWorldBounds(glm::vec4{-10000.0f, 0.0f, 10000.0f, WORLD_HEIGHT});
    m_camera->setDamping(6.0f);
    m_camera->setFollowDelay(0.045f);
    m_camera->setLookAheadMultiplier(0.12f);
    m_camera->setLookAheadLimits({140.0f, 65.0f});
    m_camera->setLookAheadSmoothing(7.0f);
    m_camera->setShakeLimits({30.0f, 18.0f}, 1.5f);
    updateCameraViewport(fbWidth, fbHeight);

    m_renderer = std::make_unique<Rendering::Renderer>(
            GL2D_ENGINE_SHADER_DIR "/vertex.vert",
            GL2D_ENGINE_SHADER_DIR "/fragment.frag");
    loadFeelings();
    if (m_needsBackgroundReload) {
        loadBackgroundChapter(m_currentBackgroundChapter);
        m_needsBackgroundReload = false;
    } else {
        loadBackgroundChapter(0);
    }
    m_inputService = std::make_unique<InputService>();
    m_inputService->initialize(m_window->getNativeHandle());
    m_inputService->loadBindingsFromFile("assets/config/input_bindings.json", "keyboard_mouse");
    m_inputController = std::make_unique<InputController>(m_inputService.get());
    m_sceneBuilder = std::make_unique<SceneBuilder>();

    m_scene.setClearColor(m_clearColor);

    if (m_sceneBuilder) {
        const glm::vec2 worldSize{WORLD_WIDTH, WORLD_HEIGHT};
        const SceneBuilder::BuildResult build = m_sceneBuilder->build(
            m_scene, m_inputService.get(), worldSize, m_spriteSize);
        m_player = build.player;
        m_groundSprite = build.groundSprite;
    }
    spawnChapterTriggers();
    if (m_camera && m_player) {
        if (auto *transform = m_player->getTransformComponent()) {
            updateCameraTargetOffset();
            const float offsetY = computeBottomAnchorOffsetY(*m_camera);
            m_camera->getTransfrom().setPos(transform->getTransform().Position + glm::vec2{0.0f, offsetY});
        }
    }

    // Opening shot: hold input while letterbox bars slide in, the camera
    // settles from a wide establishing zoom to gameplay framing, and the bars
    // release. Zoom reads the director's baseline each frame so window
    // resizes mid-cutscene stay correct.
    auto& post = m_scene.postProcess();
    m_openingTimeline
        .blockInput(0.0, 3.2)
        .tween(0.0, 0.4, Engine::TimelineEase::EaseOut,
               [&post](float alpha) { post.letterboxAmount = 0.12f * alpha; })
        .tween(0.0, 3.2, Engine::TimelineEase::SmoothStep,
               [this](float alpha) {
                   const float base = m_cameraDirector.baseline().zoom;
                   m_camera->setZoom(base * (0.72f + 0.28f * alpha));
               })
        .tween(3.0, 3.6, Engine::TimelineEase::EaseIn,
               [&post](float alpha) {
                   post.letterboxAmount = 0.12f * (1.0f - alpha);
               });
    m_openingTimeline.play();

    updateProjection();
}



void Game::update() {
    if (!m_window || !m_renderer || !m_player || !m_camera) {
        return;
    }

    auto lastTime = std::chrono::steady_clock::now();
    while (!m_window->shouldClose()) {
        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(now - lastTime).count() * m_playbackSpeed;
        lastTime = now;

        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(m_window->getNativeHandle(), &fbWidth, &fbHeight);
        if (fbWidth > 0 && fbHeight > 0 && m_camera) {
            const glm::vec2 viewportSize = m_camera->getViewportSize();
            if (std::abs(viewportSize.x - static_cast<float>(fbWidth)) > 0.5f ||
                std::abs(viewportSize.y - static_cast<float>(fbHeight)) > 0.5f) {
                updateCameraViewport(fbWidth, fbHeight);
            }
        }

        glfwPollEvents();
        // Poll instead of a key callback: InputService owns the GLFW key
        // callback, so a second callback registration would be overwritten.
        if (glfwGetKey(m_window->getNativeHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(m_window->getNativeHandle(), GLFW_TRUE);
        }
        processPendingFramebufferResize();
        if (m_needsBackgroundReload && m_renderer) {
            loadBackgroundChapter(m_currentBackgroundChapter);
            m_needsBackgroundReload = false;
        }
        if (m_inputController && !m_openingTimeline.inputBlocked()) {
            m_inputController->update();
        }

        const glm::vec2 focus = m_player->getTransformComponent()
            ? m_player->getTransformComponent()->getTransform().Position
            : cameraCenter();
        // Order matters: the director restores framing every frame; an active
        // timeline may then override zoom for authored shots.
        m_cameraDirector.apply(*m_camera, focus);
        m_openingTimeline.update(deltaTime);

        m_scene.advance(deltaTime);
        m_camera->applyFeeling(m_scene.feelings().getSnapshot());
        m_camera->update(deltaTime);
        m_cameraDirector.constrain(*m_camera, focus);
        // Parallax is now a render-time presentation pass inside RenderSystem
        // (ECS ParallaxSystem2D), so no per-frame demo bookkeeping is needed.
        RenderSystem::renderScene(m_scene, *m_camera, *m_renderer);

        m_window->swapBuffers();
    }
}

void Game::exit() {
    clearBackgroundChapter();
    m_scene.clear();
    m_player = nullptr;
    m_renderer.reset();
    m_inputController.reset();
    m_inputService.reset();
    m_sceneBuilder.reset();
    m_camera.reset();
    m_window.reset();
}

void Game::updateProjection() {
    if (m_camera) {
        const glm::vec2 viewport = m_camera->getViewportSize();
        const int width = std::max(1, static_cast<int>(std::round(viewport.x)));
        const int height = std::max(1, static_cast<int>(std::round(viewport.y)));
        glViewport(0, 0, width, height);
        m_projection = m_camera->getViewProjection();
        return;
    }
    if (!m_window) {
        return;
    }
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_window->getNativeHandle(), &width, &height);
    width = std::max(width, 1);
    height = std::max(height, 1);
    glViewport(0, 0, width, height);
    m_projection = glm::ortho(0.0f, WORLD_WIDTH, 0.0f, WORLD_HEIGHT, -1.0f, 1.0f);
}

void Game::updateCameraViewport(int width, int height) {
    if (m_camera) {
        const float safeWidth = static_cast<float>(std::max(1, width));
        const float safeHeight = static_cast<float>(std::max(1, height));
        m_camera->setViewportSize(safeWidth, safeHeight);
        const float zoomX = safeWidth / WORLD_WIDTH;
        const float zoomY = safeHeight / WORLD_HEIGHT;
        const float zoom = std::max(zoomX, zoomY);
        m_camera->setZoom(zoom);
        // The director owns zoom/damping/look-ahead per frame; keep its
        // baseline in sync with the viewport-derived framing.
        m_cameraDirector.setBaseline({zoom, 6.0f, 0.12f});
    }
    updateCameraTargetOffset();
    updateProjection();
    m_needsBackgroundReload = true;
}

void Game::updateCameraTargetOffset() {
    if (!m_camera || !m_player) {
        return;
    }
    auto *transform = m_player->getTransformComponent();
    if (!transform) {
        return;
    }
    const float offsetY = computeBottomAnchorOffsetY(*m_camera);
    m_camera->setTarget(&transform->getTransform(), glm::vec2{0.0f, offsetY});
}

void Game::queueFramebufferSize(int width, int height) {
    m_pendingFramebufferSize = {width, height};
    m_hasPendingFramebufferSize = true;
}

void Game::processPendingFramebufferResize() {
    if (!m_hasPendingFramebufferSize) {
        return;
    }
    updateCameraViewport(m_pendingFramebufferSize.x, m_pendingFramebufferSize.y);
    m_hasPendingFramebufferSize = false;
}
