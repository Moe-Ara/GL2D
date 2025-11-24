#pragma once

#include <glm/vec2.hpp>

namespace PhysicsUnits {

// Number of in-world units that represent one meter. Adjust once, then define
// all physics constants (gravity, speeds, impulses) in meters.
constexpr float kUnitsPerMeter = 100.0f;
constexpr float kMetersPerUnit = 1.0f / kUnitsPerMeter;

inline float toUnits(float meters) { return meters * kUnitsPerMeter; }
inline glm::vec2 toUnits(const glm::vec2 &meters) { return meters * kUnitsPerMeter; }

inline float toMeters(float units) { return units * kMetersPerUnit; }
inline glm::vec2 toMeters(const glm::vec2 &units) { return units * kMetersPerUnit; }

} // namespace PhysicsUnits

