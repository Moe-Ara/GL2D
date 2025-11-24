#include <boost/test/unit_test.hpp>

#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Utils/Transform.hpp"

namespace {
void attachTransform(CircleCollider &collider, const glm::vec2 &position) {
    auto *transform = new Transform();
    transform->setPos(position);
    collider.setTransform(transform);
}
}

BOOST_AUTO_TEST_SUITE(TriggerCollisionTests)

BOOST_AUTO_TEST_CASE(trigger_overlap_returns_collision_with_zero_penetration) {
    CircleCollider a(1.0f);
    CircleCollider b(1.0f);
    a.setTrigger(true);
    b.setTrigger(true);
    attachTransform(a, {0.0f, 0.0f});
    attachTransform(b, {0.5f, 0.0f});

    auto hit = CollisionDispatcher::dispatch(a, b);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->collided);
    BOOST_TEST(hit->penetration == 0.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(trigger_once_consumes_after_first_overlap) {
    CircleCollider trigger(1.0f);
    CircleCollider other(1.0f);
    trigger.setTrigger(true, true);
    attachTransform(trigger, {0.0f, 0.0f});
    attachTransform(other, {0.25f, 0.0f});

    auto first = CollisionDispatcher::dispatch(trigger, other);
    BOOST_REQUIRE(first);
    BOOST_TEST(first->collided);

    auto second = CollisionDispatcher::dispatch(trigger, other);
    BOOST_TEST(!second);
}

BOOST_AUTO_TEST_SUITE_END()
