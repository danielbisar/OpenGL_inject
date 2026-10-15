// Experiment inspired by https://aixxe.net/2016/12/imgui-linux-csgo
#include "used_opengl_functions.h"

#include <GL/glx.h>
#include <array>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

namespace {

constexpr GLfloat insetLeft = 0.7f;
constexpr GLfloat insetBottom = 0.7f;
constexpr GLfloat insetSize = 0.3f;
constexpr std::array<GLenum, 10> disabledDuringInset = {
    GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_BLEND, GL_CULL_FACE, GL_STENCIL_TEST,
    GL_ALPHA_TEST, GL_COLOR_LOGIC_OP, GL_LIGHTING, GL_FOG, GL_RASTERIZER_DISCARD
};

void reportOnce(bool& reported, const char* message) {
    if (!reported) {
        std::fprintf(stderr, "[OpenGL Inject] %s\n", message);
        reported = true;
    }
}

class SavedState {
    const GlFunctions& gl;
    GLint program, matrixMode, activeTexture, texture, sampler, unpackBuffer, readBuffer, drawBuffer;
    GLint polygonMode[2];
    GLfloat projection[16], modelview[16];
    GLboolean colorMask[4];
    std::array<GLboolean, disabledDuringInset.size()> enabled;

public:
    explicit SavedState(const GlFunctions& functions) : gl(functions) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        glGetIntegerv(GL_READ_BUFFER, &readBuffer);
        glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        for (std::size_t i = 0; i < enabled.size(); ++i)
            enabled[i] = glIsEnabled(disabledDuringInset[i]);
    }

    ~SavedState() {
        gl.useProgram(program);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(modelview);
        glMatrixMode(matrixMode);
        glBindTexture(GL_TEXTURE_2D, texture);
        gl.bindSampler(0, sampler);
        glActiveTexture(activeTexture);
        gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        glReadBuffer(readBuffer);
        glDrawBuffer(drawBuffer);
        glPolygonMode(GL_FRONT, polygonMode[0]);
        glPolygonMode(GL_BACK, polygonMode[1]);
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        for (std::size_t i = 0; i < enabled.size(); ++i) {
            if (enabled[i]) glEnable(disabledDuringInset[i]);
            else glDisable(disabledDuringInset[i]);
        }
    }

    SavedState(const SavedState&) = delete;
    SavedState& operator=(const SavedState&) = delete;
};

std::optional<std::string> loadShaderSource() {
    const char* configured = std::getenv("OGL_INJECT_SHADER");
    std::filesystem::path path;
    if (configured) {
        path = configured;
        if (!path.is_absolute()) {
            std::fprintf(stderr, "[OpenGL Inject] OGL_INJECT_SHADER must be an absolute path.\n");
            return std::nullopt;
        }
    } else {
        static int libraryAnchor;
        Dl_info info{};
        if (!dladdr(&libraryAnchor, &info) || !info.dli_fname) {
            std::fprintf(stderr, "[OpenGL Inject] Cannot locate library; set OGL_INJECT_SHADER.\n");
            return std::nullopt;
        }
        path = info.dli_fname;
        if (!path.is_absolute()) {
            std::fprintf(stderr, "[OpenGL Inject] Use an absolute LD_PRELOAD or OGL_INJECT_SHADER path.\n");
            return std::nullopt;
        }
        path = path.parent_path() / "fragment.shader";
    }
    std::ifstream file(path);
    if (!file) {
        std::fprintf(stderr, "[OpenGL Inject] Cannot open shader: %s\n", path.c_str());
        return std::nullopt;
    }
    std::ostringstream source;
    source << file.rdbuf();
    if (file.bad()) {
        std::fprintf(stderr, "[OpenGL Inject] Cannot read shader: %s\n", path.c_str());
        return std::nullopt;
    }
    return source.str();
}

struct InjectedRenderContext {
    GLXContext context = nullptr;
    std::thread::id thread;
    GLuint texture = 0, program = 0;
    GLint samplerLocation = -1, viewportLocation = -1, insetLocation = -1;
    GLint width = 0, height = 0;
    bool failed = false, unsupportedReported = false, framebufferReported = false;

    bool createProgram(const GlFunctions& gl) {
        const auto source = loadShaderSource();
        if (!source) return false;
        GLuint shader = gl.createShader(GL_FRAGMENT_SHADER);
        if (!shader) {
            std::fprintf(stderr, "[OpenGL Inject] Cannot create fragment shader.\n");
            return false;
        }
        const char* text = source->c_str();
        gl.shaderSource(shader, 1, &text, nullptr);
        gl.compileShader(shader);
        GLint success = GL_FALSE;
        gl.getShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[4096]{};
            gl.getShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "[OpenGL Inject] Fragment shader compilation failed:\n%s\n", log);
            gl.deleteShader(shader);
            return false;
        }
        program = gl.createProgram();
        if (program) {
            gl.attachShader(program, shader);
            gl.linkProgram(program);
            gl.getProgramiv(program, GL_LINK_STATUS, &success);
        } else {
            success = GL_FALSE;
        }
        gl.deleteShader(shader);
        if (!success) {
            char log[4096]{};
            if (program) gl.getProgramInfoLog(program, sizeof(log), nullptr, log);
            std::fprintf(stderr, "[OpenGL Inject] Shader program creation/link failed:\n%s\n", log);
            if (program) gl.deleteProgram(program);
            program = 0;
            return false;
        }
        samplerLocation = gl.getUniformLocation(program, "screenTexture");
        viewportLocation = gl.getUniformLocation(program, "captureViewport");
        insetLocation = gl.getUniformLocation(program, "insetBounds");
        std::fprintf(stderr, "[OpenGL Inject] Shader program ready.\n");
        return true;
    }

    void render(const GlFunctions& gl, Display* display, GLXDrawable drawable) {
        if (failed) return;
        GLint readFramebuffer, drawFramebuffer, viewport[4], maxTextureSize;
        GLboolean doubleBuffered, stereo;
        GLint sampleBuffers;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
        glGetBooleanv(GL_DOUBLEBUFFER, &doubleBuffered);
        glGetBooleanv(GL_STEREO, &stereo);
        glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
        if (readFramebuffer || drawFramebuffer || !doubleBuffered || stereo || sampleBuffers) {
            reportOnce(framebufferReported,
                       "Skipping unsupported framebuffer (requires default, double-buffered, mono, non-MSAA).");
            return;
        }
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        unsigned drawableWidth = 0, drawableHeight = 0;
        glXQueryDrawable(display, drawable, GLX_WIDTH, &drawableWidth);
        glXQueryDrawable(display, drawable, GLX_HEIGHT, &drawableHeight);
        if (viewport[0] < 0 || viewport[1] < 0 || viewport[2] <= 0 || viewport[3] <= 0 ||
            viewport[2] > maxTextureSize || viewport[3] > maxTextureSize ||
            static_cast<unsigned>(viewport[0]) + viewport[2] > drawableWidth ||
            static_cast<unsigned>(viewport[1]) + viewport[3] > drawableHeight) {
            reportOnce(framebufferReported, "Skipping viewport outside drawable/texture bounds (including resize transitions).");
            return;
        }

        SavedState saved(gl);
        if (!program && !createProgram(gl)) {
            failed = true;
            std::fprintf(stderr, "[OpenGL Inject] Injection disabled for this context; swaps continue.\n");
            return;
        }
        if (!texture) {
            glGenTextures(1, &texture);
            if (!texture) {
                failed = true;
                std::fprintf(stderr, "[OpenGL Inject] Cannot create capture texture; injection disabled.\n");
                return;
            }
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, texture);
        gl.bindSampler(0, 0);
        if (width != viewport[2] || height != viewport[3]) {
            gl.bindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, viewport[2], viewport[3],
                         0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            if (width != viewport[2] || height != viewport[3]) {
                failed = true;
                std::fprintf(stderr, "[OpenGL Inject] Texture allocation failed; injection disabled.\n");
                return;
            }
        }
        glReadBuffer(GL_BACK);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], width, height);
        glDrawBuffer(GL_BACK);
        for (GLenum capability : disabledDuringInset) glDisable(capability);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gl.useProgram(program);
        gl.uniform1i(samplerLocation, 0);
        gl.uniform4f(viewportLocation, viewport[0], viewport[1], width, height);
        gl.uniform4f(insetLocation, insetLeft, insetBottom, insetSize, insetSize);
        glBegin(GL_QUADS);
        glVertex2f(insetLeft, insetBottom);
        glVertex2f(insetLeft + insetSize, insetBottom);
        glVertex2f(insetLeft + insetSize, insetBottom + insetSize);
        glVertex2f(insetLeft, insetBottom + insetSize);
        glEnd();
    }
};

const GlFunctions& glFunctions() {
    static const GlFunctions gl;
    return gl;
}

std::mutex renderMutex;
InjectedRenderContext injected;

void tryRender(Display* display, GLXDrawable drawable) {
    std::unique_lock<std::mutex> lock(renderMutex, std::try_to_lock);
    if (!lock) return;
    GLXContext current = glXGetCurrentContext();
    if (!current || display != glXGetCurrentDisplay() ||
        drawable != glXGetCurrentDrawable() || drawable != glXGetCurrentReadDrawable()) {
        reportOnce(injected.unsupportedReported, "Skipping swap without matching current GLX context/drawables.");
        return;
    }
    if (injected.context &&
        (injected.context != current || injected.thread != std::this_thread::get_id())) {
        reportOnce(injected.unsupportedReported, "Skipping additional context or rendering thread.");
        return;
    }
    if (!injected.context) {
        int major = 0, minor = 0;
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        if (!version || std::sscanf(version, "%d.%d", &major, &minor) != 2 ||
            major < 3 || (major == 3 && minor < 3)) {
            reportOnce(injected.unsupportedReported, "Requires desktop OpenGL 3.3+ compatibility profile.");
            return;
        }
        GLint profile;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
        if (!(profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)) {
            reportOnce(injected.unsupportedReported, "Core-profile contexts are not supported.");
            return;
        }
        injected.context = current;
        injected.thread = std::this_thread::get_id();
    }
    const auto& gl = glFunctions();
    if (!gl.ready) return;
    try {
        injected.render(gl, display, drawable);
    } catch (const std::exception& error) {
        injected.failed = true;
        std::fprintf(stderr, "[OpenGL Inject] Injection disabled after exception: %s\n", error.what());
    }
}

} // namespace

extern "C" void glXSwapBuffers(Display* display, GLXDrawable drawable) {
    using Swap = void (*)(Display*, GLXDrawable);
    static const Swap original = [] {
        Swap function = nullptr;
        if (!loadNextSymbol(function, "glXSwapBuffers"))
            std::fprintf(stderr, "[OpenGL Inject] Real glXSwapBuffers unavailable; cannot forward swap.\n");
        return function;
    }();
    if (!original) return;
    tryRender(display, drawable);
    original(display, drawable);
}

extern "C" void glXDestroyContext(Display* display, GLXContext context) {
    using Destroy = void (*)(Display*, GLXContext);
    static const Destroy original = [] {
        Destroy function = nullptr;
        if (!loadNextSymbol(function, "glXDestroyContext"))
            std::fprintf(stderr, "[OpenGL Inject] Real glXDestroyContext unavailable; cannot forward destruction.\n");
        return function;
    }();
    if (!original) return;
    {
        std::lock_guard<std::mutex> lock(renderMutex);
        if (context && injected.context == context) {
            // GL deletion is only legal while this context is current.
            if (glXGetCurrentContext() == context) {
                const auto& gl = glFunctions();
                if (gl.ready) {
                    if (injected.texture) glDeleteTextures(1, &injected.texture);
                    if (injected.program) gl.deleteProgram(injected.program);
                }
            }
            injected = InjectedRenderContext{};
        }
    }
    original(display, context);
}
