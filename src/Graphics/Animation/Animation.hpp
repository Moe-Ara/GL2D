//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_ANIMATION_HPP
#define GL2D_ANIMATION_HPP

#include <string>
#include <vector>
#include <memory>

#include "Frame.hpp"

namespace Graphics {

    enum class PlaybackMode {
        Forward,
        Reverse,
        PingPong
    };

    struct AnimationTransition {
        std::string target;
        std::string condition;
    };

    class Animation {
    public:
        Animation(int totalRows, int totalCols, float frameDuration, bool loop = true,
                  PlaybackMode playbackMode = PlaybackMode::Forward);
        Animation(const std::string &folderPath, float frameDuration, bool loop = true,
                  PlaybackMode playbackMode = PlaybackMode::Forward);

        virtual ~Animation();

        Animation(const Animation &other) = delete;

        Animation &operator=(const Animation &other) = delete;

        Animation(Animation &&other) = delete;

        Animation &operator=(Animation &&other) = delete;

        void addFrame(int row, int column);
        void addFrame(Frame frame);
        const Frame &getFrame(size_t index) const;

        void addTransition(AnimationTransition transition);
        const std::vector<AnimationTransition> &getTransitions() const;

        void setName(std::string name);
        const std::string &getName() const;

        size_t getFrameCount() const;

        float getFrameDuration() const;

        void setFrameDuration(float duration);

        bool isLooping() const;
        void setLooping(bool looping);

        int getTotalRows() const;
        int getTotalCols() const;

        PlaybackMode getPlaybackMode() const;
        void setPlaybackMode(PlaybackMode playbackMode);

        void setSharedTexture(std::shared_ptr<GameObjects::Texture> texture);
        std::shared_ptr<GameObjects::Texture> getSharedTexture() const;

    private:
        std::vector<Frame> m_frames{};
        int m_totalRows{};
        int m_totalCols{};
        float m_frameDuration{};
        bool m_loop{};
        PlaybackMode m_playbackMode{PlaybackMode::Forward};
        std::shared_ptr<GameObjects::Texture> m_sharedTexture{nullptr};
        std::vector<AnimationTransition> m_transitions;
        std::string m_name;
    };

} // Graphics

#endif //GL2D_ANIMATION_HPP
