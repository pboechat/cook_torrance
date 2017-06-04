#version 330 core

in vec2 vTexcoords;
in vec3 vNormal;
in vec3 vViewtDir;
in vec3 vLightDir;
in float vLightDistance2;

in vec2 vvetor2;
in vec3 vvetor3;
in vec4 vvetor4;
in float vpontoFlutuante;
in int vinteiro;
in mat4 vmatriz4;
in mat3 vmatriz3;


uniform vec4 color = vec4(1, 1, 1, 1);
uniform vec4 lightColor = vec4(1, 1, 1, 1);
uniform float lightIntensity = 1.0f;

out vec4 outColor;

void main()
{
	int inteiro = vinteiro + 1;
	float pontoFlutuante = vpontoFlutuante + inteiro;
	vec2 vetor2 = vvetor2 * vpontoFlutuante;
	vec3 vetor3 = vvetor3 * vpontoFlutuante;
	vec4 vetor4 = vvetor4 * vpontoFlutuante;
	vec4 tmp1 = vmatriz4 * vetor4;
	vec3 tmp2 = vmatriz3 * vetor3 + vec3(vetor2.x, 0, vetor2.y);

	outColor = dot(vNormal, normalize(vLightDir + vViewtDir)) * lightColor * lightIntensity / vLightDistance2 * color + tmp1.x + tmp2.y;
}
