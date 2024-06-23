#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/CAMERA.H> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Daniel Finley"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 1200;
    const int WINDOW_HEIGHT = 1000;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Shader program
    GLuint gClayProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 3.0f, 20.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color  ***Need to figure out how to allow for new color input**
    //glm::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
    glm::vec3 gObjectColor(0.5f, 0.5f, 1.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    // Light position and scale
    glm::vec3 gLightPosition(4.0f, 8.0f, 12.0f);
    glm::vec3 gLightScale(0.5f);

    // Lamp animation
    bool gIsLampOrbiting = true;

    //Attempting to add texture to the scene ********************************
    //GLuint gTextureBlueDesk;
    //GLuint gTextureCheckerboard;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* clayVertexShaderSource = GLSL(440,

layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
}
);


/* Fragment Shader Source Code*/
const GLchar* clayFragmentShaderSource = GLSL(440,
    in vec4 vertexColor; // Variable to hold incoming color data from vertex shader

in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.1f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 1.0f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * objectColor;

    fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(clayVertexShaderSource, clayFragmentShaderSource, gClayProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Load Textures ****************************

    /* const char* texFilename = "../../resources/textures/blueDesk.png";
    if (!UCreateTexture(texFilename, gTextureBlueDesk)) {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "../../resources/textures/checkerboard.png";
    if (!UCreateTexture(texFilename, gTextureCheckerboard))  {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    } */

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
// --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture******************************
    //UDestroyTexture(gTextureBlueDesk);
   //UDestroyTexture(gCheckerboard);

    // Release shader program
    UDestroyShaderProgram(gClayProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);

    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {

    gCamera.ProcessMouseScroll(yoffset);

}


// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// Functioned called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting)
    {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;
    }

    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gClayProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gClayProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gClayProgramId, "view");
    GLint projLoc = glGetUniformLocation(gClayProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cube color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gClayProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gClayProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gClayProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gClayProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    /*
    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureBlueDesk);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureCheckerboard);
    */

    // Draws the triangles
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

     // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat verts[] = {
        //Vertex Positions    // Colors (r,g,b,a)
        //Vertex Positions      //Colors (r,g,b,a)
        
        // vertex location       vertex color        vertex texture
         
            //Front Square BLUE
       -2.5f, -0.5f, 0.5f,   0.0f, 0.0f, 1.0f, 1.0f, //1.0f, 1.0f,  //0 BLUE Front Bottom Left
       -2.0f, -0.5f, 0.5f,   0.0f, 0.0f, 1.0f, 1.0f, //1.0f, 1.0f, //1 BLUE Front Bottom Right
       -2.5f, 0.0f, 0.5f,    0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f, //2 BLUE Front Top Left
       -2.0f, 0.0f, 0.5f,    0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f, //3 BLUE Front Top Right

            //Back Square GREEN
       -2.0f, 0.0f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f,// 1.0f, 1.0f,// 4 GREEN Back Top Right
       -2.5f, 0.0f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f,// 1.0f, 1.0f,//5 GREEN Back Top Left
       -2.0f, -0.5f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f, //1.0f, 1.0f,//6 GREEN Back Bottom Right
       -2.5f, -0.5f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f, //1.0f, 1.0f,//7 GREEN Back Bottom Left

            //Left Rectangle YELLOW
       -2.5f, -0.5f, 0.5f,  1.0f, 1.0f, 0.0f, 1.0f, //1.0f, 1.0f, //8 YELLOW Front Bottom Left
       -2.5f, 0.0f, 0.5f,   1.0f, 1.0f, 0.0f, 1.0f, //1.0f, 1.0f,//9 YELLOW Front Top Left
       -2.5f, 0.0f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f, //1.0f, 1.0f,//10 YELLOW Back Top Left
       -2.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f, //1.0f, 1.0f,//11 YELLOW Back Bottom Left

           //Bottom Rectangle BLUE -> GREEN
       -2.5f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,//12 Blue Front Bottom Left
       -2.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,//13 Blue Front Bottom Right
       -2.5f, -0.5f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f,// 1.0f, 1.0f,//14 Green Back Bottom Left
       -2.0f, -0.5f, -0.5f,  0.0f, 0.5f, 0.0f, 1.0f, //1.0f, 1.0f,//15 Green Back Bottom Right

            //Top Rectangle YELLOW -> RED
       -2.5f, 0.0f, 0.5f,   1.0f, 1.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //16 YELLOW Front Top Left
       -2.0f, 0.0f, 0.5f,   1.0f, 0.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //17 RED Front Top Right
       -2.0f, 0.0f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,//1.0f, 1.0f,    //18 RED Back Top Right
       -2.5f, 0.0f, -0.5f,  1.0f, 1.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //19 YELLOW Back Top Left

            //Right Rectangle RED
       -2.0f, -0.5f, 0.5f,  1.0f, 0.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //20 RED Front Bottom Right
       -2.0f, 0.0f, 0.5f,   1.0f, 0.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //21 RED Front Top Right
       -2.0f, 0.0f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //22 RED Back Top Right
       -2.0f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f,// 1.0f, 1.0f,    //23 RED Back Bottom Right

            //Plane the box is setting on
        -5.0f, -0.51f, -5.0f,   0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,   //24 Front Left - left side
         5.0f, -0.51f, -5.0f,   0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,   //25 Back Left - left side
         5.0f, -0.51f,  5.0f,   0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,   //26 Back Right - left side
         5.0f, -0.51f, 5.0f,    0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,    //27 Back Right - right side
        -5.0f, -0.51f, 5.0f,    0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,    //28 Front Right - right side
        -5.0f, -0.51f, -5.0f,   0.5f, 0.5f, 1.0f, 1.0f,// 1.0f, 1.0f,   //29 Front Left - right side

        //To make sure the box does not show through the bottom of the table
        //Move the table down .01 further than the box

        //pyramid
        2.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,   //30
        2.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //31
        4.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //32
        4.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //33
        4.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //34
        2.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //35

        2.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //36
        2.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //37
        3.0f, 1.0f, 3.0f,   0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //38

        2.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //39
        4.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //40
        3.0f, 1.0f, 3.0f,   0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //41

        4.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //42
        2.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //43
        3.0f, 1.0f, 3.0f,   0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //44

        4.0f, -0.5f, 4.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //45
        4.0f, -0.5f, 2.0f,  0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //46
        3.0f, 1.0f, 3.0f,   0.0f, 0.0f,     1.0f, 1.0f,// 1.0f, 1.0f,    //47

        //new shapes added to scene 48 -> 

        3.5f,  2.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //48
        3.5f, -0.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //49
       -0.5f, -0.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //50

        3.5f,  2.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //51       
       -0.5f,  2.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //52
       -0.5f, -0.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //53

        3.5f,  1.0f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //54
        3.5f, -0.5f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //55
       -0.5f, -0.5f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //56

        3.5f,  1.0f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //57
       -0.5f,  1.0f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //58
       -0.5f, -0.5f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //59

        3.5f,  2.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //60
        3.5f,  1.0f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //61
        3.5f, -0.5f, -2.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //62

        3.5f,  2.5f, -4.0f,  0.0f, 0.0f,    1.0f, 1.0f,// 1.0f, 1.0f,   //63
        3.5f, -0.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //64
        3.5f, -0.5f, -2.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //65

       -0.5f,  2.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //66
       -0.5f,  1.0f, -2.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //67
       -0.5f, -0.5f, -2.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //68

       -0.5f,  2.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //69
       -0.5f, -0.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //70
       -0.5f, -0.5f, -2.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //71

        4.0f,  2.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //72
        4.0f, 0.85f, -1.8f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //73
       -1.0f, 0.85f, -1.8f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //74

        4.0f,  2.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //75
       -1.0f, 0.85f, -1.8f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //76
       -1.0f,  2.5f, -4.0f,  0.0f, 0.0f, 1.0f, 1.0f,// 1.0f, 1.0f,   //77

    };

    // Index data to share position data
    GLushort indices[] = {
       0, 1, 2,  // Triangle 1 Front Side Bottom
       1, 2, 3,  // Triangle 2 Front Side Top
       4, 5, 6,  // Triangle 3 Back Side Top
       5, 6, 7,  // Triangle 4 Back Side Bottom
       8, 9, 10,  // Triangle 5 Left Side Top 
       8, 10, 11,  // Triangle 6 Left Side Bottom
       12, 13, 14,  // Triangle 7 Bottom Side Left 
       13, 14, 15,  // Triangle 8 Bottom Side Right 
       16, 17, 18,  // Triangle 9 Top Side Right 
       16, 18, 19,  // Triangle 10 Top Side Left 
       20, 21, 22,  // Triangle 11 Right Side Top
       20, 22, 23,  // Triangle 12 Right Side Bottom

       24, 25, 26, //Plane Triangle 1
       27, 28, 29, //Plane Triangle 2

       30, 31, 32,
       33, 34, 35,
       36, 37, 38,
       39, 40, 41,

       42, 43, 44,
       45, 46, 47,

       48, 49, 50,
       51, 52, 53,

       54, 55, 56,
       57, 58, 59,

       60, 61, 62,
       63, 64, 65,

       66, 67, 68,
       69, 70, 71,

       72, 73, 74,
       75, 76, 77,


    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    //const Gluint floatsPerUV = 2;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    /*
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* (gloatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);*/
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

