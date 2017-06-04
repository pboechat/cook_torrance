#version 330 core

in vec2 vTexcoords;
in vec3 vNormal;
in vec3 vViewDir;
in vec3 vLightDir;
in float vLightDistance2;

uniform vec3 specularColor = vec3(1, 1, 1);
uniform float specularity = 0;
uniform vec3 lightDiffuseColor = vec3(1, 1, 1);
uniform float lightDiffusePower = 1.0f;
uniform vec3 lightSpecularColor = vec3(1, 1, 1);
uniform float lightSpecularPower = 1.0f;
uniform sampler2D tex0;

out vec3 outColor;

void main()
{
	float NdotH = max(0, dot(vNormal, normalize(vLightDir + vViewDir)));
	vec3 diffuseColor = texture(tex0, vTexcoords).rgb;
	outColor = NdotH * diffuseColor * 
					lightDiffuseColor * lightDiffusePower / vLightDistance2 +
				pow(NdotH, specularity) * specularColor *
					lightSpecularColor * lightSpecularPower / vLightDistance2;
}
