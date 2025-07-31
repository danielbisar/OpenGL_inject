// handles loading of OpenGL functions dynamically (assureGlFunctionsLoaded)
#include "used_opengl_functions.h"

#include <stdio.h>
#include <GL/gl.h>
#include <GL/glx.h>

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

    assureGlFunctionsLoaded();
    render();

    glXSwapBuffers_original(display, drawable);
}
