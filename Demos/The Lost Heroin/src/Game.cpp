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

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#ifndef GL2D_ENGINE_SHADER_DIR
#define GL2D_ENGINE_SHADER_DIR "Shaders"
#endif

namespace {

void requestCloseOnEscape(GLFWwindow *window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

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
        0.3f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.3f, -49, {0.0f, -40.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.6f, -48, {0.0f, -30.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.9f, -47, {0.0f, -20.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.9f, -46, {0.0f, -10.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 0.9f, -45, {0.0f, -5.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.05f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_02",
        "BG_02.png",
        glm::vec2{0.0f, -18.0f},
        0.3f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.3f, -49, {0.0f, -40.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.6f, -48, {0.0f, -28.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.9f, -47, {0.0f, -18.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.9f, -46, {0.0f, -8.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 0.9f, -45, {0.0f, -4.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.05f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_03",
        "BG_03.png",
        glm::vec2{0.0f, -22.0f},
        0.3f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.3f, -49, {0.0f, -38.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.6f, -48, {0.0f, -27.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.9f, -47, {0.0f, -17.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.9f, -46, {0.0f, -9.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 0.9f, -45, {0.0f, -5.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.05f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    },
    {
        "BG_04",
        "BG_04.png",
        glm::vec2{0.0f, -15.0f},
        0.3f,
        -50,
        Rendering::RenderLayer::BackgroundFar,
        {
            {"Sky.png", 0.3f, -49, {0.0f, -35.0f}, Rendering::RenderLayer::BackgroundFar},
            {"BG.png", 0.6f, -48, {0.0f, -25.0f}, Rendering::RenderLayer::BackgroundMid},
            {"Middle.png", 0.9f, -47, {0.0f, -15.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_01.png", 0.9f, -46, {0.0f, -7.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Ground_02.png", 0.9f, -45, {0.0f, -3.0f}, Rendering::RenderLayer::BackgroundNear},
            {"Foreground.png", 1.05f, -44, {0.0f, 0.0f}, Rendering::RenderLayer::Foreground}
        }
    }
};

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
    for (Entity* entity : m_backgroundEntities) {
        if (entity) {
            m_scene.destroyEntity(*entity);
        }
    }
    m_backgroundEntities.clear();
    m_backgroundLayers.clear();
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
        if (renderLayer == Rendering::RenderLayer::Foreground) {
            sprite->setColor(glm::vec4{1.0f, 1.0f, 1.0f, 0.4f});
        }
        const float scaleX = size.x > 0.0f ? (viewSize.x / size.x) : 1.0f;
        const float scaleY = size.y > 0.0f ? (viewSize.y / size.y) : 1.0f;
        const float scale = std::max(scaleX, scaleY);
        const float tileWidth = size.x * scale;
        const int tileCount = std::max(1, static_cast<int>(std::ceil(viewSize.x / std::max(tileWidth, 1.0f)))) + 2;
        const glm::vec2 basePos{viewBounds.x + offset.x,
                                viewBounds.y + offset.y};

        BackgroundLayerInstance instance{};
        instance.basePosition = basePos;
        instance.parallax = parallax;
        instance.baseCenter = centerPos;
        instance.tileWidth = tileWidth;
        instance.tileCount = tileCount;

        for (int i = 0; i < tileCount; ++i) {
            auto& entity = m_scene.createEntity();
            m_backgroundEntities.push_back(&entity);
            auto& transform = entity.addComponent<TransformComponent>();
            transform.setScale(glm::vec2{scale, scale});
            transform.setPosition(basePos + glm::vec2{tileWidth * static_cast<float>(i), 0.0f});
            entity.addComponent<SpriteComponent>(sprite, depth,
                                                 static_cast<int>(renderLayer));
            instance.transforms.push_back(&transform);
        }

        m_backgroundLayers.push_back(std::move(instance));
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
}

void Game::updateBackgroundParallax() {
    if (!m_camera) {
        return;
    }
    const glm::vec2 currentCenter = cameraCenter();
    for (auto& layer : m_backgroundLayers) {
        const size_t count = layer.transforms.size();
        if (count == 0 || layer.tileWidth <= 0.0f) {
            continue;
        }
        const glm::vec2 delta = currentCenter - layer.baseCenter;
        const glm::vec2 offset = delta * layer.parallax;
        float shift = std::fmod(offset.x, layer.tileWidth);
        if (shift < 0.0f) {
            shift += layer.tileWidth;
        }
        const float startX = layer.basePosition.x + shift - layer.tileWidth;
        const float y = layer.basePosition.y + offset.y;
        for (size_t i = 0; i < count; ++i) {
            layer.transforms[i]->setPosition(
                glm::vec2{startX + layer.tileWidth * static_cast<float>(i), y});
        }
    }
}
void Game::setup() {
    m_window = std::make_unique<Graphics::Window>(1280, 720, "The Lost Heroin");
    glfwSetWindowUserPointer(m_window->getNativeHandle(), this);
    m_window->setResizeCallback(handleFramebufferSize);
    glfwSetKeyCallback(m_window->getNativeHandle(), requestCloseOnEscape);

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(m_window->getNativeHandle(), &fbWidth, &fbHeight);
    m_camera = std::make_unique<Camera>(static_cast<float>(fbWidth), static_cast<float>(fbHeight));
    m_camera->setFollowMode(CameraFollowMode::DeadZone);
    m_camera->setDeadZoneSize(glm::vec2{WORLD_WIDTH * 0.15f, WORLD_HEIGHT * 0.35f});
    m_camera->setWorldBounds(glm::vec4{-100000.0f, 0.0f, 100000.0f, WORLD_HEIGHT});
    m_camera->setDamping(6.0f);
    updateCameraViewport(fbWidth, fbHeight);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit();
        return;
    }

    m_renderer = std::make_unique<Rendering::Renderer>(
            GL2D_ENGINE_SHADER_DIR "/vertex.vert",
            GL2D_ENGINE_SHADER_DIR "/fragment.frag");
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

    if (m_sceneBuilder) {
        const glm::vec2 worldSize{WORLD_WIDTH, WORLD_HEIGHT};
        const SceneBuilder::BuildResult build = m_sceneBuilder->build(
            m_scene, m_inputService.get(), worldSize, m_spriteSize);
        m_player = build.player;
        m_groundSprite = build.groundSprite;
    }
    if (m_camera && m_player) {
        if (auto *transform = m_player->getTransformComponent()) {
            updateCameraTargetOffset();
            const float offsetY = computeBottomAnchorOffsetY(*m_camera);
            m_camera->getTransfrom().setPos(transform->getTransform().Position + glm::vec2{0.0f, offsetY});
        }
    }

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
        processPendingFramebufferResize();
        if (m_needsBackgroundReload && m_renderer) {
            loadBackgroundChapter(m_currentBackgroundChapter);
            m_needsBackgroundReload = false;
        }
        if (m_inputController) {
            m_inputController->update();
        }
        m_scene.update(deltaTime);
        m_camera->update(deltaTime);
        updateBackgroundParallax();
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
        m_camera->setZoom(std::max(zoomX, zoomY));
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
