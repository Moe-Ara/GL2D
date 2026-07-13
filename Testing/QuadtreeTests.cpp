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

BOOST_AUTO_TEST_CASE(merging_after_removal_preserves_deep_descendants) {
    Quadtree::Config config{};
    config.maxDepth = 6;
    config.maxObjectsPerNode = 2;
    config.minSize = 0.1f;
    Quadtree tree(AABB{{0.0f, 0.0f}, {64.0f, 64.0f}}, config);
    int users[5]{};
    tree.insert(AABB{{1.0f, 1.0f}, {1.1f, 1.1f}}, &users[0]);
    tree.insert(AABB{{2.0f, 2.0f}, {2.1f, 2.1f}}, &users[1]);
    tree.insert(AABB{{3.0f, 3.0f}, {3.1f, 3.1f}}, &users[2]);
    tree.insert(AABB{{50.0f, 50.0f}, {51.0f, 51.0f}}, &users[3]);
    tree.insert(AABB{{55.0f, 55.0f}, {56.0f, 56.0f}}, &users[4]);

    tree.remove(&users[3]);
    tree.remove(&users[4]);

    std::vector<void*> hits;
    tree.query(AABB{{0.0f, 0.0f}, {10.0f, 10.0f}}, hits);
    BOOST_TEST(hits.size() == 3u);
}

BOOST_AUTO_TEST_SUITE_END()
