//
// LightingPass.cpp
//

#include "LightingPass.hpp"

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <array>

namespace Rendering {

GLuint LightingPass::s_vao = 0;
GLuint LightingPass::s_vbo = 0;
std::shared_ptr<Graphics::Shader> LightingPass::s_shader = nullptr;

void LightingPass::ensureResources() {
    if (!s_shader) {
        s_shader = std::make_shared<Graphics::Shader>("Shaders/lighting.vert", "Shaders/lighting.frag");
    }
    if (s_vao == 0) {
        glGenVertexArrays(1, &s_vao);
    }
}

void LightingPass::draw(const RenderTarget &target, const std::vector<Light> &lights,
                        const glm::vec4 &viewBounds, const std::vector<GLuint>& cookieTextures,
                        const glm::vec3 &ambientColor) {
    if (!target.isInitialized() || target.colorTexture() == 0) {
        return;
    }

    ensureResources();

    s_shader->enable();
    s_shader->setUniformInt1("uSceneTex", 0);
    s_shader->setUniformInt1("uNormalTex", 1);
    s_shader->setUniformFloat3("uAmbient", ambientColor);
    s_shader->setUniformFloat4("uViewBounds", viewBounds);

    const int count = static_cast<int>(std::min<size_t>(lights.size(), kMaxLights));
    s_shader->setUniformInt1("uLightCount", count);

    for (int i = 0; i < count; ++i) {
        const auto &l = lights[static_cast<size_t>(i)];
        const std::string base = "uLights[" + std::to_string(i) + "].";
        s_shader->setUniformInt1(base + "type", static_cast<int>(l.type));
        s_shader->setUniformFloat2(base + "pos", l.pos);
        s_shader->setUniformFloat2(base + "dir", l.dir);
        s_shader->setUniformFloat1(base + "radius", l.radius);
        s_shader->setUniformFloat3(base + "color", l.color);
        s_shader->setUniformFloat1(base + "intensity", l.intensity);
        s_shader->setUniformFloat1(base + "falloff", l.falloff);
        s_shader->setUniformFloat1(base + "innerCutoff", l.innerCutoff);
        s_shader->setUniformFloat1(base + "outerCutoff", l.outerCutoff);
        s_shader->setUniformFloat1(base + "emissiveBoost", l.emissiveBoost);
        s_shader->setUniformInt1(base + "cookieSlot", l.cookieSlot);
        s_shader->setUniformFloat1(base + "cookieStrength", l.cookieStrength);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, target.colorTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, target.normalTexture());
    if (!cookieTextures.empty()) {
        const int count = static_cast<int>(std::min(cookieTextures.size(), static_cast<size_t>(kMaxLights)));
        for (int i = 0; i < count; ++i) {
            s_shader->setUniformInt1("uCookieTex[" + std::to_string(i) + "]", 2 + i);
            glActiveTexture(GL_TEXTURE2 + static_cast<GLenum>(i));
            glBindTexture(GL_TEXTURE_2D, cookieTextures[static_cast<size_t>(i)]);
        }
    }

    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (blendEnabled) {
        glDisable(GL_BLEND);
    }

    glBindVertexArray(s_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
}

} // namespace Rendering
