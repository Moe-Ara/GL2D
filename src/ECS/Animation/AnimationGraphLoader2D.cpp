#include "ECS/Animation/AnimationGraphLoader2D.hpp"

#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ECS {
namespace {
AnimationPlayback2D playbackFor(const Loaders::AnimationMetadataEntry& entry) {
    if (entry.playbackMode == Graphics::PlaybackMode::PingPong) {
        return AnimationPlayback2D::PingPong;
    }
    if (entry.playbackMode == Graphics::PlaybackMode::Reverse) {
        return entry.loop ? AnimationPlayback2D::LoopReverse
                          : AnimationPlayback2D::OnceReverse;
    }
    return entry.loop ? AnimationPlayback2D::Loop : AnimationPlayback2D::Once;
}

std::string compact(std::string expression) {
    std::erase_if(expression, [](unsigned char character) {
        return std::isspace(character) != 0;
    });
    return expression;
}

float parseNumber(const std::string& value, const std::string& expression) {
    try {
        std::size_t consumed = 0;
        const float result = std::stof(value, &consumed);
        if (consumed != value.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::invalid_argument("Invalid animation transition condition: " + expression);
    }
}

AnimationTransition2D parseTransition(const std::string& source,
                                      const Loaders::TransitionMetadata& metadata) {
    AnimationTransition2D result{};
    result.fromState = source;
    result.toState = metadata.target;
    const std::string expression = compact(metadata.condition);
    if (expression.empty() || expression == "true") {
        return result;
    }
    if (expression.front() == '!' && expression.find_first_of("<>=", 1) == std::string::npos) {
        result.condition = AnimationCondition2D::BoolEquals;
        result.parameter = expression.substr(1);
        result.expectedBool = false;
        return result;
    }

    struct Operator {
        const char* text;
        AnimationCondition2D condition;
    };
    constexpr Operator operators[] = {
        {">=", AnimationCondition2D::FloatGreaterEqual},
        {"<=", AnimationCondition2D::FloatLessEqual},
        {">", AnimationCondition2D::FloatGreater},
        {"<", AnimationCondition2D::FloatLess}
    };
    for (const Operator& comparison : operators) {
        const std::size_t position = expression.find(comparison.text);
        if (position == std::string::npos) {
            continue;
        }
        result.parameter = expression.substr(0, position);
        result.threshold = parseNumber(
            expression.substr(position + std::char_traits<char>::length(comparison.text)),
            expression);
        result.condition = comparison.condition;
        if (result.parameter.empty()) {
            throw std::invalid_argument("Invalid animation transition condition: " + expression);
        }
        return result;
    }

    const std::size_t equality = expression.find("==");
    if (equality != std::string::npos) {
        const std::string value = expression.substr(equality + 2);
        if (value != "true" && value != "false") {
            throw std::invalid_argument(
                "Animation boolean equality must compare with true or false: " + expression);
        }
        result.condition = AnimationCondition2D::BoolEquals;
        result.parameter = expression.substr(0, equality);
        result.expectedBool = value == "true";
        return result;
    }

    result.condition = AnimationCondition2D::BoolEquals;
    result.parameter = expression;
    result.expectedBool = true;
    return result;
}
}

std::shared_ptr<const AnimationGraph2D> AnimationGraphLoader2D::loadFromFile(
    const std::string& metadataPath, const TextureResolver& textureResolver) {
    const Loaders::AnimationMetadataFile metadata =
        Loaders::AnimationMetadataLoader::loadFromFile(metadataPath);
    if (metadata.atlas.rows <= 0 || metadata.atlas.cols <= 0) {
        throw std::invalid_argument("Animation atlas rows and columns must be positive");
    }

    const auto atlasTexture = !metadata.atlas.texturePath.empty() && textureResolver
        ? textureResolver(metadata.atlas.texturePath) : nullptr;
    std::vector<AnimationState2D> states;
    std::vector<AnimationTransition2D> transitions;
    states.reserve(metadata.animations.size());

    for (const Loaders::AnimationMetadataEntry& entry : metadata.animations) {
        AnimationClip2D clip{};
        clip.playback = playbackFor(entry);
        clip.frames.reserve(entry.frames.size());
        for (const Loaders::FrameMetadata& sourceFrame : entry.frames) {
            AnimationFrame2D frame{};
            frame.durationSeconds = sourceFrame.duration;
            frame.event = sourceFrame.eventName;
            frame.texture = !sourceFrame.texturePath.empty() && textureResolver
                ? textureResolver(sourceFrame.texturePath) : atlasTexture;
            frame.normalTexture = !sourceFrame.normalTexturePath.empty() && textureResolver
                ? textureResolver(sourceFrame.normalTexturePath) : nullptr;
            if (sourceFrame.useCustomUV) {
                frame.uvRect = sourceFrame.uvRect;
            } else {
                const float width = 1.0f / static_cast<float>(metadata.atlas.cols);
                const float height = 1.0f / static_cast<float>(metadata.atlas.rows);
                frame.uvRect = {
                    sourceFrame.column * width, sourceFrame.row * height,
                    (sourceFrame.column + 1) * width,
                    (sourceFrame.row + 1) * height
                };
            }
            clip.frames.push_back(std::move(frame));
        }
        for (const Loaders::TransitionMetadata& transition : entry.transitions) {
            transitions.push_back(parseTransition(entry.name, transition));
        }
        states.push_back({entry.name, std::move(clip)});
    }

    const std::string initialState = !metadata.initialState.empty()
        ? metadata.initialState
        : (states.empty() ? std::string{} : states.front().name);
    return std::make_shared<const AnimationGraph2D>(
        std::move(states), std::move(transitions), initialState);
}

} // namespace ECS
