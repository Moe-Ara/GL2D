//
// LightingPass.hpp
//

#ifndef GL2D_LIGHTINGPASS_HPP
#define GL2D_LIGHTINGPASS_HPP

#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>
#include <string>

#include "Graphics/Shader.hpp"
#include "Graphics/LightingSystem/Light.hpp"
#include "RenderTarget.hpp"

namespace Rendering {

class LightingPass {
public:
    static void draw(const RenderTarget& target,
                     const std::vector<Light>& lights,
                     const glm::vec4& viewBounds,
                     const std::vector<GLuint>& cookieTextures,
                     const glm::vec3& ambientColor = glm::vec3(0.05f));

private:
    static void ensureResources();
    static constexpr int kMaxLights = 8;
    static GLuint s_vao;
    static GLuint s_vbo;
    static std::shared_ptr<Graphics::Shader> s_shader;
};

} // namespace Rendering

#endif // GL2D_LIGHTINGPASS_HPP
