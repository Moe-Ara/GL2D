#include "UIRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "GameObjects/Texture.hpp"
#include "Graphics/Shader.hpp"

#define STB_EASY_FONT_IMPLEMENTATION
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "third_party/stb/stb_easy_font.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb/stb_truetype.h"

namespace UI {
namespace {
constexpr std::size_t kMaxTextBytes = 64U << 10U;

bool finite(const glm::vec4& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z) && std::isfinite(value.w);
}
}

struct UIRenderer::Impl {
    struct Vertex {
        glm::vec2 position{0.0f};
        glm::vec2 uv{0.0f};
        glm::vec4 color{1.0f};
    };

    struct Batch {
        GLuint texture{0};
        std::size_t firstIndex{0};
        std::size_t indexCount{0};
        bool alphaMask{false};
    };

    struct FontAtlas {
        GLuint texture{0};
        int width{0};
        int height{0};
        float pixelHeight{32.0f};
        float lineHeight{32.0f};
        std::array<stbtt_bakedchar, 96> characters{};

        [[nodiscard]] bool valid() const noexcept { return texture != 0; }
    };

    explicit Impl(const std::string& vertexShader, const std::string& fragmentShader)
        : shader(vertexShader, fragmentShader) {
        try {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
            if (!vao || !vbo || !ebo) {
                throw std::runtime_error("UIRenderer failed to allocate GPU buffers");
            }

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, position)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, uv)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                  reinterpret_cast<void*>(offsetof(Vertex, color)));
            glBindVertexArray(0);

            glGenTextures(1, &whiteTexture);
            if (!whiteTexture) {
                throw std::runtime_error("UIRenderer failed to allocate its white texture");
            }
            GLint previousActiveTexture = 0;
            glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
            glActiveTexture(GL_TEXTURE0);
            GLint previousTexture = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
            glBindTexture(GL_TEXTURE_2D, whiteTexture);
            constexpr std::array<std::uint8_t, 4> white{255, 255, 255, 255};
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, white.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
            glActiveTexture(static_cast<GLenum>(previousActiveTexture));
        } catch (...) {
            releaseGpuResources();
            throw;
        }
    }

    ~Impl() {
        releaseGpuResources();
    }

    void releaseGpuResources() noexcept {
        if (font.texture) glDeleteTextures(1, &font.texture);
        if (whiteTexture) glDeleteTextures(1, &whiteTexture);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        font.texture = 0;
        whiteTexture = 0;
        ebo = 0;
        vbo = 0;
        vao = 0;
    }

    void appendQuad(GLuint texture,
                    bool alphaMask,
                    const std::array<glm::vec2, 4>& positions,
                    const std::array<glm::vec2, 4>& uvs,
                    const glm::vec4& color) {
        if (vertices.size() > std::numeric_limits<std::uint32_t>::max() - 4U) {
            throw std::length_error("UI vertex capacity exceeded");
        }
        const std::size_t firstIndex = indices.size();
        const auto base = static_cast<std::uint32_t>(vertices.size());
        for (std::size_t i = 0; i < positions.size(); ++i) {
            vertices.push_back(Vertex{positions[i], uvs[i], color});
        }
        indices.insert(indices.end(), {base, base + 1U, base + 2U,
                                       base + 2U, base + 3U, base});
        if (batches.empty() || batches.back().texture != texture ||
            batches.back().alphaMask != alphaMask) {
            batches.push_back(Batch{texture, firstIndex, 6U, alphaMask});
        } else {
            batches.back().indexCount += 6U;
        }
    }

    void appendPanel(const UIRenderCommand& command) {
        const float x0 = command.rect.x;
        const float y0 = command.rect.y;
        const float x1 = command.rect.z;
        const float y1 = command.rect.w;
        if (x1 <= x0 || y1 <= y0) return;
        const GLuint texture = command.texture && command.texture->getID()
                                   ? command.texture->getID() : whiteTexture;
        appendQuad(texture, false,
                   {{{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}}},
                   {{{0.0f, 0.0f}, {1.0f, 0.0f},
                     {1.0f, 1.0f}, {0.0f, 1.0f}}}, command.color);
    }

    void appendBakedText(const UIRenderCommand& command) {
        int lineCount = 1;
        for (const char c : command.text) {
            if (c == '\n') ++lineCount;
        }
        const float totalHeight = font.lineHeight * command.textScale *
                                  static_cast<float>(lineCount);
        float penX = 0.0f;
        float penY = 0.0f;
        for (const char c : command.text) {
            if (c == '\n') {
                penX = 0.0f;
                penY += font.lineHeight;
                continue;
            }
            auto code = static_cast<unsigned char>(c);
            if (code < 32U || code >= 128U) code = static_cast<unsigned char>('?');
            stbtt_aligned_quad quad{};
            stbtt_GetBakedQuad(font.characters.data(), font.width, font.height,
                               static_cast<int>(code - 32U), &penX, &penY, &quad, 1);
            const auto transform = [&](float x, float y) {
                return glm::vec2{command.rect.x + x * command.textScale,
                                 command.rect.y + totalHeight - y * command.textScale};
            };
            appendQuad(font.texture, true,
                       {{transform(quad.x0, quad.y0), transform(quad.x1, quad.y0),
                         transform(quad.x1, quad.y1), transform(quad.x0, quad.y1)}},
                       {{{quad.s0, quad.t0}, {quad.s1, quad.t0},
                         {quad.s1, quad.t1}, {quad.s0, quad.t1}}}, command.color);
        }
    }

    void appendFallbackText(const UIRenderCommand& command) {
        if (command.text.empty()) return;
        std::string ascii;
        ascii.reserve(command.text.size());
        std::size_t quadCapacity = 0;
        for (const char input : command.text) {
            auto code = static_cast<unsigned char>(input);
            if (input == '\n') {
                ascii.push_back('\n');
                continue;
            }
            if (code < 32U || code >= 128U) code = static_cast<unsigned char>('?');
            ascii.push_back(static_cast<char>(code));
            const int index = static_cast<int>(code - 32U);
            quadCapacity += static_cast<std::size_t>(
                stb_easy_font_charinfo[index + 1].h_seg -
                stb_easy_font_charinfo[index].h_seg +
                stb_easy_font_charinfo[index + 1].v_seg -
                stb_easy_font_charinfo[index].v_seg);
        }
        if (quadCapacity == 0) return;
        const std::size_t floatCount = quadCapacity * 16U;
        std::vector<float> raw(floatCount);
        const int quads = stb_easy_font_print(0.0f, 0.0f, ascii.c_str(),
                                              nullptr, raw.data(),
                                              static_cast<int>(raw.size() * sizeof(float)));
        const float height = static_cast<float>(stb_easy_font_height(ascii.c_str())) *
                             command.textScale;
        for (int quadIndex = 0; quadIndex < quads; ++quadIndex) {
            std::array<glm::vec2, 4> positions{};
            for (int vertex = 0; vertex < 4; ++vertex) {
                const std::size_t offset = static_cast<std::size_t>(quadIndex * 16 + vertex * 4);
                positions[static_cast<std::size_t>(vertex)] = {
                    command.rect.x + raw[offset] * command.textScale,
                    command.rect.y + height - raw[offset + 1U] * command.textScale};
            }
            appendQuad(whiteTexture, false, positions,
                       {{{0.0f, 0.0f}, {1.0f, 0.0f},
                         {1.0f, 1.0f}, {0.0f, 1.0f}}}, command.color);
        }
    }

    Graphics::Shader shader;
    GLuint vao{0};
    GLuint vbo{0};
    GLuint ebo{0};
    GLuint whiteTexture{0};
    FontAtlas font{};
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Batch> batches;
};

UIRenderer::UIRenderer(const std::string& vertexShader,
                       const std::string& fragmentShader)
    : m_impl(std::make_unique<Impl>(vertexShader, fragmentShader)) {
    setFont("assets/fonts/Roboto-Regular.ttf", 32.0f);
}

UIRenderer::~UIRenderer() = default;

bool UIRenderer::setFont(const std::string& ttfPath, float pixelHeight) {
    if (ttfPath.empty() || !std::isfinite(pixelHeight) ||
        pixelHeight <= 0.0f || pixelHeight > 512.0f) {
        throw std::invalid_argument("UI font path must be non-empty and height must be in (0, 512]");
    }
    std::ifstream file(ttfPath, std::ios::binary);
    if (!file) return false;
    std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(file), {});
    if (bytes.empty()) return false;

    const int fontOffset = stbtt_GetFontOffsetForIndex(bytes.data(), 0);
    if (fontOffset < 0) return false;
    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, bytes.data(), fontOffset)) return false;
    const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    Impl::FontAtlas candidate{};
    candidate.pixelHeight = pixelHeight;
    candidate.lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;
    std::vector<unsigned char> bitmap;
    for (int side = 256; side <= 2048; side *= 2) {
        bitmap.assign(static_cast<std::size_t>(side) * static_cast<std::size_t>(side), 0U);
        candidate.characters.fill({});
        if (stbtt_BakeFontBitmap(bytes.data(), fontOffset, pixelHeight, bitmap.data(), side, side,
                                32, 96, candidate.characters.data()) > 0) {
            candidate.width = side;
            candidate.height = side;
            break;
        }
    }
    if (candidate.width == 0) return false;

    glGenTextures(1, &candidate.texture);
    if (!candidate.texture) return false;
    GLint previousActiveTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    GLint previousTexture = 0;
    GLint previousUnpackAlignment = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glBindTexture(GL_TEXTURE_2D, candidate.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, candidate.width, candidate.height,
                 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));

    if (m_impl->font.texture) glDeleteTextures(1, &m_impl->font.texture);
    m_impl->font = candidate;
    return true;
}

bool UIRenderer::hasFont() const noexcept {
    return m_impl->font.valid();
}

void UIRenderer::render(const std::vector<UIRenderCommand>& commands,
                        int framebufferWidth,
                        int framebufferHeight) {
    if (commands.empty() || framebufferWidth <= 0 || framebufferHeight <= 0) return;

    std::vector<const UIRenderCommand*> sorted;
    sorted.reserve(commands.size());
    for (const UIRenderCommand& command : commands) sorted.push_back(&command);
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const UIRenderCommand* left, const UIRenderCommand* right) {
            return left->zIndex < right->zIndex;
        });

    m_impl->vertices.clear();
    m_impl->indices.clear();
    m_impl->batches.clear();
    for (std::size_t i = 0; i < sorted.size(); ++i) {
        const UIRenderCommand& command = *sorted[i];
        if (!finite(command.rect) || !finite(command.color) ||
            !std::isfinite(command.zIndex)) {
            throw std::invalid_argument("UI render command " + std::to_string(i) +
                                        " contains non-finite geometry, color, or depth");
        }
        if (command.text.empty()) {
            m_impl->appendPanel(command);
            continue;
        }
        if (!std::isfinite(command.textScale) || command.textScale <= 0.0f) {
            throw std::invalid_argument("UI text scale must be finite and positive");
        }
        if (command.text.size() > kMaxTextBytes) {
            throw std::length_error("UI text command exceeds the 64 KiB safety limit");
        }
        if (m_impl->font.valid()) m_impl->appendBakedText(command);
        else m_impl->appendFallbackText(command);
    }
    if (m_impl->indices.empty()) return;

    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_impl->shader.enable();
    m_impl->shader.setUniformFloat2("viewportSize",
                                    {static_cast<float>(framebufferWidth),
                                     static_cast<float>(framebufferHeight)});
    m_impl->shader.setUniformInt1("uiTexture", 0);
    glBindVertexArray(m_impl->vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_impl->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_impl->vertices.size() * sizeof(Impl::Vertex)),
                 m_impl->vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_impl->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_impl->indices.size() * sizeof(std::uint32_t)),
                 m_impl->indices.data(), GL_DYNAMIC_DRAW);
    glActiveTexture(GL_TEXTURE0);
    for (const Impl::Batch& batch : m_impl->batches) {
        m_impl->shader.setUniformInt1("alphaMask", batch.alphaMask ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D, batch.texture);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.indexCount),
                       GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(batch.firstIndex * sizeof(std::uint32_t)));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace UI
