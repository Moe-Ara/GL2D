#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace Engine {

class FixedStepClock {
public:
    struct Config {
        double stepSeconds{1.0 / 120.0};
        double maxFrameSeconds{0.25};
        std::uint32_t maxStepsPerFrame{16};
    };

    struct Result {
        std::uint32_t steps{0};
        double interpolationAlpha{0.0};
        double droppedSeconds{0.0};
    };

    FixedStepClock() = default;
    explicit FixedStepClock(Config config) { configure(config); }

    void configure(Config config) {
        if (!std::isfinite(config.stepSeconds) || config.stepSeconds <= 0.0 ||
            !std::isfinite(config.maxFrameSeconds) || config.maxFrameSeconds <= 0.0 ||
            config.maxStepsPerFrame == 0) {
            throw std::invalid_argument("FixedStepClock requires positive finite timing values");
        }
        m_config = config;
        reset();
    }

    template<typename StepFunction>
    Result advance(double frameSeconds, StepFunction&& stepFunction) {
        if (!std::isfinite(frameSeconds) || frameSeconds < 0.0) {
            throw std::invalid_argument("FixedStepClock frame time must be finite and non-negative");
        }

        Result result{};
        const double acceptedFrame = std::min(frameSeconds, m_config.maxFrameSeconds);
        result.droppedSeconds = frameSeconds - acceptedFrame;
        m_accumulator += acceptedFrame;
        constexpr double epsilon = 1e-12;
        while (m_accumulator + epsilon >= m_config.stepSeconds &&
               result.steps < m_config.maxStepsPerFrame) {
            stepFunction(static_cast<float>(m_config.stepSeconds));
            m_accumulator -= m_config.stepSeconds;
            m_elapsedSeconds += m_config.stepSeconds;
            ++result.steps;
        }

        if (m_accumulator + epsilon >= m_config.stepSeconds) {
            const double wholeSteps = std::floor(
                (m_accumulator + epsilon) / m_config.stepSeconds);
            const double droppedBacklog = wholeSteps * m_config.stepSeconds;
            m_accumulator = std::max(0.0, m_accumulator - droppedBacklog);
            result.droppedSeconds += droppedBacklog;
        }

        m_droppedSeconds += result.droppedSeconds;

        result.interpolationAlpha = std::clamp(
            m_accumulator / m_config.stepSeconds, 0.0, 1.0);
        return result;
    }

    void reset() noexcept { m_accumulator = 0.0; }

    [[nodiscard]] double stepSeconds() const noexcept { return m_config.stepSeconds; }
    [[nodiscard]] double interpolationAlpha() const noexcept {
        return std::clamp(m_accumulator / m_config.stepSeconds, 0.0, 1.0);
    }
    [[nodiscard]] double elapsedSeconds() const noexcept { return m_elapsedSeconds; }
    [[nodiscard]] double droppedSeconds() const noexcept { return m_droppedSeconds; }

private:
    Config m_config{};
    double m_accumulator{0.0};
    double m_elapsedSeconds{0.0};
    double m_droppedSeconds{0.0};
};

} // namespace Engine
