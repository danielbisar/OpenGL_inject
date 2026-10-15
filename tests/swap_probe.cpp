#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>

namespace {
int swaps = 0;
int allocations = 0, copies = 0;
int pixelX = 0, pixelY = 0;
bool expectInset = false;
}

extern "C" int probeSwapCount() { return swaps; }
extern "C" int probeAllocationCount() { return allocations; }
extern "C" int probeCopyCount() { return copies; }

extern "C" void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const void* data) {
    using Allocate = void (*)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
    static const auto original = reinterpret_cast<Allocate>(dlsym(RTLD_NEXT, "glTexImage2D"));
    if (!original) std::abort();
    ++allocations;
    original(target, level, internalFormat, width, height, border, format, type, data);
}

extern "C" void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                   GLint x, GLint y, GLsizei width, GLsizei height) {
    using Copy = void (*)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    static const auto original = reinterpret_cast<Copy>(dlsym(RTLD_NEXT, "glCopyTexSubImage2D"));
    if (!original) std::abort();
    ++copies;
    original(target, level, xoffset, yoffset, x, y, width, height);
}

extern "C" void probeExpectPixel(int x, int y, bool inset) {
    pixelX = x;
    pixelY = y;
    expectInset = inset;
}

// Loaded after the injector, this observes the backbuffer before the real swap.
extern "C" void glXSwapBuffers(Display* display, GLXDrawable drawable) {
    using Swap = void (*)(Display*, GLXDrawable);
    static const auto original = reinterpret_cast<Swap>(dlsym(RTLD_NEXT, "glXSwapBuffers"));
    if (!original) {
        std::fprintf(stderr, "FAIL: probe cannot find real swap\n");
        std::exit(1);
    }
    if (glGetError() != GL_NO_ERROR) {
        std::fprintf(stderr, "FAIL: injection left an OpenGL error\n");
        std::exit(1);
    }
    GLint readBuffer, readFramebuffer;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    glReadBuffer(GL_BACK);
    unsigned char pixel[3]{};
    glReadPixels(pixelX, pixelY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    glReadBuffer(readBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
    const bool correct = expectInset ? pixel[0] > 240 && pixel[1] < 15
                                     : pixel[0] < 15 && pixel[1] > 240;
    if (!correct || glGetError() != GL_NO_ERROR) {
        std::fprintf(stderr, "FAIL: pre-swap pixel (%d,%d) = %u,%u,%u; inset expected=%d\n",
                     pixelX, pixelY, pixel[0], pixel[1], pixel[2], expectInset);
        std::exit(1);
    }
    original(display, drawable);
    ++swaps;
}
