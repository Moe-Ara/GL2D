#include <boost/test/unit_test.hpp>

#include <vector>

#include "Physics/Quadtree.hpp"

BOOST_AUTO_TEST_SUITE(QuadtreeTests)

BOOST_AUTO_TEST_CASE(insert_and_query_returns_overlapping_items) {
    Quadtree::Config cfg{};
    cfg.maxDepth = 4;
    cfg.maxObjectsPerNode = 1;
    cfg.minSize = 1.0f;
    Quadtree tree(AABB({0.0f, 0.0f}, {8.0f, 8.0f}), cfg);

    int a = 1;
    int b = 2;
    tree.insert(AABB({0.0f, 0.0f}, {1.0f, 1.0f}), &a);
    tree.insert(AABB({6.0f, 6.0f}, {7.0f, 7.0f}), &b);

    std::vector<void*> hits;
    tree.query(AABB({-1.0f, -1.0f}, {2.0f, 2.0f}), hits);

    BOOST_TEST(hits.size() == 1u);
    BOOST_TEST(hits[0] == &a);
}

BOOST_AUTO_TEST_CASE(remove_erases_from_queries) {
    Quadtree tree(AABB({0.0f, 0.0f}, {4.0f, 4.0f}));
    int id = 42;
    tree.insert(AABB({1.0f, 1.0f}, {2.0f, 2.0f}), &id);

    std::vector<void*> hits;
    tree.query(AABB({0.0f, 0.0f}, {3.0f, 3.0f}), hits);
    BOOST_TEST(hits.size() == 1u);

    tree.remove(&id);
    hits.clear();
    tree.query(AABB({0.0f, 0.0f}, {3.0f, 3.0f}), hits);
    BOOST_TEST(hits.empty());
}

BOOST_AUTO_TEST_CASE(update_moves_item_between_regions) {
    Quadtree tree(AABB({0.0f, 0.0f}, {8.0f, 8.0f}));
    int id = 7;
    AABB oldBox({0.0f, 0.0f}, {1.0f, 1.0f});
    AABB newBox({5.0f, 5.0f}, {6.0f, 6.0f});
    tree.insert(oldBox, &id);

    std::vector<void*> hits;
    tree.query(AABB({0.0f, 0.0f}, {2.0f, 2.0f}), hits);
    BOOST_TEST(hits.size() == 1u);
    hits.clear();

    tree.update(oldBox, newBox, &id);

    tree.query(AABB({0.0f, 0.0f}, {2.0f, 2.0f}), hits);
    BOOST_TEST(hits.empty());
    hits.clear();
    tree.query(AABB({4.5f, 4.5f}, {6.5f, 6.5f}), hits);
    BOOST_TEST(hits.size() == 1u);
    BOOST_TEST(hits[0] == &id);
}

BOOST_AUTO_TEST_SUITE_END()
