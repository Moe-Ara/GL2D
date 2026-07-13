#include <boost/test/unit_test.hpp>

#include "Graphics/Camera/Camera.hpp"
#include "Graphics/Camera/CameraDirector.hpp"

#include <stdexcept>

BOOST_AUTO_TEST_SUITE(CameraDirectorTests)

BOOST_AUTO_TEST_CASE(baseline_applies_when_no_region_contains_the_focus) {
    CameraDirector director;
    director.setBaseline({1.5f, 4.0f, 0.1f});
    CameraFramingRegion region{};
    region.bounds = {0.0f, 0.0f, 100.0f, 100.0f};
    region.zoom = 0.7f;
    director.addRegion(region);

    const auto resolved = director.resolve({500.0f, 500.0f});
    BOOST_TEST(resolved.zoom == 1.5f);
    BOOST_TEST(resolved.damping == 4.0f);
    BOOST_TEST(resolved.lookAheadMultiplier == 0.1f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(hard_edged_region_overrides_fully_inside) {
    CameraDirector director;
    director.setBaseline({1.0f, 6.0f, 0.0f});
    CameraFramingRegion vista{};
    vista.bounds = {0.0f, 0.0f, 100.0f, 100.0f};
    vista.zoom = 0.65f;
    vista.damping = 3.0f;
    director.addRegion(vista);

    const auto resolved = director.resolve({50.0f, 50.0f});
    BOOST_TEST(resolved.zoom == 0.65f);
    BOOST_TEST(resolved.damping == 3.0f);
}

BOOST_AUTO_TEST_CASE(blend_margin_fades_influence_from_the_edge) {
    CameraDirector director;
    director.setBaseline({1.0f, 6.0f, 0.0f});
    CameraFramingRegion region{};
    region.bounds = {0.0f, -100.0f, 100.0f, 100.0f};
    region.blendMargin = 10.0f;
    region.zoom = 2.0f;
    director.addRegion(region);

    // 5 units inside the left edge: weight 0.5 -> zoom halfway between 1 and 2.
    const auto resolved = director.resolve({5.0f, 0.0f});
    BOOST_TEST(resolved.zoom == 1.5f, boost::test_tools::tolerance(0.0001f));
    // Deep inside: full weight.
    BOOST_TEST(director.resolve({50.0f, 0.0f}).zoom == 2.0f,
               boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(higher_priority_regions_compose_over_lower) {
    CameraDirector director;
    director.setBaseline({1.0f, 6.0f, 0.0f});
    CameraFramingRegion wide{};
    wide.bounds = {0.0f, 0.0f, 1000.0f, 1000.0f};
    wide.priority = 0;
    wide.zoom = 0.8f;
    director.addRegion(wide);
    CameraFramingRegion closeUp{};
    closeUp.bounds = {400.0f, 400.0f, 600.0f, 600.0f};
    closeUp.priority = 10;
    closeUp.zoom = 1.4f;
    director.addRegion(closeUp);

    BOOST_TEST(director.resolve({100.0f, 100.0f}).zoom == 0.8f);
    BOOST_TEST(director.resolve({500.0f, 500.0f}).zoom == 1.4f);
}

BOOST_AUTO_TEST_CASE(lock_y_rail_constrains_the_camera_after_update) {
    Camera camera{1280.0f, 720.0f};
    camera.getTransform().setPos({200.0f, 340.0f});

    CameraDirector director;
    director.setBaseline({1.0f, 6.0f, 0.0f});
    CameraFramingRegion rail{};
    rail.bounds = {0.0f, 0.0f, 1000.0f, 1000.0f};
    rail.lockY = 500.0f;
    director.addRegion(rail);

    director.apply(camera, {200.0f, 340.0f});
    camera.update(1.0 / 60.0);
    director.constrain(camera, {200.0f, 340.0f});

    BOOST_TEST(camera.getTransform().Position.y == 500.0f,
               boost::test_tools::tolerance(0.001f));
    BOOST_TEST(camera.getTransform().Position.x == 200.0f,
               boost::test_tools::tolerance(0.001f));
}

BOOST_AUTO_TEST_CASE(removing_a_region_restores_the_baseline) {
    CameraDirector director;
    director.setBaseline({1.0f, 6.0f, 0.0f});
    CameraFramingRegion region{};
    region.bounds = {0.0f, 0.0f, 10.0f, 10.0f};
    region.zoom = 3.0f;
    const int handle = director.addRegion(region);
    BOOST_TEST(director.resolve({5.0f, 5.0f}).zoom == 3.0f);
    director.removeRegion(handle);
    BOOST_TEST(director.resolve({5.0f, 5.0f}).zoom == 1.0f);
}

BOOST_AUTO_TEST_CASE(invalid_regions_and_baselines_are_rejected) {
    CameraDirector director;
    BOOST_CHECK_THROW(director.setBaseline({0.0f, 6.0f, 0.0f}),
                      std::invalid_argument);
    CameraFramingRegion inverted{};
    inverted.bounds = {10.0f, 0.0f, 0.0f, 10.0f};
    BOOST_CHECK_THROW(director.addRegion(inverted), std::invalid_argument);
    CameraFramingRegion badZoom{};
    badZoom.bounds = {0.0f, 0.0f, 1.0f, 1.0f};
    badZoom.zoom = -1.0f;
    BOOST_CHECK_THROW(director.addRegion(badZoom), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
