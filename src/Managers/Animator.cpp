//
// Created by Mohamad on 08/02/2025.
//

#include "Animator.hpp"

namespace Managers {
    Animator::Animator(std::shared_ptr<GameObjects::Sprite> sprite) : m_sprite(std::move(sprite)), m_currentAnimation(
            nullptr), m_currentFrameIndex(0), m_elapsedTime(0.0f), m_isPlaying(false) {

    }

    Animator::~Animator() {

    }

    void Animator::play(const std::shared_ptr<Graphics::Animation> &animation) {
        m_currentAnimation = animation;
        m_currentFrameIndex = 0;
        m_elapsedTime = 0.0f;
        m_isPlaying = true;
        updateSpriteUV();
    }

    void Animator::stop() {
        m_isPlaying = false;
        reset();
    }

    void Animator::pause() {
        m_isPlaying = false;
    }

    void Animator::resume() {
        m_isPlaying = true;
    }

    void Animator::update(float deltaTime) {
        if (!m_isPlaying || !m_currentAnimation) return;
        m_elapsedTime += deltaTime;
        if (m_elapsedTime >= m_currentAnimation->getFrameDuration()) {
            m_elapsedTime = 0.0f;
            m_currentFrameIndex++;
            if (m_currentFrameIndex >= m_currentAnimation->getFrameCount()) {
                if (m_currentAnimation->isLooping()) {
                    m_currentFrameIndex = 0;
                } else {
                    m_currentFrameIndex = m_currentAnimation->getFrameCount() - 1;
                    m_isPlaying = false;
                }
            }
            updateSpriteUV();
        }
    }

    void Animator::updateSpriteUV() {
        if (!m_currentAnimation) return;

        const auto& frame = m_currentAnimation->getFrame(m_currentFrameIndex);
        float frameWidth = 1.0f / m_currentAnimation->getTotalCols();
        float frameHeight = 1.0f / m_currentAnimation->getTotalRows();

        m_sprite->setUVCoords(glm::vec4(
                frame.column * frameWidth,
                frame.row * frameHeight,
                (frame.column + 1) * frameWidth,
                (frame.row + 1) * frameHeight
        ));
    }

    void Animator::reset() {
        m_currentFrameIndex = 0;
        m_elapsedTime = 0.0f;
        updateSpriteUV();
    }
} // Managers