#ifndef USED_OPENGL_FUNCTIONS_H
#define USED_OPENGL_FUNCTIONS_H

#include <stdio.h>
#include <GL/gl.h>
#include <dlfcn.h>

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

typedef void (*glLinkProgram_t)(GLuint program);
static glLinkProgram_t glLinkProgram = NULL;

typedef void (*glShaderSource_t)(GLuint shader,	GLsizei count, const GLchar **string, const GLint *length);
static glShaderSource_t glShaderSource = NULL;

typedef void (*glUseProgram_t)(GLuint program);
static glUseProgram_t glUseProgram = NULL;

#define LOAD_FUNCTION(target, name, type) \
    {\
        printf("Load " name ": "); \
        target = (type)dlsym(RTLD_NEXT, name); \
        if (!target) { \
            printf("failed\n"); \
        } else { \
            printf("%p\n", target); \
        } \
    }

void assureGlFunctionsLoaded(){
    if (!glCreateShader) {
        LOAD_FUNCTION(glAttachShader, "glAttachShader", glAttachShader_t);
        LOAD_FUNCTION(glCreateProgram, "glCreateProgram", glCreateProgram_t);
        LOAD_FUNCTION(glCompileShader, "glCompileShader", glCompileShader_t);
        LOAD_FUNCTION(glCreateShader, "glCreateShader", glCreateShader_t);
        LOAD_FUNCTION(glGetProgramiv, "glGetProgramiv", glGetProgramiv_t);
        LOAD_FUNCTION(glGetProgramInfoLog, "glGetProgramInfoLog", glGetProgramInfoLog_t);
        LOAD_FUNCTION(glGetShaderiv, "glGetShaderiv", glGetShaderiv_t);
        LOAD_FUNCTION(glGetShaderInfoLog, "glGetShaderInfoLog", glGetShaderInfoLog_t);
        LOAD_FUNCTION(glLinkProgram, "glLinkProgram", glLinkProgram_t);
        LOAD_FUNCTION(glShaderSource, "glShaderSource", glShaderSource_t);
        LOAD_FUNCTION(glUseProgram, "glUseProgram", glUseProgram_t);
    }
}

#endif // USED_OPENGL_FUNCTIONS_H
