#pragma once

#include <functional>
#include <vector>

namespace Engine {

enum class TimelineEase { Linear, SmoothStep, EaseIn, EaseOut };

// Deterministic track-based sequencer for cutscenes and scripted moments.
//
// A timeline is authored once (events, tweens, input-block windows) and then
// played; update(dt) advances it and fires everything crossed since the last
// update, in time order. Tweens receive an eased alpha in [0, 1] and are
// guaranteed a final apply(1) when their end is crossed, so end states are
// exact regardless of frame timing. Timelines drive cameras, feelings, audio,
// and entity motion through the callbacks they are authored with; the
// sequencer itself has no dependencies on those systems.
//
// Determinism contract: identical authored content advanced with identical
// delta sequences produces identical callback sequences.
class Timeline {
public:
    Timeline() = default;

    // Authoring (invalid arguments throw; authoring while playing throws).
    Timeline& event(double time, std::function<void()> action);
    Timeline& tween(double start, double end, TimelineEase ease,
                    std::function<void(float alpha)> apply);
    Timeline& blockInput(double start, double end);

    // Playback.
    void play();
    void stop();
    void update(double deltaSeconds);

    [[nodiscard]] bool playing() const noexcept { return m_playing; }
    [[nodiscard]] bool inputBlocked() const noexcept;
    [[nodiscard]] double elapsed() const noexcept { return m_elapsed; }
    [[nodiscard]] double duration() const noexcept { return m_duration; }

private:
    struct Event {
        double time{0.0};
        std::function<void()> action;
        bool fired{false};
    };
    struct Tween {
        double start{0.0};
        double end{0.0};
        TimelineEase ease{TimelineEase::Linear};
        std::function<void(float)> apply;
        bool completed{false};
    };
    struct Window {
        double start{0.0};
        double end{0.0};
    };

    void requireNotPlaying(const char* what) const;

    std::vector<Event> m_events;
    std::vector<Tween> m_tweens;
    std::vector<Window> m_inputBlocks;
    double m_duration{0.0};
    double m_elapsed{0.0};
    bool m_playing{false};
};

} // namespace Engine
