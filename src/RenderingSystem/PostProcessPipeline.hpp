#pragma once

#include "RenderingSystem/ColorRenderTarget.hpp"
#include "RenderingSystem/PostProcessSettings.hpp"

#include <GL/glew.h>

#include <memory>

namespace Graphics {
class Shader;
}

namespace Rendering {

class PostProcessPipeline {
public:
    PostProcessPipeline() = default;
    ~PostProcessPipeline();

    PostProcessPipeline(const PostProcessPipeline&) = delete;
    PostProcessPipeline& operator=(const PostProcessPipeline&) = delete;
    PostProcessPipeline(PostProcessPipeline&&) = delete;
    PostProcessPipeline& operator=(PostProcessPipeline&&) = delete;

    void draw(GLuint hdrSceneTexture, int width, int height,
              const PostProcessSettings& settings);

private:
    void ensureResources();
    void drawFullscreen() const;

    ColorRenderTarget m_bloomA;
    ColorRenderTarget m_bloomB;
    std::shared_ptr<Graphics::Shader> m_extractShader;
    std::shared_ptr<Graphics::Shader> m_blurShader;
    std::shared_ptr<Graphics::Shader> m_compositeShader;
    GLuint m_vertexArray{0};
};

} // namespace Rendering
