#include "Engine/Renderer.h"
#include "Engine/Font.h"
#include "Core/RenderFrame.h"

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>   // glfwGetProcAddress

#include <cstdio>

namespace {

constexpr int kGlyph = 8;     // font cell size in px
constexpr int kTextScale = 2; // HUD text is drawn at 2x -> 16px

unsigned int compile(unsigned int type, const char* src) {
    unsigned int s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::fprintf(stderr, "[shader] %s\n", log); }
    return s;
}

unsigned int link(const char* vs, const char* fs) {
    unsigned int v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// shared vertex shader: pos.xy (pixels), uv.xy, color.rgba
const char* kVert = R"(#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
layout(location=2) in vec4 aColor;
uniform mat4 uProj;
out vec2 vUV; out vec4 vColor;
void main(){ gl_Position = uProj * vec4(aPos,0.0,1.0); vUV=aUV; vColor=aColor; }
)";

const char* kSpriteFrag = R"(#version 330 core
in vec2 vUV; in vec4 vColor; out vec4 F;
uniform sampler2D uTex;
void main(){ F = texture(uTex, vUV) * vColor; }
)";

const char* kTextFrag = R"(#version 330 core
in vec2 vUV; in vec4 vColor; out vec4 F;
uniform sampler2D uFont;
void main(){ float a = texture(uFont, vUV).r; if (a < 0.5) discard; F = vec4(vColor.rgb, 1.0); }
)";

void orthoTopLeft(float W, float H, float out[16]) {
    // maps (0,0) top-left .. (W,H) bottom-right to clip space, column-major
    float m[16] = {
        2.0f / W, 0, 0, 0,
        0, -2.0f / H, 0, 0,
        0, 0, -1, 0,
        -1, 1, 0, 1
    };
    for (int i = 0; i < 16; ++i) out[i] = m[i];
}

} // namespace

bool Renderer::init(const std::string& assetsDir) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "gladLoadGLLoader failed\n");
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    spriteProg_ = link(kVert, kSpriteFrag);
    textProg_   = link(kVert, kTextFrag);
    fontTex_    = CreateFontAtlas();

    if (!sprites_.loadAll(assetsDir))
        std::fprintf(stderr, "[renderer] some sprites failed to load (assetsDir=%s)\n", assetsDir.c_str());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    const int stride = 8 * sizeof(float);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);                  glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float))); glEnableVertexAttribArray(2);
    return true;
}

void Renderer::shutdown() {
    sprites_.shutdown();
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (fontTex_) glDeleteTextures(1, &fontTex_);
    if (spriteProg_) glDeleteProgram(spriteProg_);
    if (textProg_) glDeleteProgram(textProg_);
}

void Renderer::draw(const RenderFrame& frame, int viewportW, int viewportH) {
    glViewport(0, 0, viewportW, viewportH);
    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawSprites(frame, viewportW, viewportH);
    drawText(frame, viewportW, viewportH);
}

void Renderer::drawSprites(const RenderFrame& frame, int W, int H) {
    if (frame.sprites.empty()) return;

    glUseProgram(spriteProg_);
    float proj[16]; orthoTopLeft((float)W, (float)H, proj);
    glUniformMatrix4fv(glGetUniformLocation(spriteProg_, "uProj"), 1, GL_FALSE, proj);
    glUniform1i(glGetUniformLocation(spriteProg_, "uTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    const CameraState& cam = frame.camera;
    const float z = cam.zoom, cx = W * 0.5f, cy = H * 0.5f;

    // Draw in submission order, flushing whenever the texture changes. This
    // preserves layering (floor -> walls -> entities) while batching runs.
    auto flush = [&](SpriteId id) {
        if (scratch_.empty()) return;
        unsigned int tex = sprites_.texture(id);
        if (tex) {
            glBindTexture(GL_TEXTURE_2D, tex);
            glBufferData(GL_ARRAY_BUFFER, scratch_.size() * sizeof(float), scratch_.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, GLsizei(scratch_.size() / 8));
        }
        scratch_.clear();
    };

    SpriteId run = SpriteId::None;
    bool first = true;
    for (const SpriteDraw& s : frame.sprites) {
        if (s.id == SpriteId::None) continue;
        if (!first && s.id != run) flush(run);
        run = s.id; first = false;

        float x0 = (s.x - cam.x) * z + cx;
        float y0 = (s.y - cam.y) * z + cy;
        float x1 = x0 + s.w * z, y1 = y0 + s.h * z;
        float r = s.r, g = s.g, b = s.b, a = s.a;

        auto push = [&](float px, float py, float u, float v) {
            scratch_.insert(scratch_.end(), { px, py, u, v, r, g, b, a });
        };
        push(x0, y0, 0, 0); push(x1, y0, 1, 0); push(x1, y1, 1, 1);
        push(x0, y0, 0, 0); push(x1, y1, 1, 1); push(x0, y1, 0, 1);
    }
    flush(run);
}

void Renderer::drawText(const RenderFrame& frame, int W, int H) {
    if (frame.texts.empty()) return;

    glUseProgram(textProg_);
    float proj[16]; orthoTopLeft((float)W, (float)H, proj);
    glUniformMatrix4fv(glGetUniformLocation(textProg_, "uProj"), 1, GL_FALSE, proj);
    glUniform1i(glGetUniformLocation(textProg_, "uFont"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    const float uStep = 1.0f / 16.0f, vStep = 1.0f / 8.0f;
    const float cell = float(kGlyph * kTextScale);

    scratch_.clear();
    for (const TextDraw& t : frame.texts) {
        for (size_t i = 0; i < t.text.size(); ++i) {
            int c = (unsigned char)t.text[i] & 0x7f;
            if (c == ' ') continue;
            float u0 = (c % 16) * uStep, v0 = (c / 16) * vStep;
            float u1 = u0 + uStep, v1 = v0 + vStep;
            float x0 = t.x + (float)i * cell, y0 = (float)t.y;
            float x1 = x0 + cell, y1 = y0 + cell;
            float r = t.r, g = t.g, b = t.b;
            auto push = [&](float px, float py, float u, float v) {
                scratch_.insert(scratch_.end(), { px, py, u, v, r, g, b, 1.0f });
            };
            push(x0, y0, u0, v0); push(x1, y0, u1, v0); push(x1, y1, u1, v1);
            push(x0, y0, u0, v0); push(x1, y1, u1, v1); push(x0, y1, u0, v1);
        }
    }
    if (!scratch_.empty()) {
        glBufferData(GL_ARRAY_BUFFER, scratch_.size() * sizeof(float), scratch_.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, GLsizei(scratch_.size() / 8));
    }
}
