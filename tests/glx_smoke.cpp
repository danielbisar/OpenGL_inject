#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

constexpr std::array<GLenum, 10> capabilities = {
    GL_DEPTH_TEST, GL_SCISSOR_TEST, GL_BLEND, GL_CULL_FACE, GL_STENCIL_TEST,
    GL_ALPHA_TEST, GL_COLOR_LOGIC_OP, GL_LIGHTING, GL_FOG, GL_RASTERIZER_DISCARD
};
constexpr std::array<GLenum, 14> integerStates = {
    GL_CURRENT_PROGRAM, GL_MATRIX_MODE, GL_ACTIVE_TEXTURE, GL_PIXEL_UNPACK_BUFFER_BINDING,
    GL_READ_BUFFER, GL_DRAW_BUFFER, GL_READ_FRAMEBUFFER_BINDING, GL_DRAW_FRAMEBUFFER_BINDING,
    GL_MODELVIEW_STACK_DEPTH, GL_PROJECTION_STACK_DEPTH, GL_TEXTURE_STACK_DEPTH,
    GL_DEPTH_FUNC, GL_CULL_FACE_MODE, GL_LOGIC_OP_MODE
};
struct Snapshot {
    std::array<GLint, integerStates.size()> integers{};
    std::array<GLfloat, 48> matrices{};
    std::array<GLint, 4> viewport{}, scissor{};
    std::array<GLint, 2> textures{}, samplers{}, polygon{};
    std::array<GLboolean, 4> colorMask{};
    std::array<GLboolean, capabilities.size()> enabled{};

    Snapshot() {
        for (std::size_t i = 0; i < integers.size(); ++i)
            glGetIntegerv(integerStates[i], &integers[i]);
        glGetFloatv(GL_PROJECTION_MATRIX, matrices.data());
        glGetFloatv(GL_MODELVIEW_MATRIX, matrices.data() + 16);
        glGetFloatv(GL_TEXTURE_MATRIX, matrices.data() + 32);
        glGetIntegerv(GL_VIEWPORT, viewport.data());
        glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
        glGetIntegerv(GL_POLYGON_MODE, polygon.data());
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMask.data());
        for (std::size_t i = 0; i < enabled.size(); ++i)
            enabled[i] = glIsEnabled(capabilities[i]);
        GLint active;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        for (unsigned i = 0; i < 2; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures[i]);
            glGetIntegerv(GL_SAMPLER_BINDING, &samplers[i]);
        }
        glActiveTexture(active);
    }

    void verify() const {
        Snapshot after;
        require(integers == after.integers, "program/matrix mode/buffer/stack state changed");
        require(matrices == after.matrices, "host matrices changed");
        require(viewport == after.viewport && scissor == after.scissor, "viewport/scissor changed");
        require(textures == after.textures && samplers == after.samplers, "texture/sampler bindings changed");
        require(polygon == after.polygon && colorMask == after.colorMask, "polygon/color mask changed");
        require(enabled == after.enabled, "enable state changed");
        require(glGetError() == GL_NO_ERROR, "OpenGL error after swap");
    }
};

GLuint hostProgram() {
    const char* text = "#version 330 compatibility\nout vec4 color;\nvoid main(){color=vec4(0,1,0,1);}";
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    require(success, "host shader compilation failed");
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    require(success, "host program link failed");
    glDeleteShader(shader);
    return program;
}
}

int main(int argc, char** argv) {
    require(argc == 2, "expected mode: injected, fallback, contexts, framebuffer, core, or no-swap");
    const std::string mode = argv[1];
    const bool core = mode == "core";
    Display* display = XOpenDisplay(nullptr);
    require(display, "cannot open X display");
    const int attributes[] = {GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                             GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_DOUBLEBUFFER, True,
                             GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8,
                             GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, None};
    int count = 0;
    GLXFBConfig* configs = glXChooseFBConfig(display, DefaultScreen(display), attributes, &count);
    require(configs && count, "no double-buffered GLX config");
    XVisualInfo* visual = glXGetVisualFromFBConfig(display, configs[0]);
    require(visual, "no GLX visual");
    XSetWindowAttributes windowAttributes{};
    windowAttributes.colormap = XCreateColormap(display, RootWindow(display, visual->screen),
                                               visual->visual, AllocNone);
    Window window = XCreateWindow(display, RootWindow(display, visual->screen), 0, 0, 320, 240,
                                  0, visual->depth, InputOutput, visual->visual,
                                  CWColormap, &windowAttributes);
    XMapWindow(display, window);
    XSync(display, False);
    auto createContext = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(
        glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));
    require(createContext, "no GLX_ARB_create_context");
    const int contextAttributes[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 3,
        GLX_CONTEXT_PROFILE_MASK_ARB,
        core ? GLX_CONTEXT_CORE_PROFILE_BIT_ARB : GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, None
    };
    GLXContext context = createContext(display, configs[0], nullptr, True, contextAttributes);
    require(context && glXMakeCurrent(display, window, context), "cannot make GLX context current");
    std::printf("GL: %s; %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    auto swaps = reinterpret_cast<int (*)()>(dlsym(RTLD_DEFAULT, "probeSwapCount"));
    auto expectPixel = reinterpret_cast<void (*)(int, int, bool)>(dlsym(RTLD_DEFAULT, "probeExpectPixel"));
    auto allocations = reinterpret_cast<int (*)()>(dlsym(RTLD_DEFAULT, "probeAllocationCount"));
    auto copies = reinterpret_cast<int (*)()>(dlsym(RTLD_DEFAULT, "probeCopyCount"));
    require(swaps && expectPixel && allocations && copies, "swap probe not preloaded");

    const auto clearHost = [](int width, int height) {
        glDisable(GL_RASTERIZER_DISCARD);
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, width / 2, height);
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    };

    if (core) {
        clearHost(320, 240);
        expectPixel(270, 200, false);
        glXSwapBuffers(display, window);
    } else {
        GLuint program = hostProgram();
        GLuint textures[2], samplers[2], unpack;
        glGenTextures(2, textures);
        glGenSamplers(2, samplers);
        glGenBuffers(1, &unpack);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpack);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, 16, nullptr, GL_STATIC_DRAW);
        for (unsigned i = 0; i < 2; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glBindSampler(i, samplers[i]);
        }
        glUseProgram(program);
        for (int frame = 0; frame < (mode == "no-swap" ? 1 : 12); ++frame) {
            const int width = frame % 4 < 2 ? 320 : 480;
            const int height = frame % 4 < 2 ? 240 : 360;
            XResizeWindow(display, window, width, height);
            XSync(display, False);
            glXWaitX();
            glViewport(11, 13, width - 22, height - 26);
            clearHost(width, height);
            glReadBuffer(GL_BACK);
            unsigned char hostPixel[3]{};
            glReadPixels(width * 0.85, height * 0.85, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, hostPixel);
            require(hostPixel[1] > 240 && hostPixel[0] < 15, "host backbuffer not ready after resize");
            glReadBuffer(GL_FRONT);
            glDrawBuffer(GL_BACK);
            glScissor(0, 0, 1, 1);
            for (GLenum capability : capabilities) glEnable(capability);
            glDepthFunc(GL_NEVER);
            glBlendFunc(GL_ZERO, GL_ZERO);
            glCullFace(GL_FRONT_AND_BACK);
            glStencilFunc(GL_NEVER, 0, ~0u);
            glAlphaFunc(GL_NEVER, 0);
            glLogicOp(GL_XOR);
            glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_POINT);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glTranslatef(0.2f, 0.3f, 0);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glScalef(2, 3, 4);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glTranslatef(0.1f, 0.2f, 0);
            const GLenum modes[] = {GL_PROJECTION, GL_MODELVIEW, GL_TEXTURE};
            glMatrixMode(modes[frame % 3]);
            GLuint readFramebuffer = 0;
            if (mode == "framebuffer") {
                glGenFramebuffers(1, &readFramebuffer);
                glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
            }
            Snapshot before;
            require(glGetError() == GL_NO_ERROR, "host setup error");
            expectPixel(11 + (width - 22) * 0.85, 13 + (height - 26) * 0.85,
                        mode == "injected" || mode == "contexts");
            glXSwapBuffers(display, window);
            before.verify();
            if (readFramebuffer) {
                glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
                glDeleteFramebuffers(1, &readFramebuffer);
            }
        }
        glUseProgram(0);
        glDeleteProgram(program);
        glDeleteTextures(2, textures);
        glDeleteSamplers(2, samplers);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glDeleteBuffers(1, &unpack);

        if (mode == "contexts") {
            GLXContext other = createContext(display, configs[0], nullptr, True, contextAttributes);
            require(other && glXMakeCurrent(display, window, other), "second context failed");
            clearHost(480, 360);
            expectPixel(400, 300, false);
            glXSwapBuffers(display, window);
            glXMakeCurrent(display, None, nullptr);
            glXDestroyContext(display, other);
            std::thread otherThread([&] {
                require(glXMakeCurrent(display, window, context), "context migration failed");
                clearHost(480, 360);
                expectPixel(400, 300, false);
                glXSwapBuffers(display, window);
                glXMakeCurrent(display, None, nullptr);
            });
            otherThread.join();
            require(glXMakeCurrent(display, window, context), "cannot restore original context");
            glXDestroyContext(display, context);
            glXMakeCurrent(display, None, nullptr);
            context = createContext(display, configs[0], nullptr, True, contextAttributes);
            require(context && glXMakeCurrent(display, window, context), "replacement context failed");
            glViewport(0, 0, 480, 360);
            clearHost(480, 360);
            expectPixel(400, 300, true);
            Snapshot before;
            glXSwapBuffers(display, window);
            before.verify();
        }
    }
    const bool injected = mode == "injected" || mode == "contexts";
    require(allocations() == (mode == "contexts" ? 7 : injected ? 6 : 0),
            "texture storage must be allocated only on initial capture or size changes");
    require(copies() == (mode == "contexts" ? 13 : injected ? 12 : 0),
            "capture must use subimage updates on every rendered frame");
    require(swaps() == (mode == "no-swap" ? 0 : core ? 1 : mode == "contexts" ? 15 : 12),
            "real swap not forwarded");
    glXDestroyContext(display, context);
    glXMakeCurrent(display, None, nullptr);
    XDestroyWindow(display, window);
    XFreeColormap(display, windowAttributes.colormap);
    XFree(visual);
    XFree(configs);
    XCloseDisplay(display);
    std::printf("PASS: %s (%s)\n", mode.c_str(), mode == "no-swap"
        ? "safe return when real swap is unavailable" : "pixels, state and forwarded swaps");
}
