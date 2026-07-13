#include <boost/test/unit_test.hpp>

#include "GameObjects/Components/ColliderComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"
#include "GameObjects/Entity.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"
#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/AABB.hpp"
#include "Physics/PhysicsCasts.hpp"

#include <memory>
#include <vector>
#include <limits>
#include <stdexcept>

BOOST_AUTO_TEST_SUITE(PhysicsCastsTests)

BOOST_AUTO_TEST_CASE(ray_capsule_reports_first_impact_not_closest_approach) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Entity>());
    Entity& target = *entities.front();
    target.addComponent<TransformComponent>();
    target.addComponent<ColliderComponent>(
        std::make_unique<CapsuleCollider>(
            glm::vec2{5.0f, -1.0f}, glm::vec2{5.0f, 1.0f}, 1.0f));

    const PhysicsCasts::CastHit hit = PhysicsCasts::rayCast(
        {0.0f, 0.0f}, {1.0f, 0.0f}, 10.0f, entities);

    BOOST_REQUIRE(hit.hit);
    BOOST_TEST(hit.entity == &target);
    BOOST_TEST(hit.distance == 4.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(hit.point.x == 4.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(hit.normal.x == -1.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(box_cast_refines_past_conservative_entry_tangency) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Entity>());
    Entity& target = *entities.front();
    target.addComponent<TransformComponent>().setPosition({5.0f, 0.0f});
    target.addComponent<ColliderComponent>(
        std::make_unique<CircleCollider>(1.0f));

    const auto hit = PhysicsCasts::boxCast(
        AABB{{-0.5f, -0.5f}, {0.5f, 0.5f}}, {1.0f, 0.0f},
        10.0f, entities);

    BOOST_REQUIRE(hit.hit);
    BOOST_TEST(hit.distance == 3.5f,
               boost::test_tools::tolerance(0.001f));
    BOOST_TEST(hit.normal.x < -0.99f);
}

BOOST_AUTO_TEST_CASE(overlap_circle_inside_box_reports_full_exit_depth) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Entity>());
    Entity& target = *entities.front();
    target.addComponent<TransformComponent>();
    target.addComponent<ColliderComponent>(
        std::make_unique<AABBCollider>(
            glm::vec2{-5.0f, -5.0f}, glm::vec2{5.0f, 5.0f}));

    const auto hits = PhysicsCasts::overlapCircle(
        {0.0f, 0.0f}, 1.0f, entities);
    BOOST_REQUIRE(hits.size() == 1u);
    BOOST_TEST(hits.front().distance == 6.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(casts_reject_non_finite_inputs) {
    std::vector<std::unique_ptr<Entity>> entities;
    BOOST_CHECK_THROW(PhysicsCasts::rayCast(
        {0.0f, 0.0f}, {1.0f, 0.0f}, -1.0f, entities),
        std::invalid_argument);
    BOOST_CHECK_THROW(PhysicsCasts::overlapCircle(
        {std::numeric_limits<float>::quiet_NaN(), 0.0f}, 1.0f, entities),
        std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
