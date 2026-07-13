//
// Created by Codex on 27/11/2025.
//

#ifndef GL2D_TRANSFORMFOLLOWERCOMPONENT_HPP
#define GL2D_TRANSFORMFOLLOWERCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "GameObjects/Components/TransformComponent.hpp"

#include <glm/vec2.hpp>

class TransformFollowerComponent : public IUpdatableComponent {
public:
    TransformFollowerComponent() = default;
    ~TransformFollowerComponent() override = default;

    TransformFollowerComponent(const TransformFollowerComponent &) = delete;
    TransformFollowerComponent &operator=(const TransformFollowerComponent &) = delete;
    TransformFollowerComponent(TransformFollowerComponent &&) = delete;
    TransformFollowerComponent &operator=(TransformFollowerComponent &&) = delete;

    void setTarget(Entity *target) { m_target = target; }
    [[nodiscard]] Entity* target() const noexcept { return m_target; }
    void setOffset(const glm::vec2 &offset) { m_offset = offset; }
    void setCopyRotation(bool copy) { m_copyRotation = copy; }

    void update(Entity &owner, double /*dt*/) override;

private:
    Entity *m_target{nullptr};
    glm::vec2 m_offset{0.0f, 0.0f};
    bool m_copyRotation{true};
};

#endif // GL2D_TRANSFORMFOLLOWERCOMPONENT_HPP
