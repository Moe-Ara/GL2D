#include <boost/test/unit_test.hpp>

#include <cmath>
#include <limits>
#include <stdexcept>

#include <glm/geometric.hpp>

#include "AISystem/NavMesh/NavRaster.hpp"
#include "AISystem/NavMesh/PolyNavMesh.hpp"

namespace {
constexpr float kTolerance = 1e-4f;

bool near(const glm::vec2& a, const glm::vec2& b) {
    return glm::length(a - b) <= kTolerance;
}

bool pathStaysOnWalkableCells(const NavPath& path, const NavRaster& raster) {
    if (path.points.empty()) return false;
    for (std::size_t segment = 1; segment < path.points.size(); ++segment) {
        const glm::vec2 from = path.points[segment - 1];
        const glm::vec2 to = path.points[segment];
        constexpr int sampleCount = 80;
        for (int sample = 0; sample < sampleCount; ++sample) {
            // Midpoint samples avoid assigning exact shared edges to either cell.
            const float t = (static_cast<float>(sample) + 0.5f) /
                            static_cast<float>(sampleCount);
            const glm::vec2 point = from + (to - from) * t;
            int x = -1;
            int y = -1;
            if (!raster.worldToCell(point, x, y) || !raster.isWalkable(x, y)) {
                return false;
            }
        }
    }
    return true;
}
}

BOOST_AUTO_TEST_SUITE(NavMeshTests)

BOOST_AUTO_TEST_CASE(raster_rejects_invalid_geometry_before_allocating) {
    BOOST_CHECK_THROW(NavRaster(-1, 10, 1.0f, glm::vec2{0.0f}), std::invalid_argument);
    BOOST_CHECK_THROW(NavRaster(10, 0, 1.0f, glm::vec2{0.0f}), std::invalid_argument);
    BOOST_CHECK_THROW(NavRaster(10, 10, 0.0f, glm::vec2{0.0f}), std::invalid_argument);
    BOOST_CHECK_THROW(
        NavRaster(10, 10, std::numeric_limits<float>::quiet_NaN(), glm::vec2{0.0f}),
        std::invalid_argument);
    BOOST_CHECK_THROW(
        NavRaster(10, 10, 1.0f,
                  glm::vec2{std::numeric_limits<float>::infinity(), 0.0f}),
        std::invalid_argument);
    BOOST_CHECK_THROW(
        NavRaster(std::numeric_limits<int>::max(), 1,
                  std::numeric_limits<float>::max(), glm::vec2{0.0f}),
        std::overflow_error);
}

BOOST_AUTO_TEST_CASE(world_to_cell_uses_floor_and_reports_failure) {
    NavRaster raster(2, 2, 1.0f, glm::vec2{10.0f, 20.0f});
    int x = 99;
    int y = 99;

    BOOST_TEST(raster.worldToCell(glm::vec2{10.25f, 20.75f}, x, y));
    BOOST_TEST(x == 0);
    BOOST_TEST(y == 0);

    BOOST_TEST(!raster.worldToCell(glm::vec2{9.99f, 20.5f}, x, y));
    BOOST_TEST(x == -1);
    BOOST_TEST(y == -1);
    BOOST_TEST(!raster.worldToCell(glm::vec2{12.0f, 20.5f}, x, y));
    BOOST_TEST(!raster.worldToCell(
        glm::vec2{std::numeric_limits<float>::quiet_NaN(), 20.5f}, x, y));
}

BOOST_AUTO_TEST_CASE(cell_access_is_checked_and_predicate_build_is_validated) {
    NavRaster raster(2, 2, 2.0f, glm::vec2{-1.0f});
    BOOST_CHECK_THROW(raster.setWalkable(2, 0, false), std::out_of_range);
    BOOST_CHECK_THROW(raster.cellCenter(-1, 0), std::out_of_range);
    BOOST_CHECK_THROW(raster.cellBounds(0, 2), std::out_of_range);
    BOOST_CHECK_THROW(raster.cellToWorld(3, 3), std::out_of_range);
    BOOST_CHECK_THROW(NavRaster::buildFromPredicate(2, 2, 1.0f, glm::vec2{0.0f}, {}),
                      std::invalid_argument);

    const NavRaster filtered = NavRaster::buildFromPredicate(
        2, 1, 1.0f, glm::vec2{0.0f},
        [](const NavAABB& bounds) { return bounds.min.x < 1.0f; });
    BOOST_TEST(filtered.isWalkable(0, 0));
    BOOST_TEST(!filtered.isWalkable(1, 0));
}

BOOST_AUTO_TEST_CASE(navmesh_finds_direct_path_inside_one_polygon) {
    NavRaster raster(4, 3, 1.0f, glm::vec2{0.0f});
    PolyNavMesh mesh;
    mesh.buildFromRaster(raster);

    const glm::vec2 start{0.25f, 0.5f};
    const glm::vec2 end{3.75f, 2.5f};
    const NavPath path = mesh.findPath(start, end);
    BOOST_REQUIRE(path.valid());
    BOOST_TEST(path.points.size() == 2u);
    BOOST_TEST(near(path.points.front(), start));
    BOOST_TEST(near(path.points.back(), end));
}

BOOST_AUTO_TEST_CASE(navmesh_routes_around_an_obstacle_without_cutting_it) {
    NavRaster raster(3, 3, 1.0f, glm::vec2{0.0f});
    raster.setWalkable(1, 1, false);
    PolyNavMesh mesh;
    mesh.buildFromRaster(raster);

    const glm::vec2 start{0.5f, 1.5f};
    const glm::vec2 end{2.5f, 1.5f};
    const NavPath path = mesh.findPath(start, end);
    BOOST_REQUIRE(path.valid());
    BOOST_TEST(near(path.points.front(), start));
    BOOST_TEST(near(path.points.back(), end));
    BOOST_TEST(pathStaysOnWalkableCells(path, raster));

    // The build should be stable: identical input produces identical waypoints.
    const NavPath repeated = mesh.findPath(start, end);
    BOOST_REQUIRE(repeated.points.size() == path.points.size());
    for (std::size_t i = 0; i < path.points.size(); ++i) {
        BOOST_TEST(near(repeated.points[i], path.points[i]));
    }
}

BOOST_AUTO_TEST_CASE(navmesh_returns_no_path_for_invalid_or_blocked_endpoints) {
    NavRaster raster(2, 1, 1.0f, glm::vec2{0.0f});
    raster.setWalkable(1, 0, false);
    PolyNavMesh mesh;
    mesh.buildFromRaster(raster);

    BOOST_TEST(!mesh.findPath(glm::vec2{0.25f, 0.5f}, glm::vec2{1.5f, 0.5f}).valid());
    BOOST_TEST(!mesh.findPath(glm::vec2{-1.0f}, glm::vec2{0.5f}).valid());
    BOOST_TEST(!mesh.findPath(
        glm::vec2{std::numeric_limits<float>::quiet_NaN(), 0.0f}, glm::vec2{0.5f})
                    .valid());
}

BOOST_AUTO_TEST_SUITE_END()
