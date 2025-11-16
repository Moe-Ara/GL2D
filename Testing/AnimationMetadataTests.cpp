#define BOOST_TEST_MODULE AnimationMetadataTests
#include <boost/test/unit_test.hpp>

#include "Loaders/AnimationMetadataLoader.hpp"

BOOST_AUTO_TEST_CASE(can_parse_metadata_file) {
    auto metadata = Loaders::AnimationMetadataLoader::loadFromFile("assets/character/animations.json");
    BOOST_TEST(!metadata.animations.empty());
    BOOST_TEST(metadata.animations.front().name == "SlimeIdle");
    BOOST_TEST(metadata.atlas.cols >= 1);
}
