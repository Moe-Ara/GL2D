#include <boost/test/unit_test.hpp>

#include "ECS/Components/ParallaxLayer2D.hpp"
#include "ECS/Components/Transform2D.hpp"
#include "ECS/Registry.hpp"
#include "ECS/Systems/ParallaxSystem2D.hpp"

namespace {
ECS::Entity addLayer(ECS::Registry& registry, glm::vec2 factor,
                     glm::vec2 basePosition = {0.0f, 0.0f},
                     float tileWidth = 0.0f, int tileIndex = 0) {
    const ECS::Entity entity = registry.create();
    registry.emplace<ECS::Transform2D>(entity);
    auto& layer = registry.emplace<ECS::ParallaxLayer2D>(entity);
    layer.factor = factor;
    layer.basePosition = basePosition;
    layer.baseCameraCenter = {0.0f, 0.0f};
    layer.tileWidth = tileWidth;
    layer.tileIndex = tileIndex;
    return entity;
}
}

BOOST_AUTO_TEST_SUITE(ParallaxSystemTests)

BOOST_AUTO_TEST_CASE(a_pinned_layer_follows_the_camera_one_to_one) {
    ECS::Registry registry;
    const ECS::Entity layer = addLayer(registry, {0.0f, 0.0f});

    ECS::ParallaxSystem2D::update(registry, {100.0f, 40.0f}, -50.0f, 50.0f);

    // factor 0 => anchor moves with the camera by (1 - 0) = full delta.
    const auto& transform = registry.get<ECS::Transform2D>(layer);
    BOOST_TEST(transform.position.x == 100.0f, boost::test_tools::tolerance(0.001f));
    BOOST_TEST(transform.position.y == 40.0f, boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_CASE(a_world_locked_layer_ignores_the_camera) {
    ECS::Registry registry;
    const ECS::Entity layer = addLayer(registry, {1.0f, 1.0f}, {10.0f, 5.0f});

    ECS::ParallaxSystem2D::update(registry, {300.0f, 80.0f}, 0.0f, 100.0f);

    // factor 1 => anchor stays at basePosition regardless of camera travel.
    const auto& transform = registry.get<ECS::Transform2D>(layer);
    BOOST_TEST(transform.position.x == 10.0f, boost::test_tools::tolerance(0.001f));
    BOOST_TEST(transform.position.y == 5.0f, boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_CASE(a_mid_layer_scrolls_at_a_fraction_of_camera_travel) {
    ECS::Registry registry;
    const ECS::Entity layer = addLayer(registry, {0.25f, 0.25f});

    ECS::ParallaxSystem2D::update(registry, {200.0f, 0.0f}, -1000.0f, 1000.0f);

    // Untiled: x = basePosition.x + delta.x * (1 - factor) = 0 + 200 * 0.75.
    const auto& transform = registry.get<ECS::Transform2D>(layer);
    BOOST_TEST(transform.position.x == 150.0f, boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_CASE(tiled_layers_wrap_to_stay_within_a_tile_of_the_view) {
    ECS::Registry registry;
    const float tileWidth = 100.0f;
    const ECS::Entity tile0 = addLayer(registry, {1.0f, 1.0f}, {0.0f, 0.0f}, tileWidth, 0);
    const ECS::Entity tile1 = addLayer(registry, {1.0f, 1.0f}, {0.0f, 0.0f}, tileWidth, 1);

    // Camera far to the right; a world-locked strip must still cover the view.
    const float viewMinX = 5000.0f;
    ECS::ParallaxSystem2D::update(registry, {5050.0f, 0.0f}, viewMinX, viewMinX + 200.0f);

    const auto& t0 = registry.get<ECS::Transform2D>(tile0);
    const auto& t1 = registry.get<ECS::Transform2D>(tile1);
    // tile0 sits within one tile-width to the left of the view edge.
    BOOST_TEST(t0.position.x <= viewMinX);
    BOOST_TEST(t0.position.x >= viewMinX - tileWidth);
    // tiles are contiguous.
    BOOST_TEST(t1.position.x - t0.position.x == tileWidth,
               boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_SUITE_END()
