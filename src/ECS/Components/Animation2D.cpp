#include "ECS/Components/Animation2D.hpp"

namespace ECS {

bool AnimationParameters2D::getBool(std::string_view name, bool fallback) const {
    const auto found = boolValues.find(std::string{name});
    return found == boolValues.end() ? fallback : found->second;
}

float AnimationParameters2D::getFloat(std::string_view name, float fallback) const {
    const auto found = floatValues.find(std::string{name});
    return found == floatValues.end() ? fallback : found->second;
}

} // namespace ECS
