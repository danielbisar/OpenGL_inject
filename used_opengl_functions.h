#ifndef USED_OPENGL_FUNCTIONS_H
#define USED_OPENGL_FUNCTIONS_H

#include <GL/gl.h>
#include <GL/glext.h>
#include <dlfcn.h>
#include <cstdio>

template <typename T>
bool loadNextSymbol(T& target, const char* name) {
    dlerror();
    target = reinterpret_cast<T>(dlsym(RTLD_NEXT, name));
    const char* error = dlerror();
    if (error || !target) {
        std::fprintf(stderr, "[OpenGL Inject] Cannot resolve %s: %s\n",
                     name, error ? error : "null address");
        target = nullptr;
        return false;
    }
    return true;
}

struct GlFunctions {
    PFNGLCREATESHADERPROC createShader = nullptr;
    PFNGLSHADERSOURCEPROC shaderSource = nullptr;
    PFNGLCOMPILESHADERPROC compileShader = nullptr;
    PFNGLGETSHADERIVPROC getShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC getShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC deleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC createProgram = nullptr;
    PFNGLATTACHSHADERPROC attachShader = nullptr;
    PFNGLLINKPROGRAMPROC linkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC getProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog = nullptr;
    PFNGLDELETEPROGRAMPROC deleteProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC getUniformLocation = nullptr;
    PFNGLUNIFORM1IPROC uniform1i = nullptr;
    PFNGLUNIFORM4FPROC uniform4f = nullptr;
    PFNGLUSEPROGRAMPROC useProgram = nullptr;
    PFNGLBINDBUFFERPROC bindBuffer = nullptr;
    PFNGLBINDSAMPLERPROC bindSampler = nullptr;
    bool ready = true;

    GlFunctions() {
#define LOAD(member, name) ready = loadNextSymbol(member, name) && ready
        LOAD(createShader, "glCreateShader");
        LOAD(shaderSource, "glShaderSource");
        LOAD(compileShader, "glCompileShader");
        LOAD(getShaderiv, "glGetShaderiv");
        LOAD(getShaderInfoLog, "glGetShaderInfoLog");
        LOAD(deleteShader, "glDeleteShader");
        LOAD(createProgram, "glCreateProgram");
        LOAD(attachShader, "glAttachShader");
        LOAD(linkProgram, "glLinkProgram");
        LOAD(getProgramiv, "glGetProgramiv");
        LOAD(getProgramInfoLog, "glGetProgramInfoLog");
        LOAD(deleteProgram, "glDeleteProgram");
        LOAD(getUniformLocation, "glGetUniformLocation");
        LOAD(uniform1i, "glUniform1i");
        LOAD(uniform4f, "glUniform4f");
        LOAD(useProgram, "glUseProgram");
        LOAD(bindBuffer, "glBindBuffer");
        LOAD(bindSampler, "glBindSampler");
#undef LOAD
        if (!ready)
            std::fprintf(stderr, "[OpenGL Inject] Required GL symbols missing; injection disabled.\n");
    }
};

#endif
