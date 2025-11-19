#include "AnimationMetadataLoader.hpp"

#include <fstream>
#include <sstream>

namespace Loaders {

    namespace {
        float extractNumber(const Utils::JsonValue &node, float fallback) {
            if (node.isNumber()) {
                return static_cast<float>(node.asNumber());
            }
            return fallback;
        }

        int extractInt(const Utils::JsonValue &node, int fallback) {
            if (node.isNumber()) {
                return static_cast<int>(node.asNumber());
            }
            return fallback;
        }
    }

    AnimationMetadataFile AnimationMetadataLoader::loadFromFile(const std::string &filepath) {
        std::ifstream input(filepath);
        if (!input.is_open()) {
            throw Utils::JsonParseException("Failed to open animation metadata file: " + filepath);
        }
        std::stringstream buffer;
        buffer << input.rdbuf();
        auto root = Utils::JsonValue::parse(buffer.str());
        return parseDocument(root);
    }

    AnimationMetadataFile AnimationMetadataLoader::parseDocument(const Utils::JsonValue &root) {
        if (!root.isObject()) {
            throw Utils::JsonParseException("Animation metadata file must contain a JSON object at the root");
        }
        AnimationMetadataFile file;
        if (root.hasKey("defaultFrameDuration")) {
            file.defaultFrameDuration = extractNumber(root.at("defaultFrameDuration"), file.defaultFrameDuration);
        }
        if (root.hasKey("atlas")) {
            file.atlas = parseAtlas(root.at("atlas"));
        }
        if (root.hasKey("initialState")) {
            file.initialState = root.at("initialState").asString();
        }
        if (!root.hasKey("animations")) {
            throw Utils::JsonParseException("Animation metadata file is missing the 'animations' array");
        }

        for (const auto &animationNode: root.at("animations").asArray()) {
            file.animations.push_back(parseAnimation(animationNode, file.defaultFrameDuration));
        }
        return file;
    }

    AtlasMetadata AnimationMetadataLoader::parseAtlas(const Utils::JsonValue &root) {
        AtlasMetadata atlas{};
        if (!root.isObject()) {
            throw Utils::JsonParseException("Atlas entry must be a JSON object");
        }
        if (root.hasKey("texture")) {
            atlas.texturePath = root.at("texture").asString();
        }
        if (root.hasKey("rows")) {
            atlas.rows = extractInt(root.at("rows"), atlas.rows);
        }
        if (root.hasKey("cols")) {
            atlas.cols = extractInt(root.at("cols"), atlas.cols);
        }
        return atlas;
    }

    AnimationMetadataEntry
    AnimationMetadataLoader::parseAnimation(const Utils::JsonValue &node, float fallbackDuration) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Animation entry must be a JSON object");
        }
        AnimationMetadataEntry entry{};
        if (!node.hasKey("name")) {
            throw Utils::JsonParseException("Animation entry is missing the 'name' field");
        }
        entry.name = node.at("name").asString();
        if (node.hasKey("loop")) {
            entry.loop = node.at("loop").asBoolean();
        }
        if (node.hasKey("frameDuration")) {
            entry.defaultFrameDuration = extractNumber(node.at("frameDuration"), fallbackDuration);
        } else {
            entry.defaultFrameDuration = fallbackDuration;
        }
        if (node.hasKey("playback")) {
            entry.playbackMode = parsePlaybackMode(node.at("playback").asString());
        }
        if (!node.hasKey("frames")) {
            throw Utils::JsonParseException("Animation entry '" + entry.name + "' must contain a 'frames' array");
        }
        for (const auto &frameNode: node.at("frames").asArray()) {
            entry.frames.push_back(parseFrame(frameNode, entry.defaultFrameDuration));
        }
        if (node.hasKey("transitions")) {
            for (const auto &transitionNode: node.at("transitions").asArray()) {
                entry.transitions.push_back(parseTransition(transitionNode));
            }
        }
        return entry;
    }

    FrameMetadata AnimationMetadataLoader::parseFrame(const Utils::JsonValue &node, float fallbackDuration) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Frame entry must be a JSON object");
        }
        FrameMetadata frame{};
        if (node.hasKey("row") && node.hasKey("col")) {
            frame.row = extractInt(node.at("row"), 0);
            frame.column = extractInt(node.at("col"), 0);
            frame.hasGridCoordinates = true;
        }
        if (node.hasKey("uv") && node.at("uv").isArray()) {
            const auto &uv = node.at("uv").asArray();
            if (uv.size() == 4) {
                frame.uvRect = glm::vec4(
                        static_cast<float>(uv[0].asNumber()),
                        static_cast<float>(uv[1].asNumber()),
                        static_cast<float>(uv[2].asNumber()),
                        static_cast<float>(uv[3].asNumber())
                );
                frame.useCustomUV = true;
            }
        }
        if (node.hasKey("duration")) {
            frame.duration = extractNumber(node.at("duration"), fallbackDuration);
        } else {
            frame.duration = fallbackDuration;
        }
        if (node.hasKey("texture")) {
            frame.texturePath = node.at("texture").asString();
        }
        if (node.hasKey("event")) {
            frame.eventName = node.at("event").asString();
        }
        return frame;
    }

    TransitionMetadata AnimationMetadataLoader::parseTransition(const Utils::JsonValue &node) {
        if (!node.isObject()) {
            throw Utils::JsonParseException("Transition entry must be a JSON object");
        }
        TransitionMetadata transition{};
        if (node.hasKey("target")) {
            transition.target = node.at("target").asString();
        }
        if (node.hasKey("condition")) {
            transition.condition = node.at("condition").asString();
        }
        if (transition.target.empty()) {
            throw Utils::JsonParseException("Transition entries require a 'target' field");
        }
        return transition;
    }

    Graphics::PlaybackMode AnimationMetadataLoader::parsePlaybackMode(const std::string &value) {
        if (value == "reverse") return Graphics::PlaybackMode::Reverse;
        if (value == "pingpong") return Graphics::PlaybackMode::PingPong;
        return Graphics::PlaybackMode::Forward;
    }

} // namespace Loaders
