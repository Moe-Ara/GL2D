//
// LightingPass.hpp
//

#ifndef GL2D_LIGHTINGPASS_HPP
#define GL2D_LIGHTINGPASS_HPP

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <vector>

#include "Graphics/Shader.hpp"
#include "Graphics/LightingSystem/Light.hpp"
#include "RenderTarget.hpp"

namespace Rendering {

class LightingPass {
public:
    LightingPass() = default;
    ~LightingPass();

    LightingPass(const LightingPass&) = delete;
    LightingPass& operator=(const LightingPass&) = delete;
    LightingPass(LightingPass&&) = delete;
    LightingPass& operator=(LightingPass&&) = delete;

    void draw(const RenderTarget& target,
              const std::vector<Light>& lights,
              const glm::mat4& inverseViewProjection,
              const std::vector<GLuint>& cookieTextures,
              const glm::vec3& ambientColor = glm::vec3(0.05f));

    // Compatibility overload for callers with an unrotated axis-aligned view.
    void draw(const RenderTarget& target,
              const std::vector<Light>& lights,
              const glm::vec4& viewBounds,
              const std::vector<GLuint>& cookieTextures,
              const glm::vec3& ambientColor = glm::vec3(0.05f));

private:
    void ensureResources();
    static constexpr int kMaxLights = 8;
    GLuint m_vertexArray{0};
    std::shared_ptr<Graphics::Shader> m_shader;
};

} // namespace Rendering

#endif // GL2D_LIGHTINGPASS_HPP
