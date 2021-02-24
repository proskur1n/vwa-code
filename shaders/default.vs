#version 330 core

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat4x4 u_light_view;
uniform mat4x4 u_light_proj;

layout(location = 0) in vec4 a_pos;
layout(location = 1) in vec3 a_normal;

out vec3 v_normal;
out vec3 v_world_pos;
out vec4 v_shadow_coords;

void main() {
	vec4 world_pos = u_model * a_pos;
	gl_Position = u_proj * u_view * world_pos;

	v_normal = mat3(u_model) * a_normal;
	v_world_pos = vec3(world_pos); // TODO if switch to directional light then this can be removed
	v_shadow_coords = u_light_proj * u_light_view * world_pos;
}