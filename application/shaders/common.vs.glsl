#version 330 core

in vec3 position; 
in vec3 normal;
in vec2 texcoords;

out vec2 vTexcoords; 
out vec3 vNormal;
out vec3 vViewDir;
out vec3 vLightDir;
out float vLightDistance2;

uniform mat4 model; 
uniform mat4 view; 
uniform mat4 projection; 
uniform vec3 eyePosition;
uniform vec3 lightPosition;

void main()
{
    vTexcoords = texcoords;
	vNormal = (model * vec4(normal, 0)).xyz;
	vec4 worldPosition = model * vec4(position, 1.0f);
	vViewDir = normalize(eyePosition - worldPosition.xyz);
	vLightDir = lightPosition - worldPosition.xyz;
	vLightDistance2 = length(vLightDir);
	vLightDir /= vLightDistance2;
	vLightDistance2 *= vLightDistance2;
    gl_Position = projection * view * worldPosition;
}
