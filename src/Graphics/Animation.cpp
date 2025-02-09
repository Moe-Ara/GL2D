//
// Created by Mohamad on 08/02/2025.
//

#include <filesystem>
#include "Animation.hpp"
#include "Managers/TextureManager.hpp"

namespace Graphics {
} // Graphics
Graphics::Animation::Animation(int totalRows, int totalCols, float frameDuration, bool loop) :
        m_totalRows(totalRows), m_totalCols(totalCols), m_frameDuration(frameDuration), m_loop(loop) {

}

Graphics::Animation::Animation(const std::string &folderPath, float frameDuration, bool loop):m_frameDuration(frameDuration),m_loop(loop){
    for(const auto& entry: std::filesystem::directory_iterator(folderPath)){
        if(entry.path().extension()==".png"||entry.path().extension()==".jpg"||entry.path().extension()==".jpeg"){
            auto texture = Managers::TextureManager::loadTexture(entry.path().string());
            addFrame(Graphics::Frame{0, static_cast<int>(m_frames.size())}, texture);
        }
    }

}
Graphics::Animation::~Animation() {

}

void Graphics::Animation::addFrame(int row, int column) {
    m_frames.push_back({row, column});
}

const Graphics::Frame &Graphics::Animation::getFrame(size_t index) const {
    return m_frames[index % m_frames.size()];
}

size_t Graphics::Animation::getFrameCount() const {
    return m_frames.size();
}

void Graphics::Animation::addFrame(Graphics::Frame frame) {
    m_frames.push_back(frame);

}


float Graphics::Animation::getFrameDuration() const {
    return m_frameDuration;
}

int Graphics::Animation::getTotalRows() {
    return m_totalRows;
}

int Graphics::Animation::getTotalCols() {
    return m_totalCols;
}

bool Graphics::Animation::isLooping() const {
    return m_loop;
}
