#include "RenderingSystem/PostProcessPipeline.hpp"

#include "Graphics/Shader.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace Rendering {
namespace {
void validate(const PostProcessSettings& settings) {
    const std::array values{
        settings.exposure, settings.gamma, settings.bloomThreshold,
        settings.bloomSoftKnee, settings.bloomStrength, settings.saturation,
        settings.contrast, settings.colorTint.x, settings.colorTint.y,
        settings.colorTint.z, settings.vignetteStrength,
        settings.vignetteSoftness, settings.letterboxAmount
    };
    if (!std::ranges::all_of(values, [](float value) { return std::isfinite(value); }) ||
        settings.exposure < 0.0f || settings.gamma <= 0.0f ||
        settings.bloomThreshold < 0.0f || settings.bloomSoftKnee < 0.0f ||
        settings.bloomSoftKnee > 1.0f || settings.bloomStrength < 0.0f ||
        settings.bloomIterations < 0 || settings.bloomIterations > 16 ||
        settings.saturation < 0.0f || settings.contrast < 0.0f ||
        settings.colorTint.x < 0.0f || settings.colorTint.y < 0.0f ||
        settings.colorTint.z < 0.0f ||
        settings.vignetteStrength < 0.0f || settings.vignetteStrength > 1.0f ||
        settings.vignetteSoftness <= 0.0f || settings.vignetteSoftness > 1.0f ||
        settings.letterboxAmount < 0.0f || settings.letterboxAmount > 0.45f) {
        throw std::invalid_argument("PostProcessSettings contains invalid values");
    }
}
}

PostProcessPipeline::~PostProcessPipeline() {
    if (m_vertexArray) {
        glDeleteVertexArrays(1, &m_vertexArray);
    }
}

void PostProcessPipeline::ensureResources() {
    if (!m_extractShader) {
        m_extractShader = std::make_shared<Graphics::Shader>(
            "Shaders/fullscreen.vert", "Shaders/bloom_extract.frag");
        m_blurShader = std::make_shared<Graphics::Shader>(
            "Shaders/fullscreen.vert", "Shaders/bloom_blur.frag");
        m_compositeShader = std::make_shared<Graphics::Shader>(
            "Shaders/fullscreen.vert", "Shaders/post_composite.frag");
    }
    if (!m_vertexArray) {
        glGenVertexArrays(1, &m_vertexArray);
    }
}

void PostProcessPipeline::drawFullscreen() const {
    glBindVertexArray(m_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void PostProcessPipeline::draw(GLuint hdrSceneTexture, int width, int height,
                               const PostProcessSettings& settings) {
    if (!hdrSceneTexture || width <= 0 || height <= 0) {
        throw std::invalid_argument("PostProcessPipeline requires a valid HDR texture and dimensions");
    }
    validate(settings);
    ensureResources();

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    if (blendWasEnabled) {
        glDisable(GL_BLEND);
    }

    GLuint bloomTexture = hdrSceneTexture;
    const bool renderBloom = settings.enabled && settings.bloomEnabled &&
                             settings.bloomStrength > 0.0f;
    if (renderBloom) {
        const int bloomWidth = std::max(1, width / 2);
        const int bloomHeight = std::max(1, height / 2);
        m_bloomA.resize(bloomWidth, bloomHeight);
        m_bloomB.resize(bloomWidth, bloomHeight);

        m_bloomA.bind();
        glViewport(0, 0, bloomWidth, bloomHeight);
        m_extractShader->enable();
        m_extractShader->setUniformInt1("uScene", 0);
        m_extractShader->setUniformFloat1("uThreshold", settings.bloomThreshold);
        m_extractShader->setUniformFloat1("uSoftKnee", settings.bloomSoftKnee);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrSceneTexture);
        drawFullscreen();

        for (int iteration = 0; iteration < settings.bloomIterations; ++iteration) {
            m_bloomB.bind();
            m_blurShader->enable();
            m_blurShader->setUniformInt1("uImage", 0);
            m_blurShader->setUniformFloat2("uDirection", {1.0f, 0.0f});
            m_blurShader->setUniformFloat2(
                "uTexelSize", {1.0f / bloomWidth, 1.0f / bloomHeight});
            glBindTexture(GL_TEXTURE_2D, m_bloomA.texture());
            drawFullscreen();

            m_bloomA.bind();
            m_blurShader->setUniformFloat2("uDirection", {0.0f, 1.0f});
            glBindTexture(GL_TEXTURE_2D, m_bloomB.texture());
            drawFullscreen();
        }
        bloomTexture = m_bloomA.texture();
    }

    ColorRenderTarget::unbind();
    glViewport(0, 0, width, height);
    m_compositeShader->enable();
    m_compositeShader->setUniformInt1("uScene", 0);
    m_compositeShader->setUniformInt1("uBloom", 1);
    m_compositeShader->setUniformInt1("uEffectsEnabled", settings.enabled ? 1 : 0);
    m_compositeShader->setUniformFloat1("uExposure", settings.exposure);
    m_compositeShader->setUniformFloat1("uGamma", settings.gamma);
    m_compositeShader->setUniformFloat1(
        "uBloomStrength", renderBloom ? settings.bloomStrength : 0.0f);
    m_compositeShader->setUniformFloat1("uSaturation", settings.saturation);
    m_compositeShader->setUniformFloat1("uContrast", settings.contrast);
    m_compositeShader->setUniformFloat3("uColorTint", settings.colorTint);
    m_compositeShader->setUniformFloat1("uVignetteStrength", settings.vignetteStrength);
    m_compositeShader->setUniformFloat1("uVignetteSoftness", settings.vignetteSoftness);
    m_compositeShader->setUniformFloat1("uLetterbox", settings.letterboxAmount);
    m_compositeShader->setUniformFloat2(
        "uResolution", {static_cast<float>(width), static_cast<float>(height)});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrSceneTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, bloomTexture);
    drawFullscreen();

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (blendWasEnabled) {
        glEnable(GL_BLEND);
    }
}

} // namespace Rendering
