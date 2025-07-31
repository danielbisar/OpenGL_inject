// handles loading of OpenGL functions dynamically (assureGlFunctionsLoaded)
#include "used_opengl_functions.h"

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glx.h>

class InjectedRenderContext {
public:
    int viewportWidth = 0;
    int viewportHeight = 0;
    GLuint _texture = 0;

    InjectedRenderContext(){
    }

    void readViewportInfos(){
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        viewportWidth = viewport[2];
        viewportHeight = viewport[3];
    }

    void assureTexture() {
        if (_texture == 0) {
            glGenTextures(1, &_texture);
            glBindTexture(GL_TEXTURE_2D, _texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, viewportWidth, viewportHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

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
};

static InjectedRenderContext _injectedRenderContext = InjectedRenderContext();

void render() {
    _injectedRenderContext.readViewportInfos();
    _injectedRenderContext.saveOpenGlState();

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

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _injectedRenderContext._texture);

    // Set color to red
    glColor3f(1.0f, 0.0f, 0.0f);

    // Draw textured quad in the 0.2-0.8 rectangle
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.2f, 0.2f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(0.8f, 0.2f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(0.8f, 0.8f);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.2f, 0.8f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

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
