//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLERENDERER_HPP
#define GL2D_PARTICLERENDERER_HPP
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <vector>
#include "FeelingsSystem/FeelingSnapshot.hpp"
#include "RenderingSystem/ParticleBlendMode.hpp"

namespace Graphics { class Shader; }
namespace GameObjects { class Texture; }

namespace Rendering{
    struct ParticleRenderData {
        glm::vec2 position;
        glm::vec2 size;
        float rotation;
        glm::vec4 color;
    };
    class ParticleRenderer {
    public:
        ParticleRenderer();

        ~ParticleRenderer();

        ParticleRenderer(const ParticleRenderer &other) = delete;

        ParticleRenderer &operator=(const ParticleRenderer &other) = delete;

        ParticleRenderer(ParticleRenderer &&other) = delete;

        ParticleRenderer &operator=(ParticleRenderer &&other) = delete;

        void begin(const glm::mat4& viewProjection = glm::mat4(1.0f),
                   std::optional<glm::vec4> viewBounds = std::nullopt);
        void submit(const ParticleRenderData& p);
        void end();
        // Changing mode flushes the current batch so submission order is kept.
        void setBlendMode(ParticleBlendMode mode);
        // A null texture selects the built-in soft radial particle.
        void setTexture(const GameObjects::Texture* texture);
        void setBorder(const glm::vec4& color, float thickness);
        void applyFeeling(const FeelingsSystem::FeelingSnapshot& snapshot);

    private:
        void flush();
        void restoreBlendState() noexcept;
        void destroyResources() noexcept;

        std::vector<ParticleRenderData> m_batch{};
        unsigned int m_vao=0;
        unsigned int m_vbo=0;
        unsigned int m_ebo=0;

        unsigned int m_defaultTexture=0;
        std::shared_ptr<Graphics::Shader> m_shader;
        glm::mat4 m_viewProj{1.0f};
        std::optional<glm::vec4> m_viewBounds{};
        glm::vec4 m_borderColor{0.0f};
        float m_borderThickness{0.0f}; // normalized radius (0 disables)
        glm::vec4 m_globalTint{1.0f};
        ParticleBlendMode m_blendMode{ParticleBlendMode::Alpha};
        unsigned int m_currentTexture{0};
        bool m_useRadialMask{true};
        bool m_frameActive{false};
        bool m_blendWasEnabled{false};
        int m_previousBlendSourceRgb{0};
        int m_previousBlendDestinationRgb{0};
        int m_previousBlendSourceAlpha{0};
        int m_previousBlendDestinationAlpha{0};
        int m_previousBlendEquationRgb{0};
        int m_previousBlendEquationAlpha{0};
    };
}



#endif //GL2D_PARTICLERENDERER_HPP
