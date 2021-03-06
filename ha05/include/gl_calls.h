#ifndef GLCALLS_H
#define GLCALLS_H

// Include the GLAD loader *before* including GLFW!
#include "glad/glad.h"

// Include the GLFW library (should be the same for all OS):
#include <GLFW/glfw3.h>

#define ATTRIB_POSITION 0
#define ATTRIB_COLOR 1
#define ATTRIB_TEXCOORD 2

#define DELTA_TIME_INCREMENT 0.01f

// TODO
typedef struct
{
    // Dimensions of the window:
    int window_width;
    int window_height;

    // Shader program handle:
    GLuint shader_program;

    // VAO:
    GLuint vao;

    // VBO:
    GLuint vbo;

    GLuint tex;

    float timer;
    float delta_time;

    uint8_t suprise;
} user_data_t;

typedef struct
{
    GLfloat position[3];
    GLubyte color[3];
    GLfloat texCoords[2];
} vertex_data_t;

// Generic error checks:
void check_error(int condition, const char* error_text);
void gl_check_error(const char* error_text);

// GL functions:
void init_gl(GLFWwindow* window, int number_of_edges);
void draw_gl(GLFWwindow* window, int number_of_edges);
void teardown_gl(GLFWwindow* window);

#endif
