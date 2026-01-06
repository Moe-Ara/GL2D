#include "AnimatorComponent.hpp"
#include "GameObjects/Sprite.hpp"

namespace {
std::shared_ptr<Graphics::Animator> wrapRawAnimator(Graphics::Animator *animator) {
    if (!animator) {
        return nullptr;
    }
    return std::shared_ptr<Graphics::Animator>(animator, [](Graphics::Animator *) {
        // Non-owning wrapper.
    });
}
} // namespace

AnimatorComponent::AnimatorComponent(std::shared_ptr<Graphics::Animator> animator)
    : m_animator(std::move(animator)) {}

AnimatorComponent::AnimatorComponent(Graphics::Animator *animator)
    : AnimatorComponent(wrapRawAnimator(animator)) {}

void AnimatorComponent::setAnimator(std::shared_ptr<Graphics::Animator> animator) {
    m_animator = std::move(animator);
}

void AnimatorComponent::setSprite(std::shared_ptr<GameObjects::Sprite> sprite) {
    m_sprite = std::move(sprite);
    if (!m_animator && m_sprite) {
        m_animator = std::make_shared<Graphics::Animator>(m_sprite);
    }
}

void AnimatorComponent::play(const std::shared_ptr<Graphics::Animation> &animation,
                             float /*crossfadeDuration*/) {
    if (!animation) {
        return;
    }
    ensureAnimator();
    if (m_animator) {
        m_animator->play(animation);
    }
}

void AnimatorComponent::ensureAnimator() {
    if (m_animator || !m_sprite) {
        return;
    }
    m_animator = std::make_shared<Graphics::Animator>(m_sprite);
}

void AnimatorComponent::update(Entity &/*owner*/, double dt) {
    ensureAnimator();
    if (m_animator) {
        m_animator->update(static_cast<float>(dt));
    }
}
