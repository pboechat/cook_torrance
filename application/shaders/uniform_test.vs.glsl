#version 330 core

in vec3 position; 
in vec3 normal;
in vec2 texcoords;

out vec2 vTexcoords; 
out vec3 vNormal;
out vec3 vViewDir;
out vec3 vLightDir;
out float vLightDistance2;

out vec2 vvetor2;
out vec3 vvetor3;
out vec4 vvetor4;
out float vpontoFlutuante;
out int vinteiro;
out mat4 vmatriz4;
out mat3 vmatriz3;

uniform mat4 model; 
uniform mat4 view; 
uniform mat4 projection; 
uniform vec3 eye;
uniform vec3 lightPosition;
uniform vec2 vetor2;
uniform vec3 vetor3;
uniform vec4 vetor4;
uniform float pontoFlutuante;
uniform int inteiro;
uniform mat4 matriz4;
uniform mat3 matriz3;

void main()
{
	vvetor2 = vetor2;
	vvetor3 = vetor3;
	vvetor4 = vetor4;
	vpontoFlutuante = pontoFlutuante;
	vinteiro = inteiro;
	vmatriz4 = matriz4;
	vmatriz3 = matriz3;


    vTexcoords = texcoords;
	vNormal = (model * vec4(normal, 0)).xyz;
	vec4 wPosition = model * vec4(position, 1.0f);
	vViewDir = normalize(eye - wPosition.xyz);
	vLightDir = lightPosition - wPosition.xyz;
	vLightDistance2 = length(vLightDir);
	vLightDir /= vLightDistance2;
	vLightDistance2 *= vLightDistance2;
    gl_Position = projection * view * wPosition;
}
