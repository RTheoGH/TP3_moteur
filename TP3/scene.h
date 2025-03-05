#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Transform{
public:
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;

    Transform() : position(0.0f),rotation(0.0f),scale(1.0f){}
    // glm::vec3 getMatrice() const {
    //     glm::mat3 mat = glm::mat3(1.0f);
    //     mat = glm::rotate(glm::rotate(glm::mat3(mat)),position.x,glm::vec3(1,0,0));
    //     mat = glm::rotate(glm::rotate(glm::mat3(mat)),position.y,glm::vec3(0,1,0));
    //     mat = glm::rotate(glm::rotate(glm::mat3(mat)),position.z,glm::vec3(0,0,1));
    //     mat = glm::scale(glm::scale(glm::mat3(mat)),glm::vec3(scale,1.0f));
    //     return mat;
    // }
    glm::mat4 getMatrice() const {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat,position);
        mat = glm::rotate(mat, position.x, glm::vec3(1, 0, 0));
        mat = glm::rotate(mat, position.y, glm::vec3(0, 1, 0));
        mat = glm::rotate(mat, position.z, glm::vec3(0, 0, 1));
        mat = glm::scale(mat, glm::vec3(scale));
        return mat;
    }
};

class SNode{
public:
    Transform transform;
    std::vector<std::shared_ptr<SNode>> feuilles;
    GLuint vao,vbo,ibo;
    size_t indexCPT;

    SNode() {buffers();}

    // void buffers(){
    //     // glGenVertexArrays(1,&vao);
        
        

    //     // glBindVertexArray(vao);
        

    //     // std::vector<glm::vec3> vertices;
    //     // std::vector<unsigned short> indices;
    //     // std::vector<std::vector<unsigned short>> triangles;

    //     std::vector<glm::vec3> vertices = {
    //         glm::vec3(0.0f,  0.5f, 0.0f),  // haut
    //         glm::vec3(0.5f, -0.5f, 0.0f),  // bas droite
    //         glm::vec3(-0.5f, -0.5f, 0.0f)  // bas gauche
    //     };
    //     std::vector<unsigned short> indices = { 0, 1, 2 };

    //     // loadOFF("suzanne.off",vertices,indices,triangles);
    //     std::cout << "Vertices loaded: " << vertices.size() << std::endl;
    //     std::cout << "Indices loaded: " << indices.size() << std::endl;
    //     glGenBuffers(1,&vbo);
    //     if (vbo == 0) {
    //         std::cout << "Erreur lors de la création du VBO!" << std::endl;
    //     } else {
    //         std::cout << "VBO créé avec succès!" << std::endl;
    //     }
    //     glBindBuffer(GL_ARRAY_BUFFER,vbo);
    //     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
        
    //     glGenBuffers(1,&ibo);
    //     if (ibo == 0) {
    //         std::cout << "Erreur lors de la création de l'IBO!" << std::endl;
    //     } else {
    //         std::cout << "IBO créé avec succès!" << std::endl;
    //     }
    //     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    //     glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
        
    //     indexCPT = indices.size();

    //     std::cout << "Buffers créés et données envoyées!" << std::endl;

    //     glBindVertexArray(0);
    // }

    void buffers() {
        std::cout << "Création des buffers..." << std::endl;

        std::vector<glm::vec3> vertices = {
            glm::vec3(0.0f,  0.5f, 0.0f),  // haut
            glm::vec3(0.5f, -0.5f, 0.0f),  // bas droite
            glm::vec3(-0.5f, -0.5f, 0.0f)  // bas gauche
        };
        std::vector<unsigned short> indices = { 0, 1, 2 };

        std::cout << "Nombre de sommets : " << vertices.size() << std::endl;
        std::cout << "Nombre d'indices : " << indices.size() << std::endl;

        glGenBuffers(1, &vbo);
        if (vbo == 0) {
            std::cout << "Erreur lors de la création du VBO!" << std::endl;
        } else {
            std::cout << "VBO créé avec succès!" << std::endl;
        }
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &ibo);
        if (ibo == 0) {
            std::cout << "Erreur lors de la création de l'IBO!" << std::endl;
        } else {
            std::cout << "IBO créé avec succès!" << std::endl;
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

        indexCPT = indices.size();

        std::cout << "Buffers créés et données envoyées!" << std::endl;

        glBindVertexArray(0);
    }


    void addFeuille(std::shared_ptr<SNode> feuille){
        feuilles.push_back(feuille);
    }

    virtual void update(float deltaTime){
        for(auto &feuille: feuilles){
            feuille->update(deltaTime);
        }
    }

    virtual void draw(GLuint shaderProgram,const glm::mat4 &brancheMatrix,const glm::vec3 &branchePos){
        glm::mat4 modelMatrix = brancheMatrix*transform.getMatrice();
        glm::vec3 worldPos = branchePos + transform.position;

        std::cout << "Dessin du noeud..." << std::endl;

        glUseProgram(shaderProgram);
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Erreur lors de l'utilisation du shader!" << std::endl;
        }

        std::cout << "Liaison du VAO..." << std::endl;
        glBindVertexArray(vao);
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Erreur lors de la liaison du VAO!" << std::endl;
        }

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Erreur lors de la liaison du VBO!" << std::endl;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ibo);
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Erreur lors de la liaison de l'IBO!" << std::endl;
        }

        std::cout << "Dessiner les éléments..." << std::endl;
        glDrawElements(
            GL_TRIANGLES,
            indexCPT,
            GL_UNSIGNED_SHORT,
            (void*)0
        );
        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Erreur lors du dessin des éléments!" << std::endl;
        }

        glDisableVertexAttribArray(0);
        glBindVertexArray(0);

        std::cout << "Fin du dessin du noeud!" << std::endl;

        for(auto &feuille: feuilles){
            feuille->draw(shaderProgram,modelMatrix,worldPos);
        }
    }
};

class Scene{
public:
    std::shared_ptr<SNode> racine;

    Scene(){racine = std::make_shared<SNode>();}

    void update(float deltaTime){
        racine->update(deltaTime);
    }

    void draw(GLuint shaderProgram){
        racine->draw(shaderProgram,glm::mat4(1.0f),glm::vec3(0.0f));
    }
};