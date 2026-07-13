#include <boost/test/unit_test.hpp>

#include "ECS/Systems/LightExtractionSystem.hpp"

#include <limits>
#include <numbers>

BOOST_AUTO_TEST_SUITE(ECSLightingTests)

BOOST_AUTO_TEST_CASE(extraction_applies_transform_and_normalizes_direction) {
    ECS::Transform2D transform{};
    transform.position = {10.0f, 20.0f};
    transform.scale = {2.0f, 3.0f};
    transform.rotationDegrees = 90.0f;
    ECS::Light2D authored = ECS::Light2D::spot(
        200.0f, {1.0f, 0.0f}, {0.3f, 0.6f, 1.0f}, 2.0f);
    authored.localOffset = {2.0f, 1.0f};

    const auto light = ECS::extractLight(transform, authored, nullptr, 0.0);

    BOOST_REQUIRE(light.has_value());
    BOOST_TEST(light->pos.x == 7.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(light->pos.y == 24.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(light->dir.x == 0.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(light->dir.y == 1.0f, boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(pulse_animation_is_deterministic) {
    ECS::Transform2D transform{};
    ECS::Light2D authored = ECS::Light2D::point(100.0f, {1.0f, 1.0f, 1.0f}, 2.0f);
    ECS::LightAnimation2D animation{};
    animation.effector.type = LightEffector::Type::Pulse;
    animation.effector.strength = 0.4f;
    animation.effector.speed = 0.0f;
    animation.effector.phase = -std::numbers::pi_v<float> * 0.5f;

    const auto light = ECS::extractLight(transform, authored, &animation, 0.0);

    BOOST_REQUIRE(light.has_value());
    BOOST_TEST(light->radius == 80.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(light->intensity == 1.6f, boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(non_finite_authoring_data_is_rejected) {
    ECS::Transform2D transform{};
    ECS::Light2D authored{};
    authored.intensity = std::numeric_limits<float>::quiet_NaN();

    BOOST_TEST(!ECS::extractLight(transform, authored, nullptr, 0.0).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
