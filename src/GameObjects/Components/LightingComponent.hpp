//
// Created by Mohamad on 26/11/2025.
//

#ifndef GL2D_LIGHTINGCOMPONENT_HPP
#define GL2D_LIGHTINGCOMPONENT_HPP

#include "GameObjects/IComponent.hpp"
#include "Graphics/LightingSystem/LightType.hpp"
#include "Graphics/LightingSystem/LightEffector.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <optional>

class LightingComponent : public IComponent {
public:
    LightingComponent() = default;

    explicit LightingComponent(const std::string &id,
                               LightType type = LightType::POINT,
                               glm::vec2 pos = {0, 0},
                               float radius = 4.0f,
                               glm::vec3 color = {1, 1, 1},
                               float intensity = 1.0f,
                               float falloff = 2.0f,
                               float emissiveBoost = 0.0f,
                               glm::vec2 dir = {0.0f, -1.0f},
                               float innerCutoff = 0.9f,
                               float outerCutoff = 0.7f,
                               const std::string& cookiePath = "",
                               float cookieStrength = 0.0f,
                               std::optional<LightEffector> effector = std::nullopt);

    ~LightingComponent() override = default;

    LightingComponent(const LightingComponent &other) = delete;
    LightingComponent &operator=(const LightingComponent &other) = delete;
    LightingComponent(LightingComponent &&other) = delete;
    LightingComponent &operator=(LightingComponent &&other) = delete;

    const std::string &id() const;
    LightType type() const;
    glm::vec2 localPos() const;
    float radius() const;
    glm::vec3 color() const;
    float intensity() const;
    float falloff() const;
    float emissiveBoost() const;
    glm::vec2 direction() const;
    float innerCutoff() const;
    float outerCutoff() const;
    const std::string& cookiePath() const;
    float cookieStrength() const;
    const std::optional<LightEffector>& effector() const { return m_effector; }

    void setLocalPos(glm::vec2 position);
    void setRadius(float radius);
    void setColor(glm::vec3 color);
    void setIntensity(float intensity);
    void setFalloff(float falloff);
    void setEmissiveBoost(float emissiveBoost);
    void setDirection(glm::vec2 dir);
    void setCutoff(float inner, float outer);
    void setCookie(const std::string& path, float strength);
    void setEffector(const std::optional<LightEffector>& eff);

private:
    std::string m_id{};
    LightType m_type{LightType::POINT};
    glm::vec2 m_localPos{0.0f};
    glm::vec3 m_color{1.0f};
    float m_radius{4.0f};
    float m_intensity{1.0f};
    float m_falloff{2.0f};
    float m_emissiveBoost{0.0f};
    glm::vec2 m_dir{0.0f, -1.0f};
    float m_innerCutoff{0.9f};
    float m_outerCutoff{0.7f};
    std::string m_cookiePath{};
    float m_cookieStrength{0.0f};
    std::optional<LightEffector> m_effector{};
};

#endif //GL2D_LIGHTINGCOMPONENT_HPP
