#ifndef GL2D_ANIMATIONMETADATALOADER_HPP
#define GL2D_ANIMATIONMETADATALOADER_HPP

#include <string>
#include <vector>
#include <optional>

#include <glm/vec4.hpp>

#include "Graphics/Animation/Animation.hpp"
#include "Utils/SimpleJson.hpp"

namespace Loaders {

    struct FrameMetadata {
        int row{};
        int column{};
        bool hasGridCoordinates{false};
        bool useCustomUV{false};
        glm::vec4 uvRect{0.0f, 0.0f, 1.0f, 1.0f};
        float duration{-1.0f};
        std::string texturePath;
        std::string eventName;
    };

    struct TransitionMetadata {
        std::string target;
        std::string condition;
    };

    struct AnimationMetadataEntry {
        std::string name;
        bool loop{true};
        float defaultFrameDuration{-1.0f};
        Graphics::PlaybackMode playbackMode{Graphics::PlaybackMode::Forward};
        std::vector<FrameMetadata> frames;
        std::vector<TransitionMetadata> transitions;
    };

    struct AtlasMetadata {
        std::string texturePath;
        int rows{1};
        int cols{1};
    };

    struct AnimationMetadataFile {
        float defaultFrameDuration{0.1f};
        AtlasMetadata atlas;
        std::string initialState;
        std::vector<AnimationMetadataEntry> animations;
    };

    class AnimationMetadataLoader {
    public:
        static AnimationMetadataFile loadFromFile(const std::string &filepath);

    private:
        static AnimationMetadataFile parseDocument(const Utils::JsonValue &root);
        static AtlasMetadata parseAtlas(const Utils::JsonValue &root);
        static AnimationMetadataEntry parseAnimation(const Utils::JsonValue &node, float fallbackDuration);
        static FrameMetadata parseFrame(const Utils::JsonValue &node, float fallbackDuration);
        static TransitionMetadata parseTransition(const Utils::JsonValue &node);
        static Graphics::PlaybackMode parsePlaybackMode(const std::string &value);
    };

} // namespace Loaders

#endif //GL2D_ANIMATIONMETADATALOADER_HPP
