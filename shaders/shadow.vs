#version 330 core

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

layout(location = 0) in vec4 a_pos;

void main() {
	gl_Position = u_proj * u_view * u_model * a_pos;
}