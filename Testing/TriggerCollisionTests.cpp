#include <boost/test/unit_test.hpp>

#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Utils/Transform.hpp"

BOOST_AUTO_TEST_SUITE(TriggerCollisionTests)

BOOST_AUTO_TEST_CASE(trigger_overlap_returns_collision_with_zero_penetration) {
    CircleCollider a(1.0f);
    CircleCollider b(1.0f);
    a.setTrigger(true);
    b.setTrigger(true);
    Transform ta{};
    Transform tb{};
    ta.setPos({0.0f, 0.0f});
    tb.setPos({0.5f, 0.0f});
    a.setTransform(&ta);
    b.setTransform(&tb);

    auto hit = CollisionDispatcher::dispatch(a, b);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->collided);
    BOOST_TEST(hit->penetration == 0.0f, boost::test_tools::tolerance(1e-5f));
}

BOOST_AUTO_TEST_CASE(trigger_once_consumes_after_first_overlap) {
    CircleCollider trigger(1.0f);
    CircleCollider other(1.0f);
    trigger.setTrigger(true, true);
    Transform ta{};
    Transform tb{};
    ta.setPos({0.0f, 0.0f});
    tb.setPos({0.25f, 0.0f});
    trigger.setTransform(&ta);
    other.setTransform(&tb);

    auto first = CollisionDispatcher::dispatch(trigger, other);
    BOOST_REQUIRE(first);
    BOOST_TEST(first->collided);

    auto second = CollisionDispatcher::dispatch(trigger, other);
    BOOST_TEST(!second);
}

BOOST_AUTO_TEST_SUITE_END()
