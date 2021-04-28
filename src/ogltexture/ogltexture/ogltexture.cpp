
#include <stdlib.h>
#include <iostream>
#include "GL/glew.h"
#include "GL/glut.h"


#include "PhotronOpenGLCapture.h"

photron::OpenGLCapture cap;
GLuint vao;
GLuint vbo;
GLuint vertexShader, fragmentShader, shaderProgram;
GLuint ebo;

// Shader sources
const GLchar* vertexSource = R"glsl(
    #version 150 core
    in vec2 position;
    in vec3 color;
    in vec2 texcoord;
    out vec3 Color;
    out vec2 Texcoord;
    void main()
    {
        Color = color;
        Texcoord = texcoord;
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";

const GLchar* fragmentSource_identity = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    void main()
    {
        outColor = texture(image, Texcoord);
    }
)glsl";

const GLchar* fragmentSource_thermal = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    vec3 hsv2rgb(vec3 c)
    {
        vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }
    void main()
    {
        outColor = texture(image, Texcoord);
        // Infra red shader
        float luma = mix(0.7, 1.0, outColor.r);
        outColor.rgb = hsv2rgb(vec3(luma, 1.0, 1.0));
    }
)glsl";

const GLchar* fragmentSource_edge = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    void main()
    {
        ivec2 size = textureSize(image, 0);
        float w = 1.0 / float(size.x);
        float h = 1.0 / float(size.y);
        outColor = texture(image, Texcoord)*8.0;
        outColor -= texture(image, Texcoord + vec2( -w, -h));
        outColor -= texture(image, Texcoord + vec2( -w, 0.0));
        outColor -= texture(image, Texcoord + vec2( -w, h));
        outColor -= texture(image, Texcoord + vec2( 0.0, -h));
        outColor -= texture(image, Texcoord + vec2( 0.0,  h));
        outColor -= texture(image, Texcoord + vec2(  w, -h));
        outColor -= texture(image, Texcoord + vec2(  w, 0.0));
        outColor -= texture(image, Texcoord + vec2(  w, h));
        outColor.rgb *= vec3(5.0);
    }
)glsl";

const GLchar* fragmentSource_invert = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    void main()
    {
        outColor = texture(image, Texcoord);
        outColor.rgb = vec3(1.0)-outColor.rgb;
    }
)glsl";

const GLchar* fragmentSource_flip_horizontal = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    void main()
    {
        outColor = texture(image, vec2(1.0-Texcoord.x, Texcoord.y));
    }
)glsl";

const GLchar* fragmentSource_flip_vertical = R"glsl(
    #version 150 core
    in vec3 Color;
    in vec2 Texcoord;
    out vec4 outColor;
    uniform sampler2D image;
    void main()
    {
        outColor = texture(image, vec2(Texcoord.x, 1.0-Texcoord.y));
    }
)glsl";

enum {
    SHADER_IDENTITY,
    SHADER_THERMAL,
    SHADER_EDGE,
    SHADER_INVERT,
    SHADER_FLIP_HORIZONTAL,
    SHADER_FLIP_VERTICAL,
};

int currentShader = SHADER_THERMAL;

const GLchar* fragmentSource[] = {
    fragmentSource_identity,
    fragmentSource_thermal,
    fragmentSource_edge,
    fragmentSource_invert,
    fragmentSource_flip_horizontal,
    fragmentSource_flip_vertical

};

void renderBitmapString(float x, float y, void* font, const char* string) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void Display(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glUseProgram(0);

    glColor3f(1.f, 1.f, 1.f);
    renderBitmapString(-0.8, 0.9, (void*)GLUT_BITMAP_9_BY_15, "Shader: Select Right Click Menu");

    glutSwapBuffers();
    glutPostRedisplay();
}

void Reshape(int w, int h)
{
    glViewport(0, 0, w, h);
}


void createShaderProgram() {

    // Create and compile the vertex shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource[currentShader], NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), 0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
    glUniform1i(glGetUniformLocation(shaderProgram, "image"), 0);

    glUseProgram(0);
}

void destroyShaderProgram() {
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}

void Setup()
{
    glewInit();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);

    GLfloat vertices[] = {
        //  Position      Color             Texcoords
            -1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // Top-left
             1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // Top-right
             1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Bottom-right
            -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // Bottom-left
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create an element array
    glGenBuffers(1, &ebo);

    GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    createShaderProgram();


    glActiveTexture(GL_TEXTURE0);
    cap.createTexture();

}

void IdleFunc() {
    glActiveTexture(GL_TEXTURE0);
    cap.updateTexture();
}

void Terminate() {
    cap.deleteTexture();
    destroyShaderProgram();
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

static int window;

void menu(int num) {
    if (num == -1) {
        glutDestroyWindow(window);
        exit(0);
    }

    currentShader = num;
    destroyShaderProgram();
    createShaderProgram();

    glutPostRedisplay();
}



void createMenu(void) {
    int submenu_id = glutCreateMenu(menu);
    glutAddMenuEntry("Identity", SHADER_IDENTITY);
    glutAddMenuEntry("Thermal", SHADER_THERMAL);
    glutAddMenuEntry("Edge Detect", SHADER_EDGE);
    glutAddMenuEntry("Invert", SHADER_INVERT);
    glutAddMenuEntry("Flip Horizontal", SHADER_FLIP_HORIZONTAL);
    glutAddMenuEntry("Flip Vertical", SHADER_FLIP_VERTICAL);
    int menu_id = glutCreateMenu(menu);
    glutAddSubMenu("Shader", submenu_id);
    glutAddMenuEntry("Quit", -1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char* argv[])
{
    int width = 1280;
    int height = 1080;


    int deviceID = 0;             // 0 = open default camera
    // open selected camera using selected API
    cap.open(deviceID);
    // check if we succeeded
    if (!cap.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        return -1;
    }

    cap.getResolution(width, height);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    window = glutCreateWindow("Photron OpenGL Infinicam Demo");
    createMenu();

    Setup();

    // Set GLUT callbacks
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutIdleFunc(IdleFunc);
    atexit(Terminate);  // Called after glutMainLoop ends

    glutMainLoop();
   
    return 0;
}