#include "mesh.h"
#include "util.h"
#include <one-stl/one-stl.h>

mesh mesh::load_stl(char const *path, vec3 position) {
	auto stl = read_binary_file(path);

	size_t buf_size;
	if (one_stl_buf_size(&buf_size, stl.data(), stl.size(), ONE_STL_VVV_NNN)) {
		die("invalid file: %s\n", path);
	}

	std::vector<float> buf(buf_size / sizeof(float));
	auto trig_count = one_stl_parse(buf.data(), stl.data(), ONE_STL_VVV_NNN);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float), buf.data(), GL_STATIC_DRAW);
	GLintptr position_offset = 0;
	GLintptr normal_offset = trig_count * 9 * sizeof(float);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, (GLvoid*)position_offset);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, (GLvoid*)normal_offset);

	// TODO this function is messy

	// TODO should it really be a static variable (maybe make it a class static ?)
	static GLint next_index = 0;

	mesh m;
	m.vao = vao;
	m.vertex_count = trig_count * 3;
	m.index = next_index++;
	mat4x4_translate(m.model_matrix, position[0], position[1], position[2]);
	// TODO
	// vec3_dup(m.position, position);
	// printf("index: %d", m.index);
	return m;
}