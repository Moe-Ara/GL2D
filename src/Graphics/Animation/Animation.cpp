//
// Created by Mohamad on 08/02/2025.
//

#include <filesystem>
#include "Animation.hpp"
#include "Managers/TextureManager.hpp"
#include <utility>

namespace Graphics {
} // Graphics
Graphics::Animation::Animation(int totalRows, int totalCols, float frameDuration, bool loop, PlaybackMode playbackMode) :
        m_totalRows(totalRows), m_totalCols(totalCols), m_frameDuration(frameDuration), m_loop(loop),
        m_playbackMode(playbackMode) {

}

Graphics::Animation::Animation(const std::string &folderPath, float frameDuration, bool loop, PlaybackMode playbackMode)
        : m_totalRows(1), m_totalCols(1), m_frameDuration(frameDuration), m_loop(loop),
          m_playbackMode(playbackMode) {
    for (const auto &entry: std::filesystem::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg" ||
            entry.path().extension() == ".jpeg") {
            auto texture = Managers::TextureManager::loadTexture(entry.path().string());
            Graphics::Frame frame{};
            frame.row = 0;
            frame.column = static_cast<int>(m_frames.size());
            frame.texture = texture;
            frame.duration = frameDuration;
            addFrame(frame);
        }
    }
}
Graphics::Animation::~Animation() {

}

void Graphics::Animation::addFrame(int row, int column) {
    Graphics::Frame frame{};
    frame.row = row;
    frame.column = column;
    frame.duration = m_frameDuration;
    frame.texture = m_sharedTexture;
    m_frames.push_back(frame);
}

const Graphics::Frame &Graphics::Animation::getFrame(size_t index) const {
    return m_frames[index % m_frames.size()];
}

size_t Graphics::Animation::getFrameCount() const {
    return m_frames.size();
}

void Graphics::Animation::addFrame(Graphics::Frame frame) {
    if (!frame.texture) {
        frame.texture = m_sharedTexture;
    }
    m_frames.push_back(frame);

}

void Graphics::Animation::setName(std::string name) {
    m_name = std::move(name);
}

const std::string &Graphics::Animation::getName() const {
    return m_name;
}


float Graphics::Animation::getFrameDuration() const {
    return m_frameDuration;
}

void Graphics::Animation::setFrameDuration(float duration) {
    m_frameDuration = duration;
}

int Graphics::Animation::getTotalRows() const {
    return m_totalRows;
}

int Graphics::Animation::getTotalCols() const {
    return m_totalCols;
}

bool Graphics::Animation::isLooping() const {
    return m_loop;
}

void Graphics::Animation::setLooping(bool looping) {
    m_loop = looping;
}

Graphics::PlaybackMode Graphics::Animation::getPlaybackMode() const {
    return m_playbackMode;
}

void Graphics::Animation::setPlaybackMode(Graphics::PlaybackMode playbackMode) {
    m_playbackMode = playbackMode;
}

void Graphics::Animation::setSharedTexture(std::shared_ptr<GameObjects::Texture> texture) {
    m_sharedTexture = std::move(texture);
}

std::shared_ptr<GameObjects::Texture> Graphics::Animation::getSharedTexture() const {
    return m_sharedTexture;
}

void Graphics::Animation::addTransition(AnimationTransition transition) {
    m_transitions.push_back(std::move(transition));
}

const std::vector<Graphics::AnimationTransition> &Graphics::Animation::getTransitions() const {
    return m_transitions;
}
