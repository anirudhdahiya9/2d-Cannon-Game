#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

typedef struct block {
    VAO * cube;
    VAO * cube_border;
    int type;//0for ground, 1 for water
    bool shm;
    int y;
    int x;
    int z;
}block;
/*
   typedef struct boat{
   int x; int y; int z;
   }boat;
 */
struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if(VertexShaderStream.is_open())
    {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

    // Link the program
    fprintf(stdout, "Linking program\n");
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
            0,                  // attribute 0. Vertices
            3,                  // size (x,y,z)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
            1,                  // attribute 1. Color
            3,                  // size (r,g,b)
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
            );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
int pmov;
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_C:
                rectangle_rot_status = !rectangle_rot_status;
                break;
            case GLFW_KEY_P:
                triangle_rot_status = !triangle_rot_status;
                break;
            case GLFW_KEY_X:
                // do something ..
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_UP:
                pmov=1;
                break;       
            case GLFW_KEY_DOWN:
                pmov=4;
                break;       
            case GLFW_KEY_LEFT:
                pmov=2;
                break;       
            case GLFW_KEY_RIGHT:
                pmov=3;
                break;       
            default:
                // pmov=0;
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
    switch (key) {
        case 'Q':
        case 'q':
            quit(window);
            break;
        default:
            break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
       is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = 90.0f;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // set the projection matrix as perspective
    /* glMatrixMode (GL_PROJECTION);
       glLoadIdentity ();
       gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-25.0f,25.0f,-25.0f,25.0f,-25.0f,25.0f);
}
int block_count=0;
block * barr[400];

// Creates the rectangle object used in this sample code
void createRectangle (block* platform)
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
        -0.9,-0.9,1.05,
        0.9,-0.9,1.05,
        0.9,0.9,1.05,

        0.9,0.9,1.05,
        -0.9,0.9,1.05,
        -0.9,-0.9,1.05,


        1.05,-0.9,0.9,
        1.05,-0.9,-0.9,
        1.05,0.9,-0.9,

        1.05,0.9,-0.9,
        1.05,0.9,0.9,
        1.05,-0.9,0.9,


        -0.9,-0.9,-1.05,
        0.9,-0.9,-1.05,
        0.9,0.9,-1.05,

        0.9,0.9,-1.05,
        -0.9,0.9,-1.05,
        -0.9,-0.9,-1.05,


        -1.05,-0.9,0.9,
        -1.05,-0.9,-0.9,
        -1.05,0.9,-0.9,

        -1.05,0.9,-0.9,
        -1.05,0.9,0.9,
        -1.05,-0.9,0.9,


        -0.9,1.05,0.9,
        0.9,1.05,0.9,
        0.9,1.05,-0.9,

        0.9,1.05,-0.9,
        -0.9,1.05,-0.9,
        -0.9,1.05,0.9,


        -0.9,-1.05,0.9,
        0.9,-1.05,0.9,
        0.9,-1.05,-0.9,

        0.9,-1.05,-0.9,
        -0.9,-1.05,-0.9,
        -0.9,-1.05,0.9,        
    };

    static const GLfloat color_buffer_data [] = {
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1
        186.0/255.0,88.0/255.0,7.0/255.0, // color 1

        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
        11.0/255.0,97.0/255.0,7.0/255.0, // color 1
    };
    printf("hi\n");
    // create3DObject creates and returns a handle to a VAO that can be used later
    platform->cube = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void create_rect_border(block * platform)
{ 
    static const GLfloat vertex_buffer_data [] = {
        -1.0,-1.0,1.0,
        1.0,-1.0,1.0,
        1.0,1.0,1.0,

        1.0,1.0,1.0,
        -1.0,1.0,1.0,
        -1.0,-1.0,1.0,


        1.0,-1.0,1.0,
        1.0,-1.0,-1.0,
        1.0,1.0,-1.0,

        1.0,1.0,-1.0,
        1.0,1.0,1.0,
        1.0,-1.0,1.0,


        1.0,-1.0,-1.0,
        -1.0,-1.0,-1.0,
        -1.0,1.0,-1.0,

        -1.0,1.0,-1.0,
        1.0,1.0,-1.0,
        1.0,-1.0,-1.0,


        -1.0,-1.0,-1.0,
        -1.0,-1.0,1.0,
        -1.0,1.0,1.0,

        -1.0,1.0,1.0,
        -1.0,1.0,-1.0,
        -1.0,-1.0,-1.0,


        -1.0,1.0,1.0,
        1.0,1.0,1.0,
        1.0,1.0,-1.0,

        1.0,1.0,-1.0,
        -1.0,1.0,-1.0,
        -1.0,1.0,1.0,


        -1.0,-1.0,-1.0,
        1.0,-1.0,-1.0,
        1.0,-1.0,1.0,

        1.0,-1.0,1.0,
        -1.0,-1.0,1.0,
        -1.0,-1.0,-1.0,    
    };

    static const GLfloat color_buffer_data [] = {
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
        0.0,0.0,0.0,
    };
    platform->cube_border = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void create_river(block * platform)
{
    static const GLfloat vertex_buffer_data [] = {
        -1.0,-1.0,1.0,
        1.0,-1.0,1.0,
        1.0,0.8,1.0,

        1.0,0.8,1.0,
        -1.0,0.8,1.0,
        -1.0,-1.0,1.0,


        1.0,-1.0,1.0,
        1.0,-1.0,-1.0,
        1.0,0.8,-1.0,

        1.0,0.8,-1.0,
        1.0,0.8,1.0,
        1.0,-1.0,1.0,


        1.0,-1.0,-1.0,
        -1.0,-1.0,-1.0,
        -1.0,0.8,-1.0,

        -1.0,0.8,-1.0,
        1.0,0.8,-1.0,
        1.0,-1.0,-1.0,


        -1.0,-1.0,-1.0,
        -1.0,-1.0,1.0,
        -1.0,0.8,1.0,

        -1.0,0.8,1.0,
        -1.0,0.8,-1.0,
        -1.0,-1.0,-1.0,


        -1.0,1.0,1.0,
        1.0,1.0,1.0,
        1.0,1.0,-1.0,

        1.0,1.0,-1.0,
        -1.0,1.0,-1.0,
        -1.0,1.0,1.0,


        -1.0,-1.0,-1.0,
        1.0,-1.0,-1.0,
        1.0,-1.0,1.0,

        1.0,-1.0,1.0,
        -1.0,-1.0,1.0,
        -1.0,-1.0,-1.0,    
    };

    static const GLfloat color_buffer_data [] = {
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
    };
    platform->cube_border = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void create_block(int j,int k,int type)
{
    block * platform=(block *)malloc(sizeof(block));
    if(type==0)
    {
        createRectangle(platform);
        create_rect_border(platform);
    }
    else if(type==1)
    {
        create_river(platform);
    }
    platform->x=j;
    platform->z=k;
    platform->y=0;
    platform->type=type;
    barr[block_count++]=platform;
}
VAO * player;
float player_pos[3];
void create_player()
{
    static const GLfloat vertex_buffer_data [] = {
        -1.0,-1.0,-1.0,
        1.0,-1.0,-1.0,
        1.0,-1.0,1.0,
        1.0,-1.0,1.0,
        -1.0,-1.0,1.0,
        -1.0,-1.0,-1.0,

        -1.0,-1.0,-1.0,
        1.0,-1.0,-1.0,
        0.0,0.0,0.0,

        1.0,-1.0,-1.0,
        1.0,-1.0,1.0,
        0.0,0.0,0.0,

        -1.0,-1.0,1.0,
        1.0,-1.0,1.0,
        0.0,0.0,0.0,

        -1.0,-1.0,-1.0,
        -1.0,-1.0,1.0,
        0.0,0.0,0.0,
    };

    static const GLfloat color_buffer_data [] = {
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        1.0,0.0,0.0,
        0.0,1.0,0.0,
        0.0,1.0,0.0,
        0.0,1.0,0.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,0.0,1.0,
        0.0,1.0,1.0,
        0.0,1.0,1.0,
        0.0,1.0,1.0,
    };

    player = create3DObject(GL_TRIANGLES, 18, vertex_buffer_data, color_buffer_data, GL_FILL);
    player_pos[0]=2;
    player_pos[1]=2;
    player_pos[2]=2;
}

VAO * boat;
void create_boat()
{
    static const GLfloat vertex_buffer_data [] = {
        -1.0,1.0,1.0,
        3.0,1.0,1.0,
        3.0,1.0,-1.0,

        3.0,1.0,-1.0,
        -1.0,1.0,-1.0,
        -1.0,1.0,1.0,
    };

    static const GLfloat color_buffer_data [] = {
        1.0,1.0,77.0/255.0,
        1.0,1.0,77.0/255.0,
        1.0,1.0,77.0/255.0,
        1.0,1.0,77.0/255.0,
        1.0,1.0,77.0/255.0,
        1.0,1.0,77.0/255.0,
    };

    boat = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw_block(int j,glm::mat4 MVP,glm::mat4 VP)
{
    /* 
       int time = glfwGetTime();
       time = time%5;
       if(time==1||time==3)
       return;
     */
    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    float omega=2;
    float amp=1;
    float shm_time=glfwGetTime();
    block * block=barr[j];
    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translate_rect_border = glm::translate (glm::vec3(2*(block->x),0,2*(block->z)));        // glTranslatef
    glm::mat4 rotate_rect_border = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,1,0)); // rotate about vector (-1,1,1)
    Matrices.model *= (translate_rect_border * rotate_rect_border);
    if(block->shm)
    {
        Matrices.model *= glm::translate(glm::vec3(0,amp*sin((shm_time)*omega),0));
    }
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(block->cube);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateRectangle = translate_rect_border;        // glTranslatef
    glm::mat4 rotateRectangle = rotate_rect_border; // rotate about vector (-1,1,1)
    Matrices.model *= (translateRectangle * rotateRectangle);
    if(block->shm)
    {
        Matrices.model*= glm::translate(glm::vec3(0,amp*sin((shm_time)*omega),0));
    }
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]); 

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(block->cube_border);

    return;
}

float boatx,boatz;
void draw_player(glm::mat4 MVP,glm::mat4 VP)
{
    if(player_pos[1]<-1)
    {
        printf("game over\n");
        glfwTerminate();
        exit(EXIT_SUCCESS);
    }

    Matrices.model = glm::mat4(1.0f);

    /*
       glm::mat4 translate_rect_border = glm::translate (glm::vec3(0,2.3,0));        // glTranslatef
       glm::mat4 rotate_rect_border = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,1,0)); // rotate about vector (-1,1,1)
       Matrices.model *= (translate_rect_border * rotate_rect_border);
     */
    /*
       if(pmov==1)
       player_move = glm::translate(glm::vec3(2,0,0));
       if(pmov==4)
       player_move = glm::translate(glm::vec3(-2,0,0));
       if(pmov==3)
       player_move = glm::translate(glm::vec3(0,0,-2));
       if(pmov==2)
       player_move = glm::translate(glm::vec3(0,0,2));
       else
       player_move = glm::translate(glm::vec3(0,0,0));
     */
    if(player_pos[0]<0||player_pos[0]>=30||player_pos[2]>=20||player_pos[2]<0||(player_pos[0]>=12&&player_pos[0]<14&&player_pos[2]!=boatz))//drowning conditions
    {
        player_pos[1]-=0.5;
    }
    else
    {
        if(pmov==3)
            player_pos[0]+=2;
        if(pmov==2)
            player_pos[0]-=2;
        if(pmov==4)
            player_pos[2]+=2;
        if(pmov==1)
            player_pos[2]-=2;
        pmov=0; 
    }
    Matrices.model *= glm::translate(glm::vec3(player_pos[0],player_pos[1],player_pos[2]));
    player_pos[0]=Matrices.model[3][0];
    player_pos[1]=Matrices.model[3][1];
    player_pos[2]=Matrices.model[3][2];

    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(player);
}
void draw_boat(glm::mat4 VP,glm::mat4 MVP)
{
    int a = glfwGetTime();
    if(a%10==0)
    {
        boatx=12;
        boatz=0;
    }
    else
    {
        
        boatz=a%10 * 2;
    }
        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translate_boat = glm::translate (glm::vec3(boatx,0,boatz));        // glTranslatef
        Matrices.model *= translate_boat;
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DObject(boat);
}

void draw ()
{
    // clear the color and depth in the frame buffer
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram (programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    // glm::vec3 eye ( 10*cos(camera_rotation_angle*M_PI/180.0f), 3, 10*sin(camera_rotation_angle*M_PI/180.0f) );
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 eye (5,5,5);
    glm::vec3 target (5, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 0, -1);

    // Compute Camera matrix (view)
    Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!
    //  Matrices.view = glm::lookAt(glm::vec3(5,1,10) glm::vec3(5,0,0), glm::vec3(0,0,1)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;


    //DRAW BLOCKS HERE...... 
    glm::mat4 MVP;	// MVP = Projection * View * Model
    int j;
    int k;
    for(j=0;j<block_count;j++)
        draw_block(j,MVP,VP);

    //DRAWING BOATS HERE
    draw_boat(VP,MVP);

    //Drawing player here
    draw_player(MVP,VP);


    // Increment angles
    float increments = 1;

    camera_rotation_angle++; // Simulating camera rotation
    //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
       is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
    // Create the models
    int j,k;
    for(j=0;j<15;j++)
        for(k=0;k<10;k++)
            if(j!=6&&j!=7)
                create_block(j,k,0);
            else
                create_block(j,k,1);
    create_boat();
    create_player();
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


    reshapeWindow (window, width, height);

    // Background color of the scene
    glClearColor (102.0/255.0, 224.0/255.0, 255.0/255.0, 0.0f); // R, G, B, A
    glClearDepth (1.0f);

    glEnable (GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
    int width = 600;
    int height = 600;

    GLFWwindow* window = initGLFW(width, height);

    initGL (window, width, height);
    printf("%d\n",block_count);
    double last_update_time = glfwGetTime(), current_time;
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
