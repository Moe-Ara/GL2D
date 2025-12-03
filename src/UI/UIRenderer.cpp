#include "UIRenderer.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_EASY_FONT_IMPLEMENTATION
#include "third_party/stb/stb_easy_font.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb/stb_truetype.h"

namespace UI {

namespace {
struct FontAtlas {
    GLuint texture{0};
    int atlasW{0};
    int atlasH{0};
    float pixelHeight{32.0f};
    float lineHeight{32.0f};
    stbtt_bakedchar chars[96]{};
    bool valid{false};
    std::string name;
};

FontAtlas g_font{};
std::string g_fontPath = "assets/fonts/Roboto-Regular.ttf";
float g_fontPixelHeight = 32.0f;
bool g_fontLoaded = false;

bool loadFontAtlas(const std::string& path, float pixelHeight) {
    // Read font file into memory.
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});
    if (buffer.empty()) {
        return false;
    }

    // Init font info for metrics.
    stbtt_fontinfo info{};
    if (!stbtt_InitFont(&info, buffer.data(), 0)) {
        return false;
    }

    const float scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    const float lineHeight = (ascent - descent + lineGap) * scale;

    // Bake bitmap atlas.
    int atlasW = 512;
    int atlasH = 512;
    std::vector<unsigned char> bitmap(atlasW * atlasH);
    int res = stbtt_BakeFontBitmap(buffer.data(), 0, pixelHeight,
                                   bitmap.data(), atlasW, atlasH,
                                   32, 96, g_font.chars);
    if (res <= 0) {
        return false;
    }

    // Upload to GL as alpha texture.
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlasW, atlasH, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    g_font.texture = tex;
    g_font.atlasW = atlasW;
    g_font.atlasH = atlasH;
    g_font.pixelHeight = pixelHeight;
    g_font.lineHeight = lineHeight;
    g_font.valid = true;
    g_font.name = path;
    return true;
}

float computeTextHeight(const std::string& text) {
    if (!g_font.valid) return 0.0f;
    int lines = 1;
    for (char c : text) {
        if (c == '\n') ++lines;
    }
    return g_font.lineHeight * static_cast<float>(lines);
}
} // namespace

bool UIRenderer::setFont(const std::string& ttfPath, float pixelHeight) {
    // Reset existing atlas if any.
    if (g_font.texture != 0) {
        glDeleteTextures(1, &g_font.texture);
        g_font = FontAtlas{};
    }
    g_fontPath = ttfPath;
    g_fontPixelHeight = pixelHeight;
    g_fontLoaded = loadFontAtlas(ttfPath, pixelHeight);
    return g_fontLoaded;
}

void UIRenderer::render(const std::vector<UIRenderCommand>& commands, int fbWidth, int fbHeight) {
    if (commands.empty() || fbWidth <= 0 || fbHeight <= 0) return;

    // Copy and stable sort by zIndex then original order to preserve submission order for ties.
    std::vector<UIRenderCommand> sorted = commands;
    std::stable_sort(sorted.begin(), sorted.end(), [](const UIRenderCommand& a, const UIRenderCommand& b) {
        return a.zIndex < b.zIndex;
    });

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(fbWidth), 0.0, static_cast<double>(fbHeight), -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (const auto& cmd : sorted) {
        const float x0 = cmd.rect.x;
        const float y0 = cmd.rect.y;
        const float x1 = cmd.rect.z;
        const float y1 = cmd.rect.w;
        const bool hasText = !cmd.text.empty();
        if (!hasText && (x1 <= x0 || y1 <= y0)) continue;
        if (!hasText && cmd.texture && cmd.texture->getID() != 0) {
            cmd.texture->bind(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
        } else {
            glDisable(GL_TEXTURE_2D);
        }

        if (hasText) {
            glDisable(GL_LIGHTING);
            glColor4f(cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);

            if (!g_fontLoaded) {
                g_fontLoaded = loadFontAtlas(g_fontPath, g_fontPixelHeight);
            }

            if (g_font.valid) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, g_font.texture);

                const float scaledHeight = computeTextHeight(cmd.text) * cmd.textScale;
                glPushMatrix();
                glTranslatef(x0, y0 + scaledHeight, 0.0f);
                glScalef(cmd.textScale, -cmd.textScale, 1.0f);

                float penX = 0.0f;
                float penY = 0.0f;
                glBegin(GL_QUADS);
                for (char c : cmd.text) {
                    if (c == '\n') {
                        penX = 0.0f;
                        penY += g_font.lineHeight;
                        continue;
                    }
                    if (c < 32 || c >= 128) continue;

                    stbtt_aligned_quad q{};
                    stbtt_GetBakedQuad(g_font.chars, g_font.atlasW, g_font.atlasH, c - 32, &penX, &penY, &q, 1);
                    glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
                    glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
                    glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
                    glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
                }
                glEnd();
                glPopMatrix();

                glBindTexture(GL_TEXTURE_2D, 0);
            } else {
                glDisable(GL_TEXTURE_2D);

                // stb_easy_font outputs 4 floats per vertex (x,y,z,color). Use a large static buffer.
                static std::array<float, 16 * 4096> verts{}; // up to 4096 quads
                const int quads = stb_easy_font_print(0.0f, 0.0f, cmd.text.c_str(), nullptr,
                                                      verts.data(), static_cast<int>(verts.size() * sizeof(float)));
                const int maxQuads = static_cast<int>(verts.size() / 16);
                const int drawQuads = std::min(quads, maxQuads);
                const int vertCount = drawQuads * 4;

                // stb_easy_font assumes +Y goes downward. Flip Y for our bottom-left origin and offset by text height to preserve top alignment.
                const float textPxHeight = static_cast<float>(stb_easy_font_height(cmd.text.c_str()));
                const float scaledHeight = textPxHeight * cmd.textScale;

                glPushMatrix();
                glTranslatef(x0, y0 + scaledHeight, 0.0f);
                glScalef(cmd.textScale, -cmd.textScale, 1.0f);

                glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
                // Ensure stale client arrays (e.g., color/texcoord) don't override our color or stride.
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                glDisableClientState(GL_COLOR_ARRAY);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(2, GL_FLOAT, 16, verts.data()); // stride 4 floats (16 bytes)
                glDrawArrays(GL_QUADS, 0, vertCount);
                glPopClientAttrib();

                glPopMatrix();
            }
        } else {
            glColor4f(cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y0);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y0);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y1);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y1);
            glEnd();
        }

        if (!hasText && cmd.texture) {
            cmd.texture->unbind();
        }
    }

    glPopMatrix(); // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

} // namespace UI
