// handles loading of OpenGL functions dynamically (assureGlFunctionsLoaded)

// based on: https://aixxe.net/2016/12/imgui-linux-csgo

#include "used_opengl_functions.h"

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <string>
#include <fstream>
#include <sstream>

class InjectedRenderContext {
private:
    std::string loadShaderSource() {
        std::ifstream shaderFile("fragment.shader");
        if (!shaderFile.is_open()) {
            printf("Failed to open fragment shader file.\n");
            return nullptr;
        }

        std::ostringstream buffer;
        buffer << shaderFile.rdbuf(); // Read the file's content into the buffer
        std::string shaderCode = buffer.str(); // Convert buffer to string
        shaderFile.close();

        return shaderCode;
    }
public:
    int viewportWidth = 0;
    int viewportHeight = 0;
    int textureWidth = 0;
    int textureHeight = 0;
    GLuint _texture = 0;
    GLuint _fragmentShader = 0;
    GLuint _shaderProgram = 0;
    GLint _textureUniformInputLocation = 0;
    clock_t _lastTextureUpdate = 0;

    InjectedRenderContext(){
    }

    void readViewportInfos(){
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        viewportWidth = viewport[2];
        viewportHeight = viewport[3];
    }

    void assureTexture() {
        if (_texture == 0 || textureWidth != viewportWidth || textureHeight != viewportHeight) {

            // reduce the frequency of texture updates
            // to avoid performance issues with frequent texture recreation (complete hangs)
            if(_lastTextureUpdate == 0)
                _lastTextureUpdate = clock();
            else if (clock() - _lastTextureUpdate < CLOCKS_PER_SEC / 2) {
                //printf("Skipping texture update, last update was too recent.\n");
                return;
            }

            _lastTextureUpdate = clock();

            if(_texture != 0) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glDeleteTextures(1, &_texture);
            }

            printf("Creating texture with size %dx%d\n", viewportWidth, viewportHeight);

            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

            textureWidth = viewportWidth;
            textureHeight = viewportHeight;

            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void saveOpenGlState() {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
    }

    void restoreOpenGlState() {
        glPopMatrix();
        glPopAttrib();
    }

    void copyBackbufferToTexture() {
        glBindTexture(GL_TEXTURE_2D, _texture);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, viewportWidth, viewportHeight, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void assureShader() {
        if(_shaderProgram == 0)
        {
            _fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            printf("Fragment shader created: %u\n", _fragmentShader);

            std::string shaderSource = loadShaderSource();
            const char* shaderSourceCStr = shaderSource.c_str();
            glShaderSource(_fragmentShader, 1, &shaderSourceCStr, NULL);
            glCompileShader(_fragmentShader);

            GLint success;
            glGetShaderiv(_fragmentShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                GLchar infoLog[512];
                glGetShaderInfoLog(_fragmentShader, 512, NULL, infoLog);
                printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
            } else {
                printf("Fragment shader compiled successfully.\n");
            }

            _shaderProgram = glCreateProgram();
            glAttachShader(_shaderProgram, _fragmentShader);
            glLinkProgram(_shaderProgram);

            _textureUniformInputLocation = glGetUniformLocation(_shaderProgram, "screenTexture");
            glUniform1i(_textureUniformInputLocation, 0); // Texture unit 0

            glUseProgram(0);
            //delete shaderSource;
        }
    }
};

static InjectedRenderContext _injectedRenderContext = InjectedRenderContext();

float left = 0.7f;   // right edge minus quad width
float right = 1.0f;  // at the very right
float bottom = 0.7f; // top edge minus quad height
float top = 1.0f;    // at the very top

void render() {
    _injectedRenderContext.readViewportInfos();
    _injectedRenderContext.saveOpenGlState();
    _injectedRenderContext.assureShader();

    _injectedRenderContext.assureTexture();
    _injectedRenderContext.copyBackbufferToTexture();

    // Set up a simple orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);       // optional, if blending is undesired
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // ensure full texture color

    glUseProgram(_injectedRenderContext._shaderProgram);

    //glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _injectedRenderContext._texture);
    glUniform1i(_injectedRenderContext._textureUniformInputLocation, 0); // Texture unit 0


    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(left, bottom);   // bottom left
        glTexCoord2f(1.0f, 0.0f); glVertex2f(right, bottom);  // bottom right
        glTexCoord2f(1.0f, 1.0f); glVertex2f(right, top);     // top right
        glTexCoord2f(0.0f, 1.0f); glVertex2f(left, top);      // top left
    glEnd();


    glBindTexture(GL_TEXTURE_2D, 0);
    //glDisable(GL_TEXTURE_2D);

    glUseProgram(0);

    // Restore matrices and state
    glPopMatrix(); // MODELVIEW
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    _injectedRenderContext.restoreOpenGlState();
}

void glXSwapBuffers(Display* display, GLXDrawable drawable) {

    assureGlFunctionsLoaded();
    render();

    glXSwapBuffers_original(display, drawable);
}
