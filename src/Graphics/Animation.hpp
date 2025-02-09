//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_ANIMATION_HPP
#define GL2D_ANIMATION_HPP

#include <vector>
#include <string>
#include "Frame.hpp"

namespace Graphics {

    class Animation {
    public:
        Animation(int totalRows, int totalCols, float frameDuration, bool loop = true);
        Animation(const std::string& folderPath, float frameDuration, bool loop=true);

        virtual ~Animation();

        Animation(const Animation &other) = delete;

        Animation &operator=(const Animation &other) = delete;

        Animation(Animation &&other) = delete;

        Animation &operator=(Animation &&other) = delete;

        void addFrame(int row, int column);
        void addFrame(Frame frame);
        const Frame &getFrame(size_t index) const;

        size_t getFrameCount() const;

        float getFrameDuration() const;

        bool isLooping() const;
        int getTotalRows();
        int getTotalCols();

    private:
        std::vector<Frame> m_frames{};
        int m_totalRows{};
        int m_totalCols{};
        float m_frameDuration{};
        bool m_loop{};
    };

} // Graphics

#endif //GL2D_ANIMATION_HPP
