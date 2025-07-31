#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GLFW/glfw3.h>

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
        printf("Try to load " name "...\n"); \
        target = (type)dlsym(RTLD_NEXT, name); \
        if (!target) { \
            printf("Failed to find " name "\n"); \
        } else { \
            printf(#name " loaded: %p\n", target); \
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

// // void render() {
//     // TODO delete shader
//     static int program = 0;
//     static GLuint vertexShader = 0;
//
//     if(vertexShader == 0) {
//         vertexShader = glCreateShader(GL_FRAGMENT_SHADER);
//
//         if (vertexShader == 0) {
//             printf("Failed to create vertex shader\n");
//             return;
//         }
//
//         printf("Vertex shader created successfully: %u\n", vertexShader);
//
//         const char* vertexShaderSource = "#version 330 core\nout vec4 FragColor;\nin  vec2 TexCoords;\nuniform \nsampler2D fboAttachment;\nvoid main(){\nFragColor = texture(fboAttachment, TexCoords);\n}\n";
//
//         glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
//         glCompileShader(vertexShader);
//
//         int success;
//         glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//
//         if (!success) {
//             char infoLog[512];
//             glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
//             printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
//             vertexShader = -1;
//         }
//         else
//         {
//             program = glCreateProgram();
//             glAttachShader(program, vertexShader);
//             glLinkProgram(program);
//
//             glGetProgramiv(program, GL_LINK_STATUS, &success);
//
//             if (!success) {
//                 char infoLog[512];
//                 glGetProgramInfoLog(program, 512, NULL, infoLog);
//                 printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
//                 program = 0;
//
//                 //glDestro
//             }
//         }
//     }
//     else
//     {
//         glUseProgram(program);
//         // glActiveTexture(GL_TEXTURE0);
//
//         // glBindTexture(GL_TEXTURE_2D, textureID);
//         // glBindVertexArray(vaoDebugTexturedRect);
//         glDrawArrays(GL_TRIANGLES, 0, 6);
//         // glBindVertexArray(0);
//         glUseProgram(0);
//     }
//
//
//         //
//         //
//
//         //int success;
//         //glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//
// //         if (!success) {
// //             char infoLog[512];
// //             glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
// //             printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
// //             vertexShader = -1;
// //         }
// //         else
// //         {
// //             program = glCreateProgram();
// //             glAttachShader(program, vertexShader);
// //             glLinkProgram(program);
// //
// //             glGetProgramiv(program, GL_LINK_STATUS, &success);
// //
// //             if (!success) {
// //                 char infoLog[512];
// //                 glGetProgramInfoLog(program, 512, NULL, infoLog);
// //                 printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
// //                 program = 0;
// //             }
// //         }
// //
// //         int version = gladLoadGL(glfwGetProcAddress);
// //     if (version == 0) {
// //         printf("Failed to initialize OpenGL context\n");
// //         return -1;
// //     }
//     //}
//
//     if(vertexShader == -1) {
//         return;
//     }
//
//     //glUseProgram(program);
//     //glUseProgram(0);
//
//     // TODO save context and restore it after rendering
//
//     // glActiveTexture(GL_TEXTURE0);
//     // glUseProgram(shaderDisplayFBOOutput);
//     //     glBindTexture(GL_TEXTURE_2D, textureID);
//     //     glBindVertexArray(vaoDebugTexturedRect);
//     //         glDrawArrays(GL_TRIANGLES, 0, 6);
//     //     glBindVertexArray(0);
//     // glUseProgram(0);
// }

void render() {
    // Set up a simple orthographic projection for 2D rendering
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Set color to red
    glColor3f(1.0f, 0.0f, 0.0f);

    // Draw a rectangle between (0.2, 0.2) and (0.8, 0.8) in normalized coordinates
    glBegin(GL_QUADS);
        glVertex2f(0.2f, 0.2f);
        glVertex2f(0.8f, 0.2f);
        glVertex2f(0.8f, 0.8f);
        glVertex2f(0.2f, 0.8f);
    glEnd();

    // Restore matrices and state
    glPopMatrix(); // MODELVIEW
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopMatrix();
    glPopAttrib();
}

void glXSwapBuffers(Display* display, GLXDrawable drawable) {
    static void (*real_glXSwapBuffers)(Display*, GLXDrawable) = NULL;

    if (!real_glXSwapBuffers)
        real_glXSwapBuffers = dlsym(RTLD_NEXT, "glXSwapBuffers");

    assureGlFunctionsLoaded();
    render();

    // Call the real function
    real_glXSwapBuffers(display, drawable);
}
