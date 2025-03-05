#version 330 core

in vec2 UV;
// Ouput data
out vec3 color;

uniform sampler2D GRASS;
uniform sampler2D ROCK;
uniform sampler2D SNOW;

in float altitude;

void main(){
        vec3 grassColor = texture(GRASS,UV).rgb;
        vec3 rockColor = texture(ROCK,UV).rgb;
        vec3 snowColor = texture(SNOW,UV).rgb;

        vec3 blendedColor = mix(grassColor,rockColor,smoothstep(0.15,0.25,altitude));
        blendedColor = mix(blendedColor,snowColor,smoothstep(0.3,0.5,altitude));

        color = blendedColor;

        // DEBUG
        // color =vec3(0.4,0.2,0.2);
        // color =vec3(UV,1.0);
        // color = texture(GRASS,UV).rgb;
        // color = texture(ROCK,UV).rgb;
        // color = texture(SNOW,UV).rgb;
}