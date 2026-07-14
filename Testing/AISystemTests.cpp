#include <boost/test/unit_test.hpp>

#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "AISystem/AICombatBrain.hpp"
#include "AISystem/Perception.hpp"
#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/CombatComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/AABBCollider.hpp"

namespace {
std::unique_ptr<Entity> makeBoxEntity(const glm::vec2& position,
                                      std::uint32_t layer = 0) {
    auto entity = std::make_unique<Entity>();
    entity->addComponent<TransformComponent>().setPosition(position);
    auto collider = std::make_unique<AABBCollider>(glm::vec2{-0.5f}, glm::vec2{0.5f});
    auto& component = entity->addComponent<ColliderComponent>(std::move(collider));
    component.setLayer(layer);
    component.ensureCollider(*entity);
    return entity;
}
}

BOOST_AUTO_TEST_SUITE(AISystemTests)

BOOST_AUTO_TEST_CASE(combat_brain_advances_time_independently_from_attack_attempts) {
    AICombatBrain brain;
    brain.setAttackRange(5.0f);
    brain.setCooldown(1.0f);

    BOOST_TEST(brain.tryAttack(glm::vec2{0.0f}, glm::vec2{3.0f, 4.0f}));
    BOOST_TEST(brain.isOnCooldown());
    BOOST_TEST(!brain.tryAttack(glm::vec2{0.0f}, glm::vec2{1.0f}));
    brain.update(0.4f);
    BOOST_TEST(!brain.tryAttack(glm::vec2{0.0f}, glm::vec2{1.0f}));
    brain.update(0.6f);
    BOOST_TEST(brain.tryAttack(glm::vec2{0.0f}, glm::vec2{1.0f}));
}

BOOST_AUTO_TEST_CASE(combat_configuration_and_inputs_are_validated) {
    AICombatBrain brain;
    BOOST_CHECK_THROW(brain.setAttackRange(-1.0f), std::invalid_argument);
    BOOST_CHECK_THROW(brain.setCooldown(std::numeric_limits<float>::infinity()),
                      std::invalid_argument);
    BOOST_CHECK_THROW(brain.update(-0.01f), std::invalid_argument);
    BOOST_CHECK_THROW(
        static_cast<void>(brain.tryAttack(
            glm::vec2{std::numeric_limits<float>::quiet_NaN()},
            glm::vec2{0.0f})),
        std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(combat_component_does_not_attack_without_an_explicit_target) {
    Entity owner;
    owner.addComponent<TransformComponent>().setPosition(glm::vec2{0.0f});
    auto& combat = owner.addComponent<CombatComponent>();
    combat.setRange(10.0f);
    combat.setCooldown(1.0f);
    int attacks = 0;
    combat.setOnAttack([&](Entity&) { ++attacks; });

    combat.update(owner, 0.25);
    BOOST_TEST(attacks == 0);
    combat.setTargetPosition(glm::vec2{2.0f, 0.0f});
    combat.update(owner, 0.0);
    BOOST_TEST(attacks == 1);
    combat.update(owner, 1.0);
    BOOST_TEST(attacks == 2);
    combat.clearTarget();
    combat.update(owner, 1.0);
    BOOST_TEST(attacks == 2);
}

BOOST_AUTO_TEST_CASE(line_of_sight_recognizes_the_target_collider) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(makeBoxEntity(glm::vec2{10.0f, 0.0f}));
    Entity* target = entities.back().get();

    BOOST_TEST(!AI::hasLineOfSight(glm::vec2{0.0f}, glm::vec2{10.0f, 0.0f},
                                   entities));
    BOOST_TEST(AI::hasLineOfSight(glm::vec2{0.0f}, glm::vec2{10.0f, 0.0f},
                                  entities, 0xFFFFFFFFu, nullptr, target));

    entities.insert(entities.begin(), makeBoxEntity(glm::vec2{5.0f, 0.0f}));
    BOOST_TEST(!AI::hasLineOfSight(glm::vec2{0.0f}, glm::vec2{10.0f, 0.0f},
                                   entities, 0xFFFFFFFFu, nullptr, target));
}

BOOST_AUTO_TEST_CASE(hearing_clears_output_and_respects_radius_and_layers) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(makeBoxEntity(glm::vec2{2.0f, 0.0f}, 3));
    entities.push_back(makeBoxEntity(glm::vec2{20.0f, 0.0f}, 3));
    std::vector<Entity*> heard{entities.back().get()};

    BOOST_TEST(!AI::canHear(glm::vec2{0.0f}, 0.0f, entities, 1u << 3u,
                            nullptr, &heard));
    BOOST_TEST(heard.empty());
    BOOST_TEST(AI::canHear(glm::vec2{0.0f}, 3.0f, entities, 1u << 3u,
                           nullptr, &heard));
    BOOST_REQUIRE(heard.size() == 1u);
    BOOST_TEST(heard.front() == entities.front().get());

    BOOST_TEST(!AI::canHear(glm::vec2{0.0f}, 3.0f, entities, 1u << 2u,
                            nullptr, &heard));
    BOOST_TEST(heard.empty());
    BOOST_CHECK_THROW(AI::canHear(glm::vec2{0.0f}, -1.0f, entities),
                      std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
