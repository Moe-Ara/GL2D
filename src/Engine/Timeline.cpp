#include "Engine/Timeline.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Engine {
namespace {

float applyEase(TimelineEase ease, float t) {
    switch (ease) {
    case TimelineEase::Linear: return t;
    case TimelineEase::SmoothStep: return t * t * (3.0f - 2.0f * t);
    case TimelineEase::EaseIn: return t * t;
    case TimelineEase::EaseOut: return 1.0f - (1.0f - t) * (1.0f - t);
    }
    return t;
}

} // namespace

void Timeline::requireNotPlaying(const char* what) const {
    if (m_playing) {
        throw std::logic_error(
            std::string{"Timeline cannot "} + what + " while playing");
    }
}

Timeline& Timeline::event(double time, std::function<void()> action) {
    requireNotPlaying("author an event");
    if (!std::isfinite(time) || time < 0.0) {
        throw std::invalid_argument("Timeline event time must be finite and non-negative");
    }
    if (!action) {
        throw std::invalid_argument("Timeline event requires an action");
    }
    m_events.push_back(Event{time, std::move(action), false});
    m_duration = std::max(m_duration, time);
    return *this;
}

Timeline& Timeline::tween(double start, double end, TimelineEase ease,
                          std::function<void(float)> apply) {
    requireNotPlaying("author a tween");
    if (!std::isfinite(start) || !std::isfinite(end) || start < 0.0 || end < start) {
        throw std::invalid_argument(
            "Timeline tween requires finite 0 <= start <= end");
    }
    if (!apply) {
        throw std::invalid_argument("Timeline tween requires an apply function");
    }
    m_tweens.push_back(Tween{start, end, ease, std::move(apply), false});
    m_duration = std::max(m_duration, end);
    return *this;
}

Timeline& Timeline::blockInput(double start, double end) {
    requireNotPlaying("author an input block");
    if (!std::isfinite(start) || !std::isfinite(end) || start < 0.0 || end < start) {
        throw std::invalid_argument(
            "Timeline input block requires finite 0 <= start <= end");
    }
    m_inputBlocks.push_back(Window{start, end});
    m_duration = std::max(m_duration, end);
    return *this;
}

void Timeline::play() {
    m_elapsed = 0.0;
    for (Event& event : m_events) {
        event.fired = false;
    }
    for (Tween& tween : m_tweens) {
        tween.completed = false;
    }
    // Events and tweens fire in time order regardless of authoring order.
    std::stable_sort(m_events.begin(), m_events.end(),
                     [](const Event& a, const Event& b) { return a.time < b.time; });
    m_playing = true;
}

void Timeline::stop() {
    m_playing = false;
}

bool Timeline::inputBlocked() const noexcept {
    if (!m_playing) {
        return false;
    }
    for (const Window& window : m_inputBlocks) {
        if (m_elapsed >= window.start && m_elapsed <= window.end) {
            return true;
        }
    }
    return false;
}

void Timeline::update(double deltaSeconds) {
    if (!m_playing) {
        return;
    }
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
        throw std::invalid_argument(
            "Timeline update requires a finite, non-negative delta");
    }

    m_elapsed += deltaSeconds;

    for (Event& event : m_events) {
        if (!event.fired && event.time <= m_elapsed) {
            event.fired = true;
            event.action();
        }
    }

    for (Tween& tween : m_tweens) {
        if (tween.completed || m_elapsed < tween.start) {
            continue;
        }
        if (m_elapsed >= tween.end) {
            tween.apply(1.0f);
            tween.completed = true;
            continue;
        }
        const double span = tween.end - tween.start;
        const float t = span <= 0.0
            ? 1.0f
            : static_cast<float>((m_elapsed - tween.start) / span);
        tween.apply(applyEase(tween.ease, t));
    }

    if (m_elapsed >= m_duration) {
        m_playing = false;
    }
}

} // namespace Engine
