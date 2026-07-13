//
// LightingPass.cpp
//

#include "LightingPass.hpp"

#include <GL/glew.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/vec2.hpp>
#include <array>
#include <stdexcept>

namespace Rendering {

LightingPass::~LightingPass() {
    if (m_vertexArray) glDeleteVertexArrays(1, &m_vertexArray);
}

void LightingPass::ensureResources() {
    if (!m_shader) {
        m_shader = std::make_shared<Graphics::Shader>(
            "Shaders/lighting.vert", "Shaders/lighting.frag");
    }
    if (m_vertexArray == 0) {
        glGenVertexArrays(1, &m_vertexArray);
        if (!m_vertexArray) {
            throw std::runtime_error(
                "OpenGL failed to allocate the lighting pass vertex array");
        }
    }
}

void LightingPass::draw(const RenderTarget &target, const std::vector<Light> &lights,
                        const glm::vec4 &viewBounds, const std::vector<GLuint>& cookieTextures,
                        const glm::vec3 &ambientColor) {
    const glm::mat4 viewProjection = glm::ortho(viewBounds.x, viewBounds.z,
                                                viewBounds.y, viewBounds.w,
                                                -1.0f, 1.0f);
    draw(target, lights, glm::inverse(viewProjection), cookieTextures, ambientColor);
}

void LightingPass::draw(const RenderTarget &target, const std::vector<Light> &lights,
                        const glm::mat4 &inverseViewProjection,
                        const std::vector<GLuint>& cookieTextures,
                        const glm::vec3 &ambientColor) {
    if (!target.isInitialized() || target.colorTexture() == 0) {
        return;
    }

    ensureResources();

    m_shader->enable();
    m_shader->setUniformInt1("uSceneTex", 0);
    m_shader->setUniformInt1("uNormalTex", 1);
    m_shader->setUniformFloat3("uAmbient", ambientColor);
    m_shader->setUniformMat4("uInverseViewProjection", inverseViewProjection);

    const int count = static_cast<int>(std::min<size_t>(lights.size(), kMaxLights));
    m_shader->setUniformInt1("uLightCount", count);

    for (int i = 0; i < count; ++i) {
        const auto &l = lights[static_cast<size_t>(i)];
        const std::string base = "uLights[" + std::to_string(i) + "].";
        m_shader->setUniformInt1(base + "type", static_cast<int>(l.type));
        m_shader->setUniformFloat2(base + "pos", l.pos);
        m_shader->setUniformFloat2(base + "dir", l.dir);
        m_shader->setUniformFloat1(base + "radius", l.radius);
        m_shader->setUniformFloat3(base + "color", l.color);
        m_shader->setUniformFloat1(base + "intensity", l.intensity);
        m_shader->setUniformFloat1(base + "falloff", l.falloff);
        m_shader->setUniformFloat1(base + "innerCutoff", l.innerCutoff);
        m_shader->setUniformFloat1(base + "outerCutoff", l.outerCutoff);
        m_shader->setUniformFloat1(base + "emissiveBoost", l.emissiveBoost);
        m_shader->setUniformInt1(base + "cookieSlot", l.cookieSlot);
        m_shader->setUniformFloat1(base + "cookieStrength", l.cookieStrength);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target.colorTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, target.normalTexture());
    if (!cookieTextures.empty()) {
        const int count = static_cast<int>(std::min(cookieTextures.size(), static_cast<size_t>(kMaxLights)));
        for (int i = 0; i < count; ++i) {
            m_shader->setUniformInt1("uCookieTex[" + std::to_string(i) + "]", 2 + i);
            glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i));
            glBindTexture(GL_TEXTURE_2D, cookieTextures[static_cast<size_t>(i)]);
        }
    }

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (blendEnabled) {
        glDisable(GL_BLEND);
    }

    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    for (int i = 0; i < static_cast<int>(std::min(
             cookieTextures.size(), static_cast<std::size_t>(kMaxLights))); ++i) {
        glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
}

} // namespace Rendering
