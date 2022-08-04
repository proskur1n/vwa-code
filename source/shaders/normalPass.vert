#version 330 core

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightView;
uniform mat4 uLightProj;

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

out vec3 vNormal;
out vec3 vWorldPosition;
out vec3 vColor;
out vec4 vShadowCoordinates;

void main() {
	vec4 worldPosition = uModel * aPosition;
	gl_Position = uProj * uView * worldPosition;

	vNormal = mat3(uModel) * aNormal;
	vWorldPosition = vec3(worldPosition);
	vColor = aColor;
	vShadowCoordinates = uLightProj * uLightView * worldPosition;
}
