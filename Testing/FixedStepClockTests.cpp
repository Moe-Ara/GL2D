#include <boost/test/unit_test.hpp>

#include "Engine/FixedStepClock.hpp"

BOOST_AUTO_TEST_SUITE(FixedStepClockTests)

BOOST_AUTO_TEST_CASE(accumulates_frames_into_deterministic_steps) {
    Engine::FixedStepClock clock{{0.01, 0.25, 16}};
    int calls = 0;
    double simulated = 0.0;

    const auto first = clock.advance(0.006, [&](float dt) { ++calls; simulated += dt; });
    BOOST_TEST(first.steps == 0u);
    const auto second = clock.advance(0.025, [&](float dt) { ++calls; simulated += dt; });

    BOOST_TEST(second.steps == 3u);
    BOOST_TEST(calls == 3);
    BOOST_TEST(simulated == 0.03, boost::test_tools::tolerance(1e-5));
    BOOST_TEST(second.interpolationAlpha == 0.1, boost::test_tools::tolerance(1e-5));
}

BOOST_AUTO_TEST_CASE(limits_catch_up_and_reports_dropped_time) {
    Engine::FixedStepClock clock{{0.01, 1.0, 2}};
    const auto result = clock.advance(0.1, [](float) {});

    BOOST_TEST(result.steps == 2u);
    BOOST_TEST(result.droppedSeconds == 0.08, boost::test_tools::tolerance(1e-5));
    BOOST_TEST(result.interpolationAlpha < 1e-5);
}

BOOST_AUTO_TEST_CASE(rejects_invalid_frame_times) {
    Engine::FixedStepClock clock;
    BOOST_CHECK_THROW(clock.advance(-0.1, [](float) {}), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
