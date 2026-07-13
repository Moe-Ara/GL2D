#include <boost/test/unit_test.hpp>

#include "Physics/BroadphaseBVH.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_SUITE(BroadphaseBVHTests)

BOOST_AUTO_TEST_CASE(emits_each_overlapping_pair_exactly_once) {
    int users[7]{};
    BroadphaseBVH bvh;
    bvh.build({
        {AABB{{0.0f, 0.0f}, {2.0f, 2.0f}}, &users[0]},
        {AABB{{1.0f, 1.0f}, {3.0f, 3.0f}}, &users[1]},
        {AABB{{10.0f, 0.0f}, {12.0f, 2.0f}}, &users[2]},
        {AABB{{11.0f, 1.0f}, {13.0f, 3.0f}}, &users[3]},
        {AABB{{1000.0f, -1.0f}, {1001.0f, 1.0f}}, &users[4]},
        {AABB{{-1000.0f, -1.0f}, {-999.0f, 1.0f}}, &users[5]},
        {AABB{{50.0f, 50.0f}, {51.0f, 51.0f}}, &users[6]}
    });

    const auto pairs = bvh.overlappingPairs();
    BOOST_TEST(pairs.size() == 2u);
    BOOST_TEST(std::ranges::count_if(pairs, [&](const auto& pair) {
        return pair.first == &users[0] && pair.second == &users[1];
    }) == 1);
    BOOST_TEST(std::ranges::count_if(pairs, [&](const auto& pair) {
        return pair.first == &users[2] && pair.second == &users[3];
    }) == 1);
}

BOOST_AUTO_TEST_CASE(query_handles_worlds_without_authored_bounds) {
    int near = 0;
    int far = 0;
    BroadphaseBVH bvh;
    bvh.build({
        {AABB{{-1.0f, -1.0f}, {1.0f, 1.0f}}, &near},
        {AABB{{1000000.0f, 0.0f}, {1000001.0f, 1.0f}}, &far}
    });

    std::vector<void*> hits;
    bvh.query(AABB{{-2.0f, -2.0f}, {2.0f, 2.0f}}, hits);
    BOOST_REQUIRE(hits.size() == 1u);
    BOOST_TEST(hits.front() == &near);
}

BOOST_AUTO_TEST_CASE(rejects_ambiguous_user_identity) {
    int user = 0;
    BroadphaseBVH bvh;
    BOOST_CHECK_THROW(bvh.build({
        {AABB{{0.0f, 0.0f}, {1.0f, 1.0f}}, &user},
        {AABB{{2.0f, 2.0f}, {3.0f, 3.0f}}, &user}
    }), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
