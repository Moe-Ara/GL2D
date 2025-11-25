//
// Created by Mohamad on 25/11/2025.
//

#ifndef GL2D_PARTICLERENDERER_HPP
#define GL2D_PARTICLERENDERER_HPP
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vector>

namespace Graphics { class Shader; }

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

        virtual ~ParticleRenderer();

        ParticleRenderer(const ParticleRenderer &other) = delete;

        ParticleRenderer &operator=(const ParticleRenderer &other) = delete;

        ParticleRenderer(ParticleRenderer &&other) = delete;

        ParticleRenderer &operator=(ParticleRenderer &&other) = delete;

        void begin(const glm::mat4& viewProjection = glm::mat4(1.0f));
        void submit(const ParticleRenderData& p);
        void end();
        void flush();
        void setBorder(const glm::vec4& color, float thickness);

    private:
        std::vector<ParticleRenderData> m_batch{};
        unsigned int m_vao=0;
        unsigned int m_vbo=0;
        unsigned int m_ebo=0;

        unsigned int m_defaultTexture=0;
        std::shared_ptr<Graphics::Shader> m_shader;
        glm::mat4 m_viewProj{1.0f};
        glm::vec4 m_borderColor{0.0f};
        float m_borderThickness{0.0f}; // normalized radius (0 disables)
    };
}



#endif //GL2D_PARTICLERENDERER_HPP
