#ifndef GL2D_ROPESEGMENTCOMPONENT_HPP
#define GL2D_ROPESEGMENTCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"

class Entity;

#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <utility>

class RopeSegmentComponent : public IComponent {
public:
    RopeSegmentComponent(glm::vec2 direction = glm::vec2{0.0f, -1.0f},
                         glm::vec2 ropeTop = glm::vec2{0.0f, 0.0f},
                         glm::vec2 ropeBottom = glm::vec2{0.0f, 0.0f})
        : m_ropeTop(ropeTop),
          m_ropeBottom(ropeBottom) {
        const float dirLen = glm::length(direction);
        if (dirLen > 1e-4f) {
            m_direction = direction / dirLen;
        }
        const glm::vec2 diff = m_ropeBottom - m_ropeTop;
        m_ropeLength = glm::length(diff);
        m_segmentLength = m_ropeLength;
    }

    ~RopeSegmentComponent() override = default;

    RopeSegmentComponent(const RopeSegmentComponent& other) = delete;
    RopeSegmentComponent& operator=(const RopeSegmentComponent& other) = delete;
    RopeSegmentComponent(RopeSegmentComponent&& other) = delete;
    RopeSegmentComponent& operator=(RopeSegmentComponent&& other) = delete;

    glm::vec2 direction() const { return m_direction; }
    glm::vec2 ropeTop() const { return m_ropeTop; }
    glm::vec2 ropeBottom() const { return m_ropeBottom; }
    float ropeLength() const { return m_ropeLength; }
    void setSegmentLength(float length);
    [[nodiscard]] float segmentLength() const noexcept { return m_segmentLength; }
    void setPrevious(Entity* previous) noexcept { m_previous = previous; }
    void setNext(Entity* next) noexcept { m_next = next; }
    [[nodiscard]] Entity* previous() const noexcept { return m_previous; }
    [[nodiscard]] Entity* next() const noexcept { return m_next; }
    // Returns the current world-space endpoints of this dynamic segment.
    [[nodiscard]] std::pair<glm::vec2, glm::vec2> worldEndpoints(
        const Entity& owner) const;

private:
    glm::vec2 m_direction{0.0f, -1.0f};
    glm::vec2 m_ropeTop{0.0f};
    glm::vec2 m_ropeBottom{0.0f};
    float m_ropeLength{0.0f};
    float m_segmentLength{0.0f};
    Entity* m_previous{nullptr};
    Entity* m_next{nullptr};
};

#endif // GL2D_ROPESEGMENTCOMPONENT_HPP
