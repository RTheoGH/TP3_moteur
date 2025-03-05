#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertices_position_modelspace;
layout(location = 1) in vec2 vertexUV;

uniform mat4 MVP;

uniform sampler2D heightmap;
uniform float heightScale;

// Values that stay constant for the whole mesh.
out vec2 UV;
out float altitude;

void main(){
        // float altitude = sin(vertices_position_modelspace.x) * sin(vertices_position_modelspace.z);
        altitude = texture(heightmap,vertexUV).r * heightScale;
        vec3 relief = vec3(vertices_position_modelspace.x,altitude,vertices_position_modelspace.z);

        gl_Position = MVP * vec4(relief,1);
        // gl_Position = MVP * vec4(vertices_position_modelspace,1);
        UV = vertexUV;

}

