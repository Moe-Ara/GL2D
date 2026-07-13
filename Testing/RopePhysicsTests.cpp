#include <boost/test/unit_test.hpp>

#include "Engine/Scene.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/HingeComponent.hpp"
#include "GameObjects/Components/RigidBodyComponent.hpp"
#include "GameObjects/Components/RopeSegmentComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Prefabs/PrefabDefinitions.hpp"
#include "GameObjects/Prefabs/RopePrefab.hpp"
#include "Physics/RigidBody.hpp"

#include <glm/geometric.hpp>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, RigidBodyType type) {
    switch (type) {
    case RigidBodyType::STATIC: return os << "STATIC";
    case RigidBodyType::KINEMATIC: return os << "KINEMATIC";
    case RigidBodyType::DYNAMIC: return os << "DYNAMIC";
    }
    return os << "Unknown";
}

namespace {
Entity& addStaticAnchor(Scene& scene, glm::vec2 position) {
    Entity& anchor = scene.createEntity();
    auto& transform = anchor.addComponent<TransformComponent>();
    transform.setPosition(position);
    anchor.addComponent<RigidBodyComponent>(
        std::make_unique<RigidBody>(0.0f, RigidBodyType::STATIC));
    return anchor;
}
}

BOOST_AUTO_TEST_SUITE(RopePhysicsTests)

BOOST_AUTO_TEST_CASE(default_rope_creates_a_real_world_anchor_and_chain_links) {
    Prefabs::registerGamePrefabs();
    Scene scene;
    Prefabs::RopePrefabConfig config{};
    config.anchorPosition = {4.0f, 10.0f};
    config.segmentCount = 3;
    config.segmentLength = 2.0f;
    config.segmentThickness = 0.25f;
    config.segmentSpacing = 0.0f;
    config.limitEnabled = false;

    const auto rope = Prefabs::RopePrefab::instantiate(scene, config);
    BOOST_REQUIRE(rope.anchor);
    BOOST_REQUIRE(rope.segments.size() == 3u);
    BOOST_REQUIRE(rope.anchor->getComponent<RigidBodyComponent>());
    BOOST_TEST(rope.anchor->getComponent<RigidBodyComponent>()->body()->getBodyType() ==
               RigidBodyType::STATIC);

    const auto* first = rope.segments[0]->getComponent<RopeSegmentComponent>();
    const auto* middle = rope.segments[1]->getComponent<RopeSegmentComponent>();
    const auto* last = rope.segments[2]->getComponent<RopeSegmentComponent>();
    BOOST_REQUIRE(first && middle && last);
    BOOST_TEST(first->previous() == nullptr);
    BOOST_TEST(first->next() == rope.segments[1]);
    BOOST_TEST(middle->previous() == rope.segments[0]);
    BOOST_TEST(middle->next() == rope.segments[2]);
    BOOST_TEST(last->next() == nullptr);

    const auto* collider = rope.segments[0]->getComponent<ColliderComponent>();
    BOOST_REQUIRE(collider);
    BOOST_TEST((collider->getCollisionMask() &
                (1u << collider->getLayer())) == 0u);
}

BOOST_AUTO_TEST_CASE(two_ended_rope_solves_both_tail_hinges) {
    Prefabs::registerGamePrefabs();
    Scene scene;
    Entity& endAnchor = addStaticAnchor(scene, {0.0f, -6.0f});
    Prefabs::RopePrefabConfig config{};
    config.anchorPosition = {0.0f, 0.0f};
    config.segmentCount = 3;
    config.segmentLength = 2.0f;
    config.segmentThickness = 0.25f;
    config.segmentSpacing = 0.0f;
    config.limitEnabled = false;
    config.endAnchor = &endAnchor;

    const auto rope = Prefabs::RopePrefab::instantiate(scene, config);
    std::size_t tailHingeCount = 0;
    for (const auto& component : rope.segments.back()->components()) {
        tailHingeCount += dynamic_cast<HingeComponent*>(component.get()) != nullptr;
    }
    BOOST_TEST(tailHingeCount == 2u);

    scene.update(1.0f / 120.0f);
    auto* disturbed = rope.segments[1]->getComponent<RigidBodyComponent>()->body();
    disturbed->setPosition(disturbed->getPosition() + glm::vec2{2.0f, 0.0f});
    for (int i = 0; i < 30; ++i) {
        scene.update(1.0f / 120.0f);
    }

    const auto* firstInfo = rope.segments.front()->getComponent<RopeSegmentComponent>();
    const auto* lastInfo = rope.segments.back()->getComponent<RopeSegmentComponent>();
    const auto firstEndpoints = firstInfo->worldEndpoints(*rope.segments.front());
    const auto lastEndpoints = lastInfo->worldEndpoints(*rope.segments.back());
    BOOST_TEST(glm::length(firstEndpoints.first - config.anchorPosition) < 0.05f);
    BOOST_TEST(glm::length(lastEndpoints.second - glm::vec2{0.0f, -6.0f}) < 0.05f);
}

BOOST_AUTO_TEST_CASE(segment_spacing_is_preserved_by_joint_anchors) {
    Prefabs::registerGamePrefabs();
    Scene scene;
    Prefabs::RopePrefabConfig config{};
    config.segmentCount = 2;
    config.segmentLength = 2.0f;
    config.segmentThickness = 0.25f;
    config.segmentSpacing = 0.5f;
    config.limitEnabled = false;

    const auto rope = Prefabs::RopePrefab::instantiate(scene, config);
    scene.update(1.0f / 120.0f);
    const auto* first = rope.segments[0]->getComponent<TransformComponent>();
    const auto* second = rope.segments[1]->getComponent<TransformComponent>();
    BOOST_REQUIRE(first && second);
    BOOST_TEST(glm::length(second->getTransform().Position -
                           first->getTransform().Position) == 2.5f,
               boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_CASE(destroying_a_segment_detaches_neighbor_links) {
    Prefabs::registerGamePrefabs();
    Scene scene;
    Prefabs::RopePrefabConfig config{};
    config.segmentCount = 3;
    config.segmentLength = 2.0f;
    config.segmentThickness = 0.25f;
    const auto rope = Prefabs::RopePrefab::instantiate(scene, config);

    scene.destroyEntity(*rope.segments[1]);

    BOOST_TEST(rope.segments[0]->getComponent<RopeSegmentComponent>()->next() == nullptr);
    BOOST_TEST(rope.segments[2]->getComponent<RopeSegmentComponent>()->previous() == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
