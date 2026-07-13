#include <boost/test/unit_test.hpp>

#include "Physics/Collision/AABBCollider.hpp"
#include "Physics/Collision/CapsuleCollider.hpp"
#include "Physics/Collision/CircleCollider.hpp"
#include "Physics/Collision/CollisionDispatcher.hpp"
#include "Utils/Transform.hpp"

#include <cmath>
#include <glm/geometric.hpp>

namespace {
void requireSymmetric(const ICollider& a, const ICollider& b) {
    const auto ab = CollisionDispatcher::dispatch(a, b);
    const auto ba = CollisionDispatcher::dispatch(b, a);
    BOOST_REQUIRE(ab);
    BOOST_REQUIRE(ba);
    BOOST_TEST(ab->penetration == ba->penetration,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(ab->normal.x == -ba->normal.x,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(ab->normal.y == -ba->normal.y,
               boost::test_tools::tolerance(0.0001f));
}
}

BOOST_AUTO_TEST_SUITE(CollisionDispatcherTests)

BOOST_AUTO_TEST_CASE(normals_point_from_second_collider_toward_first) {
    CircleCollider first{1.0f};
    CircleCollider second{1.0f};
    Transform firstTransform{};
    Transform secondTransform{};
    secondTransform.setPos({1.5f, 0.0f});
    first.setTransform(&firstTransform);
    second.setTransform(&secondTransform);

    const auto hit = CollisionDispatcher::dispatch(first, second);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->normal.x == -1.0f);
    BOOST_TEST(hit->normal.y == 0.0f);
    requireSymmetric(first, second);
}

BOOST_AUTO_TEST_CASE(circle_inside_box_reports_full_exit_penetration) {
    CircleCollider circle{1.0f};
    AABBCollider box{{-5.0f, -5.0f}, {5.0f, 5.0f}};

    const auto hit = CollisionDispatcher::dispatch(circle, box);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->penetration == 6.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(glm::length(hit->normal) == 1.0f,
               boost::test_tools::tolerance(0.0001f));
    requireSymmetric(circle, box);
}

BOOST_AUTO_TEST_CASE(capsule_inside_box_reports_full_exit_penetration) {
    CapsuleCollider capsule{{0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f};
    AABBCollider box{{-5.0f, -5.0f}, {5.0f, 5.0f}};

    const auto hit = CollisionDispatcher::dispatch(capsule, box);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->penetration == 6.0f,
               boost::test_tools::tolerance(0.0001f));
    requireSymmetric(capsule, box);
}

BOOST_AUTO_TEST_CASE(capsule_box_uses_nearest_edge_or_corner) {
    CapsuleCollider capsule{{1.3f, 1.4f}, {2.5f, 2.0f}, 0.6f};
    AABBCollider box{{-1.0f, -1.0f}, {1.0f, 1.0f}};

    const auto hit = CollisionDispatcher::dispatch(capsule, box);
    BOOST_REQUIRE(hit);
    BOOST_TEST(hit->penetration == 0.1f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(hit->normal.x == 0.6f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(hit->normal.y == 0.8f,
               boost::test_tools::tolerance(0.0001f));
    requireSymmetric(capsule, box);
}

BOOST_AUTO_TEST_CASE(degenerate_capsules_collide_without_nan) {
    CapsuleCollider first{{0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f};
    CapsuleCollider second{{0.0f, 0.0f}, {0.0f, 0.0f}, 1.0f};
    Transform firstTransform{};
    Transform secondTransform{};
    secondTransform.setPos({1.5f, 0.0f});
    first.setTransform(&firstTransform);
    second.setTransform(&secondTransform);

    const auto hit = CollisionDispatcher::dispatch(first, second);
    BOOST_REQUIRE(hit);
    BOOST_TEST(std::isfinite(hit->normal.x));
    BOOST_TEST(std::isfinite(hit->normal.y));
    BOOST_TEST(hit->penetration == 0.5f,
               boost::test_tools::tolerance(0.0001f));
    requireSymmetric(first, second);
}

BOOST_AUTO_TEST_CASE(collider_bounds_respect_rotation_and_negative_scale) {
    AABBCollider box{{0.0f, 0.0f}, {2.0f, 1.0f}};
    Transform boxTransform{};
    boxTransform.setRotation(90.0f);
    box.setTransform(&boxTransform);
    const AABB bounds = box.getAABB();
    BOOST_TEST(bounds.width() == 1.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(bounds.height() == 2.0f,
               boost::test_tools::tolerance(0.0001f));

    CircleCollider circle{1.0f};
    circle.setLocalOffset({1.0f, 0.0f});
    Transform circleTransform{};
    circleTransform.setPos({5.0f, 0.0f});
    circleTransform.setScale({-2.0f, -3.0f});
    circleTransform.setRotation(90.0f);
    circle.setTransform(&circleTransform);
    BOOST_TEST(circle.getWorldCenter().x == 5.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(circle.getWorldCenter().y == -2.0f,
               boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(circle.getWorldRadius() == 3.0f);
}

BOOST_AUTO_TEST_CASE(rotated_boxes_do_not_collide_through_inflated_bounds) {
    AABBCollider first{{-2.0f, -0.1f}, {2.0f, 0.1f}};
    AABBCollider second{{-2.0f, -0.1f}, {2.0f, 0.1f}};
    Transform firstTransform{};
    Transform secondTransform{};
    firstTransform.setRotation(45.0f);
    secondTransform.setRotation(45.0f);
    secondTransform.setPos({-0.5f, 0.5f});
    first.setTransform(&firstTransform);
    second.setTransform(&secondTransform);

    BOOST_TEST(first.getAABB().overlaps(second.getAABB()));
    BOOST_TEST(!CollisionDispatcher::dispatch(first, second));
}

BOOST_AUTO_TEST_CASE(rotated_box_pairs_use_oriented_narrowphase) {
    AABBCollider first{{-2.0f, -0.2f}, {2.0f, 0.2f}};
    AABBCollider second{{-1.0f, -0.4f}, {1.0f, 0.4f}};
    Transform firstTransform{};
    Transform secondTransform{};
    firstTransform.setRotation(35.0f);
    secondTransform.setRotation(-20.0f);
    secondTransform.setPos({0.3f, 0.2f});
    first.setTransform(&firstTransform);
    second.setTransform(&secondTransform);

    requireSymmetric(first, second);
}

BOOST_AUTO_TEST_CASE(circle_ignores_empty_corner_of_rotated_box_bounds) {
    AABBCollider box{{-2.0f, -0.1f}, {2.0f, 0.1f}};
    CircleCollider circle{0.1f};
    Transform boxTransform{};
    Transform circleTransform{};
    boxTransform.setRotation(45.0f);
    circleTransform.setPos({1.3f, -1.3f});
    box.setTransform(&boxTransform);
    circle.setTransform(&circleTransform);

    BOOST_TEST(box.getAABB().overlaps(circle.getAABB()));
    BOOST_TEST(!CollisionDispatcher::dispatch(circle, box));
}

BOOST_AUTO_TEST_CASE(capsule_collides_with_rotated_box_symmetrically) {
    AABBCollider box{{-1.5f, -0.2f}, {1.5f, 0.2f}};
    CapsuleCollider capsule{{0.0f, -0.4f}, {0.0f, 0.4f}, 0.3f};
    Transform boxTransform{};
    Transform capsuleTransform{};
    boxTransform.setRotation(30.0f);
    capsuleTransform.setPos({0.0f, 0.2f});
    box.setTransform(&boxTransform);
    capsule.setTransform(&capsuleTransform);

    requireSymmetric(capsule, box);
}

BOOST_AUTO_TEST_CASE(shape_pairs_preserve_dispatch_symmetry) {
    CircleCollider circle{1.0f};
    AABBCollider box{{-0.75f, -0.75f}, {0.75f, 0.75f}};
    CapsuleCollider capsule{{0.0f, -0.5f}, {0.0f, 0.5f}, 0.75f};
    Transform circleTransform{};
    Transform boxTransform{};
    Transform capsuleTransform{};
    circleTransform.setPos({0.8f, 0.0f});
    capsuleTransform.setPos({-0.8f, 0.0f});
    circle.setTransform(&circleTransform);
    box.setTransform(&boxTransform);
    capsule.setTransform(&capsuleTransform);

    requireSymmetric(circle, box);
    requireSymmetric(capsule, circle);
    requireSymmetric(capsule, box);
}

BOOST_AUTO_TEST_SUITE_END()
