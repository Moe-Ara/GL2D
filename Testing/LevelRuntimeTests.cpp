#include <boost/test/unit_test.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Components/TriggerComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Exceptions/SubsystemExceptions.hpp"
#include "LevelBuildingSystem/LevelLoader.hpp"
#include "LevelBuildingSystem/LevelManager.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/TriggerSystem.hpp"

#include <memory>
#include <string>
#include <utility>

namespace {
LevelData levelWithTrigger(TriggerActivation activation,
                           std::string shape = "AABB") {
    LevelData data{};
    data.metadata.name = "trigger_test";
    LevelTrigger trigger{};
    trigger.id = "test.trigger";
    trigger.event = "test.event";
    trigger.activation = activation;
    trigger.shape.type = std::move(shape);
    trigger.shape.pos = {3.0f, 4.0f};
    trigger.shape.size = {4.0f, 6.0f};
    trigger.shape.radius = 2.0f;
    trigger.params["amount"] = 2.0f;
    data.triggers.push_back(std::move(trigger));
    return data;
}

std::unique_ptr<Entity> overlappingBody(glm::vec2 position) {
    auto entity = std::make_unique<Entity>();
    auto& transform = entity->addComponent<TransformComponent>();
    transform.setPosition(position);
    entity->addComponent<ColliderComponent>(
        std::make_unique<CircleCollider>(0.5f));
    return entity;
}
}

BOOST_AUTO_TEST_SUITE(LevelRuntimeTests)

BOOST_AUTO_TEST_CASE(level_loader_builds_exact_aabb_and_circle_triggers) {
    Level boxLevel = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::OnEnter));
    BOOST_REQUIRE(boxLevel.entities.size() == 1u);
    auto* boxTrigger = boxLevel.entities[0]->getComponent<TriggerComponent>();
    auto* boxCollider = boxLevel.entities[0]->getComponent<ColliderComponent>();
    BOOST_REQUIRE(boxTrigger && boxCollider);
    boxCollider->ensureCollider(*boxLevel.entities[0]);
    BOOST_TEST(boxCollider->isTrigger());
    BOOST_REQUIRE(dynamic_cast<AABBCollider*>(boxCollider->collider()));
    BOOST_TEST(boxTrigger->eventId() == "test.event");
    BOOST_TEST(boxTrigger->parameters().at("amount") == 2.0f);
    BOOST_TEST(boxCollider->collider()->getAABB().width() == 4.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(boxCollider->collider()->getAABB().height() == 6.0f,
               boost::test_tools::tolerance(0.0001f));

    Level circleLevel = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::OnEnter, "Circle"));
    auto* circleCollider =
        circleLevel.entities[0]->getComponent<ColliderComponent>();
    BOOST_REQUIRE(circleCollider);
    circleCollider->ensureCollider(*circleLevel.entities[0]);
    BOOST_REQUIRE(dynamic_cast<CircleCollider*>(circleCollider->collider()));
}

BOOST_AUTO_TEST_CASE(serialized_trigger_modes_follow_exact_overlap_lifetime) {
    Level level = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::WhileInside));
    Entity& triggerEntity = *level.entities[0];
    auto* trigger = triggerEntity.getComponent<TriggerComponent>();
    BOOST_REQUIRE(trigger);
    int activations = 0;
    trigger->setCallback(
        [&](Entity& owner, Entity* other, const TriggerComponent& source) {
            BOOST_TEST(&owner == &triggerEntity);
            BOOST_REQUIRE(other);
            BOOST_TEST(source.eventId() == "test.event");
            ++activations;
        });
    level.entities.push_back(overlappingBody({3.0f, 4.0f}));

    TriggerSystem system;
    system.update(level.entities);
    system.update(level.entities);
    BOOST_TEST(activations == 2);

    level.entities[1]->getComponent<TransformComponent>()->setPosition(
        {30.0f, 40.0f});
    system.update(level.entities);
    BOOST_TEST(activations == 2);
}

BOOST_AUTO_TEST_CASE(enter_exit_and_manual_modes_are_distinct) {
    Level enterLevel = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::OnEnter));
    auto* enterTrigger = enterLevel.entities[0]->getComponent<TriggerComponent>();
    int enterCount = 0;
    enterTrigger->setCallback(
        [&](Entity&, Entity*, const TriggerComponent&) { ++enterCount; });
    enterLevel.entities.push_back(overlappingBody({3.0f, 4.0f}));
    TriggerSystem enterSystem;
    enterSystem.update(enterLevel.entities);
    enterSystem.update(enterLevel.entities);
    BOOST_TEST(enterCount == 1);

    Level exitLevel = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::OnExit));
    auto* exitTrigger = exitLevel.entities[0]->getComponent<TriggerComponent>();
    int exitCount = 0;
    exitTrigger->setCallback(
        [&](Entity&, Entity*, const TriggerComponent&) { ++exitCount; });
    exitLevel.entities.push_back(overlappingBody({3.0f, 4.0f}));
    TriggerSystem exitSystem;
    exitSystem.update(exitLevel.entities);
    exitLevel.entities[1]->getComponent<TransformComponent>()->setPosition(
        {30.0f, 40.0f});
    exitSystem.update(exitLevel.entities);
    BOOST_TEST(exitCount == 1);

    Level manualLevel = LevelLoader::loadFromData(
        levelWithTrigger(TriggerActivation::Manual));
    Entity& manualEntity = *manualLevel.entities[0];
    auto* manual = manualEntity.getComponent<TriggerComponent>();
    int manualCount = 0;
    manual->setCallback(
        [&](Entity&, Entity* other, const TriggerComponent&) {
            BOOST_TEST(other == nullptr);
            ++manualCount;
        });
    manual->activateManual(manualEntity);
    BOOST_TEST(manualCount == 1);
}

BOOST_AUTO_TEST_CASE(level_loader_rejects_broken_content_references) {
    LevelData missingPrefab{};
    missingPrefab.metadata.name = "broken";
    missingPrefab.instances.push_back(
        LevelInstance{.prefabId = "definitely.missing.prefab"});
    BOOST_CHECK_THROW(LevelLoader::loadFromData(missingPrefab),
                      Engine::LevelException);

    LevelData missingTilemap{};
    missingTilemap.metadata.name = "broken";
    missingTilemap.tileLayers.push_back(
        TileLayer{.tilemapId = "definitely.missing.tilemap"});
    BOOST_CHECK_THROW(LevelLoader::loadFromData(missingTilemap),
                      Engine::LevelException);

    LevelData unsupported = levelWithTrigger(
        TriggerActivation::OnEnter, "Polygon");
    BOOST_CHECK_THROW(LevelLoader::loadFromData(unsupported),
                      Engine::LevelException);

    LevelData unsupportedVersion{};
    unsupportedVersion.metadata.name = "future";
    unsupportedVersion.metadata.schemaVersion = 2;
    BOOST_CHECK_THROW(LevelLoader::loadFromData(unsupportedVersion),
                      Engine::LevelException);
}

BOOST_AUTO_TEST_CASE(level_manager_preserves_context_from_load_failures) {
    LevelManager manager;
    BOOST_TEST(!manager.loadLevel("/definitely/missing/level.json"));
    BOOST_TEST(manager.currentLevel() == nullptr);
    BOOST_TEST(manager.lastError().find("cannot open file") !=
               std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
