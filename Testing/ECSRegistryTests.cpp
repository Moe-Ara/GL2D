#include <boost/test/unit_test.hpp>

#include "EcsTestOutput.hpp"

#include "ECS/Registry.hpp"
#include "ECS/Components/Transform2D.hpp"

namespace {
struct Position {
    float x{0.0f};
    float y{0.0f};
};

struct Velocity {
    float x{0.0f};
    float y{0.0f};
};
}

BOOST_AUTO_TEST_SUITE(ECSRegistryTests)

BOOST_AUTO_TEST_CASE(generational_handles_reject_destroyed_entities) {
    ECS::Registry registry;
    const ECS::Entity first = registry.create();
    registry.emplace<Position>(first, 2.0f, 3.0f);

    BOOST_TEST(registry.alive(first));
    BOOST_TEST(registry.destroy(first));
    BOOST_TEST(!registry.alive(first));
    BOOST_TEST(registry.tryGet<Position>(first) == nullptr);

    const ECS::Entity replacement = registry.create();
    BOOST_TEST(replacement.index() == first.index());
    BOOST_TEST(replacement.generation() != first.generation());
}

BOOST_AUTO_TEST_CASE(queries_join_components_and_persist_mutations) {
    ECS::Registry registry;
    const ECS::Entity moving = registry.create();
    const ECS::Entity stationary = registry.create();
    registry.emplace<Position>(moving, 1.0f, 2.0f);
    registry.emplace<Velocity>(moving, 3.0f, 4.0f);
    registry.emplace<Position>(stationary, 10.0f, 20.0f);

    std::size_t visited = 0;
    registry.each<Position, Velocity>([&](ECS::Entity entity, Position& position, Velocity& velocity) {
        BOOST_TEST(entity == moving);
        position.x += velocity.x;
        position.y += velocity.y;
        ++visited;
    });

    BOOST_TEST(visited == 1u);
    BOOST_TEST(registry.get<Position>(moving).x == 4.0f);
    BOOST_TEST(registry.get<Position>(moving).y == 6.0f);
}

BOOST_AUTO_TEST_CASE(destruction_during_query_is_deferred_and_safe) {
    ECS::Registry registry;
    const ECS::Entity first = registry.create();
    const ECS::Entity second = registry.create();
    registry.emplace<Position>(first);
    registry.emplace<Position>(second);

    std::size_t visited = 0;
    registry.each<Position>([&](ECS::Entity entity, Position&) {
        ++visited;
        BOOST_TEST(registry.destroy(entity));
        BOOST_TEST(!registry.alive(entity));
    });

    BOOST_TEST(visited == 2u);
    BOOST_TEST(registry.empty());
}

BOOST_AUTO_TEST_CASE(structural_component_changes_fail_during_query) {
    ECS::Registry registry;
    const ECS::Entity entity = registry.create();
    registry.emplace<Position>(entity);

    BOOST_CHECK_THROW(
        registry.each<Position>([&](ECS::Entity current, Position&) {
            registry.emplace<Velocity>(current);
        }),
        std::logic_error);
    BOOST_TEST(!registry.has<Velocity>(entity));
}

BOOST_AUTO_TEST_CASE(clear_invalidates_handles_without_reviving_them) {
    ECS::Registry registry;
    const ECS::Entity beforeClear = registry.create();
    registry.emplace<Position>(beforeClear);
    registry.clear();

    const ECS::Entity afterClear = registry.create();
    BOOST_TEST(afterClear.index() == beforeClear.index());
    BOOST_TEST(afterClear.generation() != beforeClear.generation());
    BOOST_TEST(!registry.alive(beforeClear));
}

BOOST_AUTO_TEST_CASE(transform_component_composes_translation_rotation_and_scale) {
    ECS::Transform2D transform{{10.0f, 20.0f}, {2.0f, 3.0f}, 90.0f};
    const glm::vec4 world = ECS::toMatrix(transform) * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
    BOOST_TEST(world.x == 10.0f, boost::test_tools::tolerance(1e-5f));
    BOOST_TEST(world.y == 22.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_SUITE_END()
