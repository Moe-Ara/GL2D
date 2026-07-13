#include "ECS/Animation/AnimationGraph2D.hpp"
#include "ECS/Components/Animation2D.hpp"
#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Collision2D.hpp"
#include "ECS/Components/Climbable2D.hpp"
#include "ECS/Components/SpriteRender.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Components/Light2D.hpp"
#include "ECS/Components/ParticleEmitter2D.hpp"
#include "ECS/Components/ParticleRender2D.hpp"
#include "Engine/Scene.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "Graphics/Window.hpp"
#include "InputSystem/InputService.hpp"
#include "InputSystem/InputTypes.hpp"
#include "RenderingSystem/RenderLayers.hpp"
#include "RenderingSystem/Renderer.hpp"
#include "RenderingSystem/RenderSystem.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifndef GL2D_ENGINE_SHADER_DIR
#define GL2D_ENGINE_SHADER_DIR "Shaders"
#endif

#ifndef GL2D_INPUT_BINDINGS_PATH
#define GL2D_INPUT_BINDINGS_PATH "assets/config/input_bindings.json"
#endif

namespace {
struct InputState {
    bool left{false};
    bool right{false};
    bool up{false};
    bool down{false};
    float analogAxis{0.0f};
    float analogClimbAxis{0.0f};
};

std::shared_ptr<GameObjects::Sprite> makeSprite(glm::vec2 size, glm::vec4 color) {
    auto sprite = std::make_shared<GameObjects::Sprite>(glm::vec2{0.0f}, size, glm::vec3(color));
    sprite->setColor(color);
    return sprite;
}

ECS::AnimationFrame2D animationFrame(float duration, glm::vec4 tint,
                                     std::string event = {}) {
    ECS::AnimationFrame2D frame{};
    frame.durationSeconds = duration;
    frame.tint = tint;
    frame.event = std::move(event);
    return frame;
}

std::shared_ptr<const ECS::AnimationGraph2D> createPlayerAnimationGraph() {
    using Playback = ECS::AnimationPlayback2D;
    std::vector<ECS::AnimationState2D> states{
        {"Idle", {{animationFrame(0.28f, {0.75f, 0.95f, 1.0f, 1.0f}),
                    animationFrame(0.28f, {1.0f, 1.0f, 1.0f, 1.0f}, "idle_breathe")},
                   Playback::Loop}},
        {"Run", {{animationFrame(0.09f, {0.75f, 1.0f, 0.8f, 1.0f}, "footstep"),
                   animationFrame(0.09f, {1.0f, 0.82f, 0.55f, 1.0f}),
                   animationFrame(0.09f, {0.75f, 1.0f, 0.8f, 1.0f}, "footstep"),
                   animationFrame(0.09f, {0.8f, 0.9f, 1.0f, 1.0f})},
                  Playback::Loop}},
        {"Rise", {{animationFrame(0.12f, {1.0f, 0.9f, 0.35f, 1.0f}, "jump")},
                   Playback::Once}},
        {"Fall", {{animationFrame(0.16f, {0.55f, 0.7f, 1.0f, 1.0f})},
                   Playback::Loop}},
        {"Climb", {{animationFrame(0.12f, {0.55f, 1.4f, 1.1f, 1.0f}),
                     animationFrame(0.12f, {0.35f, 0.9f, 1.8f, 1.0f}, "climb_step")},
                    Playback::Loop}}
    };
    std::vector<ECS::AnimationTransition2D> transitions{
        {"*", "Climb", ECS::AnimationCondition2D::BoolEquals, "climbing", 0.0f, true},
        {"*", "Rise", ECS::AnimationCondition2D::BoolEquals, "rising", 0.0f, true},
        {"*", "Fall", ECS::AnimationCondition2D::BoolEquals, "falling", 0.0f, true},
        {"Idle", "Run", ECS::AnimationCondition2D::BoolEquals, "moving", 0.0f, true},
        {"Run", "Idle", ECS::AnimationCondition2D::BoolEquals, "moving", 0.0f, false},
        {"Rise", "Idle", ECS::AnimationCondition2D::BoolEquals, "grounded", 0.0f, true},
        {"Fall", "Idle", ECS::AnimationCondition2D::BoolEquals, "grounded", 0.0f, true}
    };
    return std::make_shared<const ECS::AnimationGraph2D>(
        std::move(states), std::move(transitions), "Idle");
}

ECS::Entity createClimbable(Scene& scene, glm::vec2 center, glm::vec2 size) {
    auto& registry = scene.registry();
    const ECS::Entity entity = registry.create();
    auto& transform = registry.emplace<ECS::Transform2D>(entity);
    transform.position = center - size * 0.5f;
    auto& collider = registry.emplace<ECS::AabbCollider2D>(entity);
    collider.halfExtents = size * 0.5f;
    collider.offset = size * 0.5f;
    registry.emplace<ECS::Climbable2D>(entity);
    auto sprite = makeSprite(size, {0.2f, 0.75f, 1.35f, 0.65f});
    registry.emplace<ECS::SpriteRender>(entity, ECS::SpriteRender{
        sprite, static_cast<int>(Rendering::RenderLayer::Gameplay), -1, true});
    return entity;
}

ECS::Entity createStaticBox(Scene& scene, glm::vec2 center, glm::vec2 size,
                            glm::vec4 color, int zIndex = 0) {
    auto& registry = scene.registry();
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::Transform2D>(entity).position = center;
    registry.emplace<ECS::AabbCollider2D>(entity).halfExtents = size * 0.5f;
    registry.emplace<ECS::StaticCollider2D>(entity);

    // Sprites use a lower-left origin while collider transforms use centers.
    auto& renderTransform = registry.get<ECS::Transform2D>(entity);
    auto sprite = makeSprite(size, color);
    ECS::SpriteRender renderable{sprite,
        static_cast<int>(Rendering::RenderLayer::Gameplay), zIndex, true};
    registry.emplace<ECS::SpriteRender>(entity, std::move(renderable));
    renderTransform.position = center - size * 0.5f;
    auto& collider = registry.get<ECS::AabbCollider2D>(entity);
    collider.offset = size * 0.5f;
    return entity;
}

void applyInputEvents(InputService& input, ECS::CharacterIntent& intent,
                      InputState& state) {
    for (const ActionEvent& event : input.getActionEvents()) {
        const bool pressed = event.eventType == InputEventType::ButtonPressed;
        const bool released = event.eventType == InputEventType::ButtonReleased;
        if (event.actionName == "MoveLeft") {
            if (pressed) state.left = true;
            if (released) state.left = false;
        } else if (event.actionName == "MoveRight") {
            if (pressed) state.right = true;
            if (released) state.right = false;
        } else if (event.actionName == "MoveHorizontal" &&
                   event.eventType == InputEventType::AxisChanged) {
            state.analogAxis = event.value.x;
        } else if (event.actionName == "MoveUp") {
            if (pressed) state.up = true;
            if (released) state.up = false;
        } else if (event.actionName == "MoveDown") {
            if (pressed) state.down = true;
            if (released) state.down = false;
        } else if (event.actionName == "MoveVertical" &&
                   event.eventType == InputEventType::AxisChanged) {
            state.analogClimbAxis = -event.value.x;
        } else if (event.actionName == "Jump") {
            intent.jumpPressed |= pressed;
            intent.jumpReleased |= released;
            if (pressed) intent.jumpHeld = true;
            if (released) intent.jumpHeld = false;
        }
    }
    intent.moveAxis = std::abs(state.analogAxis) >= 0.1f
        ? state.analogAxis
        : (state.right ? 1.0f : 0.0f) - (state.left ? 1.0f : 0.0f);
    intent.climbAxis = std::abs(state.analogClimbAxis) >= 0.1f
        ? state.analogClimbAxis
        : (state.up ? 1.0f : 0.0f) - (state.down ? 1.0f : 0.0f);
}
}

int main() {
    try {
        Graphics::Window window{1280, 720, "GL2D ECS Platformer"};
        Rendering::Renderer renderer{
            GL2D_ENGINE_SHADER_DIR "/vertex.vert",
            GL2D_ENGINE_SHADER_DIR "/fragment.frag"};
        Camera camera{1280.0f, 720.0f};
        camera.setZoom(1.0f);
        camera.setFollowMode(CameraFollowMode::DeadZone);
        camera.setDeadZoneSize({72.0f, 42.0f});
        camera.setDamping(7.5f);
        camera.setFollowDelay(0.04f);
        camera.setLookAheadMultiplier(0.12f);
        camera.setLookAheadLimits({110.0f, 55.0f});
        camera.setLookAheadSmoothing(8.0f);
        camera.setShakeLimits({24.0f, 16.0f}, 1.5f);

        InputService input;
        input.initialize(window.getNativeHandle());
        input.loadBindingsFromFile(GL2D_INPUT_BINDINGS_PATH, "keyboard_mouse");

        Scene scene;
        scene.setAmbientLight({0.08f, 0.1f, 0.16f});
        scene.postProcess().exposure = 1.05f;
        scene.postProcess().bloomThreshold = 0.85f;
        scene.postProcess().bloomStrength = 0.45f;
        scene.postProcess().vignetteStrength = 0.16f;
        createStaticBox(scene, {0.0f, -20.0f}, {1800.0f, 40.0f},
                        {0.08f, 0.12f, 0.2f, 1.0f});
        createStaticBox(scene, {260.0f, 100.0f}, {260.0f, 30.0f},
                        {0.12f, 0.28f, 0.35f, 1.0f});
        createStaticBox(scene, {-340.0f, 180.0f}, {220.0f, 30.0f},
                        {0.18f, 0.22f, 0.42f, 1.0f});
        createStaticBox(scene, {560.0f, 180.0f}, {30.0f, 400.0f},
                        {0.2f, 0.12f, 0.28f, 1.0f});
        createClimbable(scene, {20.0f, 145.0f}, {48.0f, 290.0f});

        auto& registry = scene.registry();
        const ECS::Entity cyanLight = registry.create();
        registry.emplace<ECS::Transform2D>(cyanLight).position = {-180.0f, 180.0f};
        registry.emplace<ECS::Light2D>(cyanLight,
            ECS::Light2D::point(420.0f, {0.2f, 0.75f, 1.0f}, 1.8f));
        registry.emplace<ECS::LightAnimation2D>(cyanLight).effector = {
            LightEffector::Type::Pulse, 0.12f, 2.2f, 0.0f};

        const ECS::Entity magentaLight = registry.create();
        registry.emplace<ECS::Transform2D>(magentaLight).position = {390.0f, 260.0f};
        registry.emplace<ECS::Light2D>(magentaLight,
            ECS::Light2D::point(360.0f, {1.0f, 0.2f, 0.65f}, 1.5f));

        const ECS::Entity player = registry.create();
        auto& transform = registry.emplace<ECS::Transform2D>(player);
        transform.position = {-100.0f, 70.01f};
        registry.emplace<ECS::CharacterIntent>(player);
        registry.emplace<ECS::CharacterMotorConfig>(player);
        registry.emplace<ECS::CharacterMotorState>(player);
        registry.emplace<ECS::KinematicBody2D>(player);
        registry.emplace<ECS::GroundContact2D>(player).grounded = true;
        registry.emplace<ECS::CharacterCollisionState2D>(player);
        registry.emplace<ECS::ClimbingState2D>(player);
        auto& playerCollider = registry.emplace<ECS::AabbCollider2D>(player);
        playerCollider.halfExtents = {30.0f, 50.0f};
        // Values above 1.0 are intentional HDR emission and feed the bloom pass.
        auto playerSprite = makeSprite({60.0f, 100.0f}, {0.5f, 1.25f, 1.8f, 1.0f});
        playerCollider.offset = {30.0f, 50.0f};
        registry.emplace<ECS::SpriteRender>(player, ECS::SpriteRender{
            playerSprite, static_cast<int>(Rendering::RenderLayer::Gameplay), 10, true});
        registry.emplace<ECS::AnimationParameters2D>(player);
        registry.emplace<ECS::Animator2D>(player).graph = createPlayerAnimationGraph();
        registry.emplace<ECS::AnimationEventQueue2D>(player);
        ParticleEmitterConfig auraConfig{};
        auraConfig.spawnRate = 55.0f;
        auraConfig.minLifeTime = 0.35f;
        auraConfig.maxLifeTime = 0.8f;
        auraConfig.minSpeed = 8.0f;
        auraConfig.maxSpeed = 45.0f;
        auraConfig.direction = 1.5707963f;
        auraConfig.spread = 6.2831853f;
        auraConfig.minSize = 7.0f;
        auraConfig.maxSize = 15.0f;
        auraConfig.endSizeMultiplier = 0.15f;
        auraConfig.startColor = {0.25f, 1.0f, 2.4f, 0.75f};
        auraConfig.endColor = {0.05f, 0.3f, 1.2f, 0.0f};
        auraConfig.gravity = {0.0f, 22.0f};
        auraConfig.drag = 2.0f;
        auraConfig.minAngularVelocity = -2.0f;
        auraConfig.maxAngularVelocity = 2.0f;
        auraConfig.randomSeed = 0x51a7e2u;
        auto& aura = registry.emplace<ECS::ParticleEmitter2D>(
            player, 96, auraConfig);
        registry.emplace<ECS::ParticleRender2D>(player).blendMode =
            Rendering::ParticleBlendMode::Additive;
        aura.localOffset = {30.0f, 52.0f};
        aura.targetOffset = aura.localOffset;
        // Transform is the lower-left render origin; the collider offset places
        // its center in the sprite while physics remains authoritative.
        transform.position = {-130.0f, 0.01f};

        Transform cameraTarget{};
        cameraTarget.setPos(transform.position);
        camera.setTarget(&cameraTarget, {30.0f, 100.0f});
        bool cameraWasGrounded = registry.get<ECS::GroundContact2D>(player).grounded;
        float strongestFallSpeed = 0.0f;

        InputState inputState{};
        auto previousTime = std::chrono::steady_clock::now();
        while (!window.shouldClose()) {
            const auto now = std::chrono::steady_clock::now();
            const float frameDelta = std::chrono::duration<float>(now - previousTime).count();
            previousTime = now;

            window.pollEvents();
            input.update();
            input.pollEvents();
            applyInputEvents(input, registry.get<ECS::CharacterIntent>(player), inputState);
            if (glfwGetKey(window.getNativeHandle(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.getNativeHandle(), GLFW_TRUE);
            }

            scene.advance(frameDelta);
            camera.applyFeeling(scene.feelings().getSnapshot());
            auto& playerTransform = registry.get<ECS::Transform2D>(player);
            const auto& motor = registry.get<ECS::CharacterMotorState>(player);
            const auto& contact = registry.get<ECS::GroundContact2D>(player);
            const auto& body = registry.get<ECS::KinematicBody2D>(player);
            registry.get<ECS::SpriteRender>(player).flipX =
                motor.facingDirection < 0.0f;

            if (!contact.grounded) {
                strongestFallSpeed = std::min(strongestFallSpeed, body.velocity.y);
            } else if (!cameraWasGrounded) {
                const float impact = std::clamp(
                    std::abs(strongestFallSpeed) / 900.0f, 0.0f, 1.0f);
                camera.shake(5.0f + 9.0f * impact, 0.16f + 0.08f * impact,
                             20.0f, playerTransform.position);
                camera.pulseZoom(0.012f + 0.018f * impact, 0.18f);
                strongestFallSpeed = 0.0f;
            }
            cameraWasGrounded = contact.grounded;

            int width = 1;
            int height = 1;
            glfwGetFramebufferSize(window.getNativeHandle(), &width, &height);
            camera.setViewportSize(static_cast<float>(std::max(width, 1)),
                                   static_cast<float>(std::max(height, 1)));
            cameraTarget.setPos(playerTransform.position);
            camera.update(frameDelta);
            RenderSystem::renderScene(scene, camera, renderer);
            window.swapBuffers();
        }
    } catch (const std::exception& error) {
        std::cerr << "ECS Platformer: " << error.what() << '\n';
        return 1;
    }
}
