#include <boost/test/unit_test.hpp>

#include "ECS/Animation/AnimationGraphLoader2D.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(ECSAnimationLoaderTests)

BOOST_AUTO_TEST_CASE(loads_atlas_frames_playback_events_and_compiled_transitions) {
    const std::string document = R"({
        "defaultFrameDuration": 0.1,
        "atlas": {"texture": "hero.png", "rows": 2, "cols": 4},
        "initialState": "Idle",
        "animations": [
            {
                "name": "Idle", "loop": true, "frames": [
                    {"row": 1, "col": 2, "event": "blink"}
                ],
                "transitions": [{"target": "Run", "condition": "speed >= 1.5"}]
            },
            {
                "name": "Run", "loop": false, "playback": "reverse",
                "frames": [{"uv": [0.0, 0.0, 0.5, 1.0], "duration": 0.05}]
            }
        ]
    })";
    const auto path = std::filesystem::temp_directory_path() / "gl2d_ecs_animation.json";
    {
        std::ofstream output(path);
        output << document;
    }
    std::vector<std::string> requestedTextures;
    const auto graph = ECS::AnimationGraphLoader2D::loadFromFile(
        path.string(), [&](const std::string& texture) {
            requestedTextures.push_back(texture);
            return std::shared_ptr<GameObjects::Texture>{};
        });
    std::filesystem::remove(path);

    BOOST_REQUIRE(graph);
    BOOST_TEST(graph->state(graph->initialStateIndex()).name == "Idle");
    const auto idle = graph->findState("Idle");
    const auto run = graph->findState("Run");
    BOOST_REQUIRE(idle);
    BOOST_REQUIRE(run);
    BOOST_TEST(graph->state(*idle).clip.frames[0].uvRect.x == 0.5f);
    BOOST_TEST(graph->state(*idle).clip.frames[0].uvRect.y == 0.5f);
    BOOST_TEST(graph->state(*idle).clip.frames[0].event == "blink");
    BOOST_TEST(static_cast<int>(graph->state(*run).clip.playback) ==
               static_cast<int>(ECS::AnimationPlayback2D::OnceReverse));
    BOOST_REQUIRE_EQUAL(graph->transitions().size(), 1u);
    BOOST_TEST(graph->transitions()[0].fromState == *idle);
    BOOST_TEST(graph->transitions()[0].toState == *run);
    BOOST_TEST(graph->transitions()[0].threshold == 1.5f);
    BOOST_TEST(requestedTextures.size() == 1u);
    BOOST_TEST(requestedTextures.front() == "hero.png");
}

BOOST_AUTO_TEST_SUITE_END()
