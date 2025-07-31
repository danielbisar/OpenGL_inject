#ifndef USED_OPENGL_FUNCTIONS_H
#define USED_OPENGL_FUNCTIONS_H

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <dlfcn.h>

static bool used_opengl_functions_loaded = false;

typedef void (*glAttachShader_t)(GLuint program, GLuint shader);
static glAttachShader_t glAttachShader = NULL;

typedef GLuint (*glCreateProgram_t)(void);
static glCreateProgram_t glCreateProgram = NULL;

typedef void (*glCompileShader_t)(GLuint shader);
static glCompileShader_t glCompileShader = NULL;

typedef GLuint (*glCreateShader_t)(GLenum shaderType);
static glCreateShader_t glCreateShader = NULL;

typedef void (*glGetProgramiv_t)(GLuint program, GLenum pname, GLint *params);
static glGetProgramiv_t glGetProgramiv = NULL;

typedef void (*glGetProgramInfoLog_t)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static glGetProgramInfoLog_t glGetProgramInfoLog = NULL;

typedef void (*glGetShaderiv_t)(GLuint shader, GLenum pname, GLint *params);
static glGetShaderiv_t glGetShaderiv = NULL;

typedef void (*glGetShaderInfoLog_t)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static glGetShaderInfoLog_t glGetShaderInfoLog = NULL;

typedef GLint (*glGetUniformLocation_t)(GLuint program, const GLchar *name);
static glGetUniformLocation_t glGetUniformLocation = NULL;

typedef void (*glLinkProgram_t)(GLuint program);
static glLinkProgram_t glLinkProgram = NULL;

typedef void (*glShaderSource_t)(GLuint shader,	GLsizei count, const GLchar **string, const GLint *length);
static glShaderSource_t glShaderSource = NULL;

typedef void (*glUniform1i_t)(GLint location, GLint v0);
static glUniform1i_t glUniform1i = NULL;

typedef void (*glUseProgram_t)(GLuint program);
static glUseProgram_t glUseProgram = NULL;

typedef void (*glXSwapBuffers_t)(Display *display, GLXDrawable drawable);
static glXSwapBuffers_t glXSwapBuffers_original = NULL;


#define LOAD_GL_FUNCTION(target, name, type) \
    {\
        printf("Load " name ": "); \
        target = (type)dlsym(RTLD_NEXT, name); \
        if (!target) { \
            printf("failed\n"); \
        } else { \
            printf("%p\n", target); \
        } \
    }

#define LOAD_GL_FUNCTION_ORIGINAL(target, name, type) \
    {\
        printf("Load original " name ": "); \
        target = (type)dlsym(RTLD_NEXT, name); \
        if (!target) { \
            printf("failed\n"); \
        } else { \
            printf("%p\n", target); \
        } \
    }

void assureGlFunctionsLoaded(){
    if (!used_opengl_functions_loaded) {
        LOAD_GL_FUNCTION(glAttachShader, "glAttachShader", glAttachShader_t);
        LOAD_GL_FUNCTION(glCreateProgram, "glCreateProgram", glCreateProgram_t);
        LOAD_GL_FUNCTION(glCompileShader, "glCompileShader", glCompileShader_t);
        LOAD_GL_FUNCTION(glCreateShader, "glCreateShader", glCreateShader_t);
        LOAD_GL_FUNCTION(glGetProgramiv, "glGetProgramiv", glGetProgramiv_t);
        LOAD_GL_FUNCTION(glGetProgramInfoLog, "glGetProgramInfoLog", glGetProgramInfoLog_t);
        LOAD_GL_FUNCTION(glGetShaderiv, "glGetShaderiv", glGetShaderiv_t);
        LOAD_GL_FUNCTION(glGetShaderInfoLog, "glGetShaderInfoLog", glGetShaderInfoLog_t);
        LOAD_GL_FUNCTION(glGetUniformLocation, "glGetUniformLocation", glGetUniformLocation_t);
        LOAD_GL_FUNCTION(glLinkProgram, "glLinkProgram", glLinkProgram_t);
        LOAD_GL_FUNCTION(glShaderSource, "glShaderSource", glShaderSource_t);
        LOAD_GL_FUNCTION(glUniform1i, "glUniform1i", glUniform1i_t);
        LOAD_GL_FUNCTION(glUseProgram, "glUseProgram", glUseProgram_t);

        LOAD_GL_FUNCTION_ORIGINAL(glXSwapBuffers_original, "glXSwapBuffers", glXSwapBuffers_t);

        used_opengl_functions_loaded = true;
    }
}

#endif // USED_OPENGL_FUNCTIONS_H
