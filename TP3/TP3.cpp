// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

#include "scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
glm::vec3 camera_position   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

// glm::vec3 orbital_camera_position = glm::vec3(0.0f, 10.0f, 10.0f);

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

int sommets = 16;
const int MIN_SOMMETS = 4;
const int MAX_SOMMETS = 248;
bool scaleT = false;

// rotation
float angle = 0.;
float angle_perspective = 45.;
float zoom = 1.;
bool orbital = false;
float rotation_speed = 0.5f;

/*******************************************************************************/

// Chargeur de texture
GLuint loadTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Paramètres de texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

int main( void ){
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "TP1 - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    //glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders( "vertex_shader.glsl", "fragment_shader.glsl" );

    /****************************************/
    // Get a handle for our "Model View Projection" matrices uniforms
    GLuint MatrixID = glGetUniformLocation(programID,"MVP");
    glm::mat4 MVP;

    /****************************************/

    // Get a handle for our "LightPosition" uniform
    glUseProgram(programID);
    GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

    // std::shared_ptr<SNode> racine = std::make_shared<SNode>();
    auto soleil = std::make_shared<SNode>();
    auto terre = std::make_shared<SNode>();
    auto lune = std::make_shared<SNode>();

    terre->transform.position = glm::vec3(1.0f, 0.0f, 0.0f);
    terre->transform.rotation.x = glm::radians(23.0f);
    lune->transform.position = glm::vec3(1.5f, 0.0f, 0.0f);

    terre->addFeuille(lune);
    soleil->addFeuille(terre);
    // racine->addFeuille(soleil);

    float time = 0.0f;
    float terreRevSpeed = 0.5f;
    float luneRevSpeed = 1.0f;
    float terreRotaSpeed = 0.1f;
    float luneRotaSpeed = 0.2f;

    glm::mat4 initialMatrix = glm::mat4(1.0f);
    glm::vec3 initialPos = glm::vec3(0.0f,0.0f,0.0f);

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do{
        time += deltaTime;
        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        terre->transform.rotation.y = time * terreRevSpeed;
        terre->transform.rotation.z += terreRotaSpeed * deltaTime;

        lune->transform.rotation.y = time * luneRevSpeed;
        lune->transform.rotation.z += luneRotaSpeed * deltaTime;

        soleil->update(deltaTime);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        soleil->draw(programID,initialMatrix,initialPos);

        // Use our shader
        glUseProgram(programID);

        /****************************************/
        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        glm::mat4 ViewMatrix = glm::lookAt(
            camera_position,
            camera_position+camera_target,
            camera_up
        );
        glm::mat4 ProjectionMatrix = glm::perspective(
            glm::radians(angle_perspective),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            100.0f
        );

        MVP = ProjectionMatrix*ViewMatrix*ModelMatrix;
        glUniformMatrix4fv(MatrixID,1,GL_FALSE,&MVP[0][0]);
        /****************************************/

        

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    // glDeleteBuffers(1,&vertexbuffer_plan);
    // glDeleteBuffers(1,&elementbuffer_plan);
    // glDeleteBuffers(1,&uvbuffer);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1,&VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 2.5 * deltaTime;
    if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS){
        camera_position += cameraSpeed * camera_target;
        std::cout << "forward" << std::endl;
    }
    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS){
        camera_position -= cameraSpeed * camera_target;
        std::cout << "backward" << std::endl;
    }
    if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS){
        camera_position -= glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        std::cout << "gauche" << std::endl;
    }
    if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS){
        camera_position += cameraSpeed * camera_up;
        std::cout << "haut" << std::endl;
    }
    if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS){
        camera_position += glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        std::cout << "droite" << std::endl;
    }
    if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS){
        camera_position -= cameraSpeed * camera_up;
        std::cout << "bas" << std::endl;
    }
    /****************************************/
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
