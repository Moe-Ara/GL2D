#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Animation/AnimationGraph2D.hpp"
#include "ECS/Components/Animation2D.hpp"
#include "ECS/Components/SpriteRender.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/AnimationSystem2D.hpp"
#include "GameObjects/Sprite.hpp"

#include <memory>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
ECS::AnimationFrame2D frame(float duration, glm::vec4 uv, std::string event = {}) {
    ECS::AnimationFrame2D result{};
    result.durationSeconds = duration;
    result.uvRect = uv;
    result.event = std::move(event);
    return result;
}

std::shared_ptr<const ECS::AnimationGraph2D> makeGraph() {
    std::vector<ECS::AnimationState2D> states{
        {"Idle", {{frame(0.1f, {0.0f, 0.0f, 0.5f, 1.0f}, "idle_start"),
                     frame(0.1f, {0.5f, 0.0f, 1.0f, 1.0f}, "idle_end")},
                    ECS::AnimationPlayback2D::Loop}},
        {"Run", {{frame(0.05f, {0.0f, 0.0f, 0.25f, 1.0f}, "footstep"),
                    frame(0.05f, {0.25f, 0.0f, 0.5f, 1.0f})},
                   ECS::AnimationPlayback2D::Loop}}
    };
    std::vector<ECS::AnimationTransition2D> transitions{
        {"Idle", "Run", ECS::AnimationCondition2D::BoolEquals, "moving", 0.0f, true},
        {"Run", "Idle", ECS::AnimationCondition2D::BoolEquals, "moving", 0.0f, false}
    };
    return std::make_shared<const ECS::AnimationGraph2D>(
        std::move(states), std::move(transitions), "Idle");
}

ECS::Entity addAnimated(ECS::Registry& registry,
                        const std::shared_ptr<const ECS::AnimationGraph2D>& graph,
                        const std::shared_ptr<GameObjects::Sprite>& sprite) {
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::SpriteRender>(entity, ECS::SpriteRender{sprite});
    registry.emplace<ECS::AnimationParameters2D>(entity);
    registry.emplace<ECS::Animator2D>(entity).graph = graph;
    registry.emplace<ECS::AnimationEventQueue2D>(entity);
    return entity;
}
}

BOOST_AUTO_TEST_SUITE(ECSAnimationTests)

BOOST_AUTO_TEST_CASE(graph_rejects_invalid_resources) {
    BOOST_CHECK_THROW(
        ECS::AnimationGraph2D({}, {}, "Idle"), std::invalid_argument);

    std::vector<ECS::AnimationState2D> states{
        {"Idle", {{frame(0.0f, {0.0f, 0.0f, 1.0f, 1.0f})},
                    ECS::AnimationPlayback2D::Loop}}
    };
    BOOST_CHECK_THROW(
        ECS::AnimationGraph2D(std::move(states), {}, "Idle"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(shared_graph_keeps_instance_presentation_independent) {
    ECS::Registry registry;
    const auto graph = makeGraph();
    auto sprite = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{32.0f}, glm::vec3{1.0f});
    const glm::vec4 resourceUV = sprite->getUVCoords();
    const ECS::Entity running = addAnimated(registry, graph, sprite);
    const ECS::Entity idle = addAnimated(registry, graph, sprite);
    registry.get<ECS::AnimationParameters2D>(running).setBool("moving", true);
    registry.get<ECS::AnimationParameters2D>(idle).setBool("moving", false);

    ECS::AnimationSystem2D::beginFrame(registry);
    ECS::AnimationSystem2D::update(registry, 0.01f);

    BOOST_TEST(graph->state(registry.get<ECS::Animator2D>(running).stateIndex).name == "Run");
    BOOST_TEST(graph->state(registry.get<ECS::Animator2D>(idle).stateIndex).name == "Idle");
    BOOST_TEST(registry.get<ECS::SpriteRender>(running).uvRect.z == 0.25f);
    BOOST_TEST(registry.get<ECS::SpriteRender>(idle).uvRect.z == 0.5f);
    BOOST_TEST(sprite->getUVCoords().x == resourceUV.x);
    BOOST_TEST(sprite->getUVCoords().z == resourceUV.z);
}

BOOST_AUTO_TEST_CASE(frame_events_accumulate_across_fixed_steps_in_one_frame) {
    ECS::Registry registry;
    const auto graph = makeGraph();
    auto sprite = std::make_shared<GameObjects::Sprite>(
        glm::vec2{0.0f}, glm::vec2{32.0f}, glm::vec3{1.0f});
    const ECS::Entity entity = addAnimated(registry, graph, sprite);
    registry.get<ECS::AnimationParameters2D>(entity).setBool("moving", true);

    ECS::AnimationSystem2D::beginFrame(registry);
    ECS::AnimationSystem2D::update(registry, 0.06f);
    ECS::AnimationSystem2D::update(registry, 0.05f);

    const auto& events = registry.get<ECS::AnimationEventQueue2D>(entity).events;
    BOOST_TEST(events.size() >= 4u);
    BOOST_TEST(std::ranges::any_of(events, [](const ECS::AnimationEvent2D& event) {
        return event.kind == ECS::AnimationEventKind2D::Frame && event.name == "footstep";
    }));

    ECS::AnimationSystem2D::beginFrame(registry);
    BOOST_TEST(registry.get<ECS::AnimationEventQueue2D>(entity).events.empty());
}

BOOST_AUTO_TEST_SUITE_END()
