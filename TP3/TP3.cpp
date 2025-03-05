// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

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
glm::vec3 camera_position   = glm::vec3(0.0f, 1.0f, 7.0f);
glm::vec3 camera_target = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 camera_up    = glm::vec3(0.0f, 1.0f,  0.0f);

glm::vec3 orbital_camera_position = glm::vec3(0.0f, 10.0f, 10.0f);

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

void scaleTerrain(std::vector<glm::vec3> &plan,std::vector<glm::vec2> &uvs,std::vector<unsigned short> &indices_plan,GLuint &vertexbuffer_plan,GLuint &uvbuffer,GLuint &elementbuffer_plan, int sommets) {
    plan.clear();
    uvs.clear();
    indices_plan.clear();

    float taille = 10.0f;
    float m = taille / 2.0f;
    float pas = taille / (float)sommets;

    for (int i = 0; i <= sommets; i++) {
        for (int j = 0; j <= sommets; j++) {
            float x = -m + j * pas;
            float z = -m + i * pas;
            plan.emplace_back(glm::vec3(x,0.0f, z));

            float u = (float)j / (float)(sommets - 1);
            float v = (float)i / (float)(sommets - 1);
            uvs.emplace_back(glm::vec2(u, v));
        }
    }

    for (int i = 0; i < sommets-1; i++) {
        for (int j = 0; j < sommets-1; j++) {
            int topleft = i * (sommets + 1) + j;
            int topright = topleft + 1;
            int bottomleft = (i + 1) * (sommets + 1) + j;
            int bottomright = bottomleft + 1;

            indices_plan.push_back(topleft);
            indices_plan.push_back(bottomleft);
            indices_plan.push_back(topright);

            indices_plan.push_back(topright);
            indices_plan.push_back(bottomleft);
            indices_plan.push_back(bottomright);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer_plan);
    glBufferData(GL_ARRAY_BUFFER, plan.size() * sizeof(glm::vec3), &plan[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer_plan);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_plan.size() * sizeof(unsigned short), &indices_plan[0] , GL_STATIC_DRAW);
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

    { // Textures
        glActiveTexture(GL_TEXTURE0);
        GLuint grassTexture = loadTexture("grass.png");
        glBindTexture(GL_TEXTURE_2D,grassTexture);
        GLuint grassTextureID = glGetUniformLocation(programID,"GRASS");
        glUniform1i(grassTextureID, 0);
        
        glActiveTexture(GL_TEXTURE1);
        GLuint rockTexture = loadTexture("rock.png");
        glBindTexture(GL_TEXTURE_2D,rockTexture);
        GLuint rockTextureID = glGetUniformLocation(programID,"ROCK");
        glUniform1i(rockTextureID, 1);

        glActiveTexture(GL_TEXTURE2);
        GLuint snowTexture = loadTexture("snowrocks.png");
        glBindTexture(GL_TEXTURE_2D,snowTexture);
        GLuint snowTextureID = glGetUniformLocation(programID,"SNOW");
        glUniform1i(snowTextureID, 2);

        glActiveTexture(GL_TEXTURE3);
        GLuint heightmapTexture = loadTexture("heightmap-1024x1024.png");
        glBindTexture(GL_TEXTURE_2D,heightmapTexture);
        GLuint heightmapID = glGetUniformLocation(programID,"heightmap");
        glUniform1i(heightmapID, 3);

        GLuint heightScaleID = glGetUniformLocation(programID,"heightScale");
        glUniform1f(heightScaleID,1.0f);
    }
    
    std::vector<glm::vec3> plan;
    std::vector<glm::vec2> uvs;
    std::vector<unsigned short> indices_plan;

    { // Initialisation du plan
        float taille = 10.0f;
        float m = taille / 2.0f;
        float pas = taille / (float)sommets;
    
        for(int i = 0; i <= sommets; i++){
            for(int j = 0; j <= sommets; j++){
                float x = -m + j * pas;
                float z = -m + i * pas;
    
                plan.emplace_back(glm::vec3(x,0.0f,z));
    
                float u = (float)j / (float)(sommets-1);
                float v = (float)i / (float)(sommets-1);
    
                uvs.emplace_back(glm::vec2(u,v));
            }
        }
    
        for(int i = 0; i < sommets-1; i++){
            for(int j = 0; j < sommets-1; j++){
                int topleft = i * (sommets+1) + j;
                int topright = topleft + 1;
                int bottomleft = (i+1) * (sommets+1) + j;
                int bottomright = bottomleft + 1;
    
                indices_plan.push_back(topleft);
                indices_plan.push_back(bottomleft);
                indices_plan.push_back(topright);
    
                indices_plan.push_back(topright);
                indices_plan.push_back(bottomleft);
                indices_plan.push_back(bottomright);
            }
        }
    }
    
    GLuint vertexbuffer_plan,uvbuffer,elementbuffer_plan;
    
    glGenBuffers(1,&vertexbuffer_plan);
    glBindBuffer(GL_ARRAY_BUFFER,vertexbuffer_plan);
    glBufferData(GL_ARRAY_BUFFER,plan.size() * sizeof(glm::vec3),&plan[0],GL_STATIC_DRAW);

    glGenBuffers(1,&uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER,uvbuffer);
    glBufferData(GL_ARRAY_BUFFER,uvs.size() * sizeof(glm::vec2),&uvs[0],GL_STATIC_DRAW);

    glGenBuffers(1,&elementbuffer_plan);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,elementbuffer_plan);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices_plan.size() * sizeof(unsigned short),&indices_plan[0],GL_STATIC_DRAW);

    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    do{
        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // Mise à jour de la résolution
        // --------------------
        if(scaleT){
            scaleTerrain(plan,uvs,indices_plan,vertexbuffer_plan,uvbuffer,elementbuffer_plan,sommets);
            scaleT = false;
        }

        // Rotation mode orbital
        // --------------------
        if(orbital){
            angle += rotation_speed * deltaTime ;
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);


        /****************************************/
        // Model matrix : an identity matrix (model will be at the origin) then change
        glm::mat4 ModelMatrix;
        if(orbital){
            ModelMatrix = glm::rotate(glm::mat4(1.0f),angle,glm::vec3(0.0f,1.0f,0.0f));
        }else{
            ModelMatrix = glm::mat4(1.0f);
        }
        // View matrix : camera/view transformation lookat() utiliser camera_position camera_target camera_up
        glm::mat4 ViewMatrix = glm::lookAt(camera_position,camera_position+camera_target,camera_up);
        // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
        glm::mat4 ProjectionMatrix = glm::perspective(glm::radians(angle_perspective),(float)SCR_WIDTH / (float)SCR_HEIGHT,0.1f,100.0f);
        // Send our transformation to the currently bound shader,
        // in the "Model View Projection" to the shader uniforms
        MVP = ProjectionMatrix*ViewMatrix*ModelMatrix;
        glUniformMatrix4fv(MatrixID,1,GL_FALSE,&MVP[0][0]);
        /****************************************/

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,vertexbuffer_plan);
        glVertexAttribPointer(0,3,GL_FLOAT, GL_FALSE,0,(void*)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER,uvbuffer);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,elementbuffer_plan);
        glDrawElements(GL_TRIANGLES,indices_plan.size(),GL_UNSIGNED_SHORT,(void*)0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    // Cleanup VBO and shader
    glDeleteBuffers(1,&vertexbuffer_plan);
    glDeleteBuffers(1,&elementbuffer_plan);
    glDeleteBuffers(1,&uvbuffer);
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

    if(!orbital){
        float cameraSpeed = 2.5 * deltaTime;
        if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            camera_position += cameraSpeed * camera_target;
        if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            camera_position -= cameraSpeed * camera_target;

        if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS)
            camera_position -= glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS)
            camera_position += cameraSpeed * camera_up;
        if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS)
            camera_position += glm::normalize(glm::cross(camera_target,camera_up))*cameraSpeed;
        if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS)
            camera_position -= cameraSpeed * camera_up;
    }else{
        camera_position = orbital_camera_position;
        camera_target = glm::normalize(camera_target - orbital_camera_position);
    }

    /****************************************/
    // Augmenter ou diminuer la résolution du plan

    if(glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS && sommets <= MAX_SOMMETS){
        sommets += 2;
        scaleT = true;
    }
    if(glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS && sommets > MIN_SOMMETS){
        sommets -= 2;
        scaleT = true;
    }

    /****************************************/
    // Angle de perspective

    if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        angle_perspective += 1.0f;
    if(glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        angle_perspective -= 1.0f;

    /****************************************/
    // Vitesse de rotation (mode orbital)

    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        rotation_speed += 0.1f;
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        rotation_speed -= 0.1f;

    /****************************************/
    // Position de la caméra selon le mode (manuel ou orbital)

    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
        orbital = !orbital;
        if(orbital){
            camera_position = orbital_camera_position;
            camera_target = glm::normalize(camera_target - orbital_camera_position);
        }
        if(!orbital){
            camera_position   = glm::vec3(0.0f,1.0f,7.0f);
            camera_target = glm::vec3(0.0f,0.0f,-1.0f);
        }
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
