#version 330 core

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

void main() {
	gl_Position = uProj * uView * uModel * aPosition;
}
