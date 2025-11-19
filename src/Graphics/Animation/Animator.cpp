//
// Created by Mohamad on 08/02/2025.
//

#include "Animator.hpp"

#include <algorithm>

namespace Graphics {
Animator::Animator(std::shared_ptr<GameObjects::Sprite> sprite)
    : m_sprite(std::move(sprite)), m_currentAnimation(nullptr),
      m_currentFrameIndex(0), m_elapsedTime(0.0f), m_isPlaying(false) {}

Animator::~Animator() {}

void Animator::play(const std::shared_ptr<Graphics::Animation> &animation) {
  if (!animation || animation->getFrameCount() == 0) {
    m_currentAnimation.reset();
    m_isPlaying = false;
    return;
  }

  m_currentAnimation = animation;
  m_isPlaying = true;
  configurePlaybackStart();
  updateSpriteUV();
  dispatchFrameChanged(m_currentAnimation->getFrame(m_currentFrameIndex));
}

void Animator::stop() {
  m_isPlaying = false;
  reset();
}

void Animator::pause() { m_isPlaying = false; }

void Animator::resume() { m_isPlaying = true; }

void Animator::update(float deltaTime) {
  if (!m_isPlaying || !m_currentAnimation)
    return;
  if (m_currentAnimation->getFrameCount() == 0)
    return;
  m_elapsedTime += deltaTime;
  float frameDuration =
      resolveFrameDuration(m_currentAnimation->getFrame(m_currentFrameIndex));
  while (m_elapsedTime >= frameDuration && m_isPlaying) {
    m_elapsedTime -= frameDuration;
    advanceFrame();
    if (!m_isPlaying) {
      break;
    }
    frameDuration =
        resolveFrameDuration(m_currentAnimation->getFrame(m_currentFrameIndex));
    if (frameDuration <= 0.0f) {
      break;
    }
  }
}

std::shared_ptr<Graphics::Animation> Animator::getCurrentAnimation() const {
  return m_currentAnimation;
}

const Graphics::Frame *Animator::getCurrentFrame() const {
  if (!m_currentAnimation || m_currentAnimation->getFrameCount() == 0) {
    return nullptr;
  }
  return &m_currentAnimation->getFrame(m_currentFrameIndex);
}

void Animator::setFrameChangedCallback(FrameChangedCallback callback) {
  m_frameChangedCallback = std::move(callback);
}

void Animator::setAnimationFinishedCallback(
    AnimationFinishedCallback callback) {
  m_animationFinishedCallback = std::move(callback);
}

void Animator::setFrameEventCallback(FrameEventCallback callback) {
  m_frameEventCallback = std::move(callback);
}

void Animator::updateSpriteUV() {
  if (!m_currentAnimation || m_currentAnimation->getFrameCount() == 0)
    return;

  const auto &frame = m_currentAnimation->getFrame(m_currentFrameIndex);
  auto texture =
      frame.texture ? frame.texture : m_currentAnimation->getSharedTexture();
  m_sprite->setTexture(texture);

  float frameWidth =
      1.0f /
      static_cast<float>(std::max(1, m_currentAnimation->getTotalCols()));
  float frameHeight =
      1.0f /
      static_cast<float>(std::max(1, m_currentAnimation->getTotalRows()));

  glm::vec4 uv = frame.useCustomUV ? frame.uvRect
                                   : glm::vec4(frame.column * frameWidth,
                                               frame.row * frameHeight,
                                               (frame.column + 1) * frameWidth,
                                               (frame.row + 1) * frameHeight);
  m_sprite->setUVCoords(uv);
}

void Animator::reset() {
  m_currentFrameIndex = 0;
  m_elapsedTime = 0.0f;
  m_frameDirection = 1;
  m_hasNotifiedCompletion = false;
  if (m_currentAnimation) {
    configurePlaybackStart();
    updateSpriteUV();
  }
}

void Animator::dispatchFrameChanged(const Graphics::Frame &frame) {
  if (m_frameChangedCallback && m_currentAnimation) {
    m_frameChangedCallback(m_currentAnimation, m_currentFrameIndex, frame);
  }
  if (m_frameEventCallback && m_currentAnimation && !frame.eventName.empty()) {
    m_frameEventCallback(frame.eventName, m_currentAnimation,
                         m_currentFrameIndex);
  }
}

void Animator::dispatchAnimationFinished() {
  if (m_hasNotifiedCompletion || !m_animationFinishedCallback ||
      !m_currentAnimation) {
    return;
  }
  m_hasNotifiedCompletion = true;
  m_animationFinishedCallback(m_currentAnimation);
}

void Animator::advanceFrame() {
  if (!m_currentAnimation || m_currentAnimation->getFrameCount() == 0) {
    return;
  }
  const auto frameCount = m_currentAnimation->getFrameCount();
  if (frameCount <= 1) {
    if (!m_currentAnimation->isLooping()) {
      m_isPlaying = false;
      dispatchAnimationFinished();
    }
    return;
  }

  size_t nextIndex = m_currentFrameIndex;
  bool frameChanged = false;
  switch (m_currentAnimation->getPlaybackMode()) {
  case Graphics::PlaybackMode::Forward:
    if (m_currentFrameIndex + 1 >= frameCount) {
      if (m_currentAnimation->isLooping()) {
        nextIndex = 0;
        frameChanged = true;
      } else {
        m_isPlaying = false;
      }
    } else {
      nextIndex = m_currentFrameIndex + 1;
      frameChanged = true;
    }
    break;
  case Graphics::PlaybackMode::Reverse:
    if (m_currentFrameIndex == 0) {
      if (m_currentAnimation->isLooping()) {
        nextIndex = frameCount - 1;
        frameChanged = true;
      } else {
        m_isPlaying = false;
      }
    } else {
      nextIndex = m_currentFrameIndex - 1;
      frameChanged = true;
    }
    break;
  case Graphics::PlaybackMode::PingPong:
    if (m_frameDirection > 0) {
      if (m_currentFrameIndex + 1 >= frameCount) {
        if (m_currentAnimation->isLooping()) {
          m_frameDirection = -1;
          nextIndex = frameCount - 2;
          frameChanged = true;
        } else {
          m_isPlaying = false;
        }
      } else {
        nextIndex = m_currentFrameIndex + 1;
        frameChanged = true;
      }
    } else {
      if (m_currentFrameIndex == 0) {
        if (m_currentAnimation->isLooping()) {
          m_frameDirection = 1;
          nextIndex = 1;
          frameChanged = true;
        } else {
          m_isPlaying = false;
        }
      } else {
        nextIndex = m_currentFrameIndex - 1;
        frameChanged = true;
      }
    }
    break;
  }

  if (!m_isPlaying) {
    dispatchAnimationFinished();
    return;
  }

  if (frameChanged) {
    m_currentFrameIndex = nextIndex;
    updateSpriteUV();
    dispatchFrameChanged(m_currentAnimation->getFrame(m_currentFrameIndex));
  }
}

void Animator::configurePlaybackStart() {
  m_elapsedTime = 0.0f;
  m_hasNotifiedCompletion = false;
  if (!m_currentAnimation || m_currentAnimation->getFrameCount() == 0) {
    m_currentFrameIndex = 0;
    m_frameDirection = 1;
    return;
  }
  switch (m_currentAnimation->getPlaybackMode()) {
  case Graphics::PlaybackMode::Forward:
  case Graphics::PlaybackMode::PingPong:
    m_currentFrameIndex = 0;
    m_frameDirection = 1;
    break;
  case Graphics::PlaybackMode::Reverse:
    m_currentFrameIndex = m_currentAnimation->getFrameCount() - 1;
    m_frameDirection = -1;
    break;
  }
}

float Animator::resolveFrameDuration(const Graphics::Frame &frame) const {
  float duration =
      frame.duration > 0.0f
          ? frame.duration
          : (m_currentAnimation ? m_currentAnimation->getFrameDuration()
                                : 0.016f);
  if (duration <= 0.0f) {
    duration = 0.016f;
  }
  return duration;
}
} // namespace Graphics
