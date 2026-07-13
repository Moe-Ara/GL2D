#pragma once

#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Entity.hpp"

#include <ostream>

namespace ECS {
inline std::ostream& operator<<(std::ostream& os, const Entity& entity) {
    return os << "Entity(" << entity.index() << ", gen " << entity.generation() << ")";
}

inline std::ostream& operator<<(std::ostream& os, LocomotionState state) {
    switch (state) {
    case LocomotionState::Idle: return os << "Idle";
    case LocomotionState::Running: return os << "Running";
    case LocomotionState::Rising: return os << "Rising";
    case LocomotionState::Falling: return os << "Falling";
    case LocomotionState::Climbing: return os << "Climbing";
    }
    return os << "Unknown";
}
} // namespace ECS
