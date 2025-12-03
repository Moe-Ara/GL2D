#define BOOST_TEST_MODULE AnimationMetadataTests
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>

#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"

BOOST_AUTO_TEST_CASE(can_parse_metadata_file) {
    auto metadata = Loaders::AnimationMetadataLoader::loadFromFile("assets/character/animations.json");
    BOOST_TEST(!metadata.animations.empty());
    BOOST_TEST(metadata.animations.front().name == "Idle");
    BOOST_TEST(metadata.atlas.cols >= 1);
}

BOOST_AUTO_TEST_CASE(respects_defaults_and_frame_overrides) {
    const std::string json = R"({
        "defaultFrameDuration": 0.25,
        "atlas": { "texture": "atlas.png", "rows": 4, "cols": 8 },
        "initialState": "Idle",
        "animations": [
            {
                "name": "Idle",
                "loop": false,
                "playback": "pingpong",
                "frames": [
                    { "row": 1, "col": 2 },
                    { "uv": [0.0, 0.0, 0.5, 0.5], "duration": 0.1, "texture": "custom.png", "event": "blink" }
                ],
                "transitions": [{ "target": "Run", "condition": "speed>0" }]
            }
        ]
    })";

    const auto path = std::filesystem::temp_directory_path() / "gl2d_animation_test.json";
    {
        std::ofstream out(path);
        out << json;
    }

    auto metadata = Loaders::AnimationMetadataLoader::loadFromFile(path.string());
    std::filesystem::remove(path);

    BOOST_TEST(metadata.defaultFrameDuration == 0.25f);
    BOOST_TEST(metadata.initialState == "Idle");
    BOOST_TEST(metadata.atlas.texturePath == "atlas.png");
    BOOST_TEST(metadata.atlas.rows == 4);
    BOOST_TEST(metadata.atlas.cols == 8);
    BOOST_REQUIRE_EQUAL(metadata.animations.size(), 1);

    const auto &anim = metadata.animations.front();
    BOOST_TEST(anim.name == "Idle");
    BOOST_TEST(!anim.loop);
    BOOST_TEST(static_cast<int>(anim.playbackMode) == static_cast<int>(Graphics::PlaybackMode::PingPong));
    BOOST_TEST(anim.defaultFrameDuration == 0.25f);
    BOOST_REQUIRE_EQUAL(anim.frames.size(), 2);

    const auto &gridFrame = anim.frames[0];
    BOOST_TEST(gridFrame.hasGridCoordinates);
    BOOST_TEST(gridFrame.row == 1);
    BOOST_TEST(gridFrame.column == 2);
    BOOST_TEST(!gridFrame.useCustomUV);
    BOOST_TEST(gridFrame.duration == 0.25f);

    const auto &customFrame = anim.frames[1];
    BOOST_TEST(customFrame.useCustomUV);
    BOOST_TEST(customFrame.uvRect.x == 0.0f);
    BOOST_TEST(customFrame.uvRect.z == 0.5f);
    BOOST_TEST(customFrame.duration == 0.1f);
    BOOST_TEST(customFrame.texturePath == "custom.png");
    BOOST_TEST(customFrame.eventName == "blink");

    BOOST_REQUIRE_EQUAL(anim.transitions.size(), 1);
    BOOST_TEST(anim.transitions[0].target == "Run");
    BOOST_TEST(anim.transitions[0].condition == "speed>0");
}
