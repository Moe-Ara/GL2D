#include <boost/test/unit_test.hpp>

#include "Engine/Timeline.hpp"

#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_SUITE(TimelineTests)

BOOST_AUTO_TEST_CASE(events_fire_once_in_time_order_across_updates) {
    Engine::Timeline timeline;
    std::vector<int> fired;
    timeline.event(0.5, [&] { fired.push_back(2); });
    timeline.event(0.0, [&] { fired.push_back(1); });
    timeline.event(1.0, [&] { fired.push_back(3); });

    timeline.play();
    timeline.update(0.25); // fires t=0
    timeline.update(0.5);  // crosses t=0.5
    timeline.update(0.5);  // crosses t=1.0

    const std::vector<int> expected{1, 2, 3};
    BOOST_TEST(fired == expected, boost::test_tools::per_element());
    BOOST_TEST(!timeline.playing());

    // Re-running produces the identical sequence (determinism contract).
    fired.clear();
    timeline.play();
    timeline.update(2.0);
    BOOST_TEST(fired == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(tweens_reach_exact_endpoint_regardless_of_frame_timing) {
    Engine::Timeline timeline;
    float lastAlpha = -1.0f;
    int applies = 0;
    timeline.tween(1.0, 2.0, Engine::TimelineEase::Linear, [&](float alpha) {
        lastAlpha = alpha;
        ++applies;
    });

    timeline.play();
    timeline.update(0.5); // before start: no apply
    BOOST_TEST(applies == 0);
    timeline.update(1.0); // t=1.5, mid-tween
    BOOST_TEST(lastAlpha == 0.5f, boost::test_tools::tolerance(0.0001f));
    timeline.update(10.0); // far past the end: final apply is exactly 1
    BOOST_TEST(lastAlpha == 1.0f);
    timeline.update(1.0); // completed tweens never re-apply
    const int appliesAfterEnd = applies;
    timeline.update(1.0);
    BOOST_TEST(applies == appliesAfterEnd);
}

BOOST_AUTO_TEST_CASE(smoothstep_ease_is_monotonic_with_fixed_endpoints) {
    Engine::Timeline timeline;
    std::vector<float> samples;
    timeline.tween(0.0, 1.0, Engine::TimelineEase::SmoothStep,
                   [&](float alpha) { samples.push_back(alpha); });
    timeline.play();
    for (int i = 0; i < 10; ++i) {
        timeline.update(0.1);
    }
    BOOST_REQUIRE(!samples.empty());
    for (std::size_t i = 1; i < samples.size(); ++i) {
        BOOST_TEST(samples[i] >= samples[i - 1]);
    }
    BOOST_TEST(samples.back() == 1.0f);
}

BOOST_AUTO_TEST_CASE(input_block_windows_report_only_while_playing_inside) {
    Engine::Timeline timeline;
    timeline.blockInput(0.5, 1.5);
    timeline.event(2.0, [] {});

    BOOST_TEST(!timeline.inputBlocked()); // not playing
    timeline.play();
    timeline.update(0.25);
    BOOST_TEST(!timeline.inputBlocked());
    timeline.update(0.5); // t=0.75, inside window
    BOOST_TEST(timeline.inputBlocked());
    timeline.update(1.0); // t=1.75, past window
    BOOST_TEST(!timeline.inputBlocked());
    timeline.stop();
    BOOST_TEST(!timeline.inputBlocked());
}

BOOST_AUTO_TEST_CASE(stop_halts_playback_and_authoring_while_playing_throws) {
    Engine::Timeline timeline;
    int fired = 0;
    timeline.event(1.0, [&] { ++fired; });
    timeline.play();
    BOOST_CHECK_THROW(timeline.event(2.0, [] {}), std::logic_error);
    timeline.stop();
    timeline.update(5.0); // stopped: nothing fires
    BOOST_TEST(fired == 0);
}

BOOST_AUTO_TEST_CASE(invalid_authoring_is_rejected) {
    Engine::Timeline timeline;
    BOOST_CHECK_THROW(timeline.event(-1.0, [] {}), std::invalid_argument);
    BOOST_CHECK_THROW(timeline.event(1.0, nullptr), std::invalid_argument);
    BOOST_CHECK_THROW(
        timeline.tween(2.0, 1.0, Engine::TimelineEase::Linear, [](float) {}),
        std::invalid_argument);
    BOOST_CHECK_THROW(timeline.blockInput(3.0, 1.0), std::invalid_argument);
    timeline.play();
    BOOST_CHECK_THROW(timeline.update(-0.1), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
