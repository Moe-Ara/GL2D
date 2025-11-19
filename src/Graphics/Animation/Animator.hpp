//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_ANIMATOR_HPP
#define GL2D_ANIMATOR_HPP

#include "Animation.hpp"
#include "GameObjects/Sprite.hpp"
#include <functional>
#include <memory>

namespace Graphics {

class Animator {
public:
  Animator(std::shared_ptr<GameObjects::Sprite> sprite);

  virtual ~Animator();

  Animator(const Animator &other) = delete;

  Animator &operator=(const Animator &other) = delete;

  Animator(Animator &&other) = delete;

  Animator &operator=(Animator &&other) = delete;

  void play(const std::shared_ptr<Graphics::Animation> &animation);

  void stop();

  void pause();

  void resume();

  void update(float deltaTime);

  std::shared_ptr<Graphics::Animation> getCurrentAnimation() const;

  const Graphics::Frame *getCurrentFrame() const;

  void reset();

  using FrameChangedCallback =
      std::function<void(const std::shared_ptr<Graphics::Animation> &, size_t,
                         const Graphics::Frame &)>;
  using AnimationFinishedCallback =
      std::function<void(const std::shared_ptr<Graphics::Animation> &)>;
  using FrameEventCallback =
      std::function<void(const std::string &,
                         const std::shared_ptr<Graphics::Animation> &, size_t)>;

  void setFrameChangedCallback(FrameChangedCallback callback);
  void setAnimationFinishedCallback(AnimationFinishedCallback callback);
  void setFrameEventCallback(FrameEventCallback callback);

private:
  void updateSpriteUV();
  void dispatchFrameChanged(const Graphics::Frame &frame);
  void dispatchAnimationFinished();
  void advanceFrame();
  void configurePlaybackStart();
  float resolveFrameDuration(const Graphics::Frame &frame) const;

  std::shared_ptr<GameObjects::Sprite> m_sprite;
  std::shared_ptr<Graphics::Animation> m_currentAnimation;
  size_t m_currentFrameIndex;
  float m_elapsedTime;
  bool m_isPlaying;
  int m_frameDirection{1};
  bool m_hasNotifiedCompletion{false};
  FrameChangedCallback m_frameChangedCallback;
  AnimationFinishedCallback m_animationFinishedCallback;
  FrameEventCallback m_frameEventCallback;
};

} // namespace Graphics

#endif // GL2D_ANIMATOR_HPP
