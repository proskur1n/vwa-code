#pragma once

#include <glad/glad.h>
#include <linmath/linmath.h>
#include <vector>
#include "camera.h"
#include "framebuffer.h"
#include "mesh.h"

class program {
	GLuint handle {0};

	GLint location(char const *name) const;

	static GLuint compile_shader(GLenum type, char const *path);
	static GLuint link_program(GLuint vs, GLuint fs);
public:
	program() {};
	program(char const *vs_path, char const *fs_path);

	void use() const;
	void uniform(char const *name, vec3 const) const;
	void uniform(char const *name, mat4x4 const) const;
	void uniform(char const *name, GLint) const;
	void uniform(char const *name, GLfloat) const;
};

struct shadow_renderer {
	void draw(program const &, camera const &, std::vector<mesh> const &) const;
};

// TODO this looks messy as hell
struct color_renderer {
	vec3 ambient_light {0.43f, 0.44f, 0.53f};
	vec3 diffuse_light {0.95f, 0.92f, 0.84f};

	// TODO default values ?
	camera const *shadow_camera {nullptr};
	// TODO should it be fbo or only handle ?
	framebuffer const *shadow_fbo {nullptr};

	float filter_radius {0.006f};
	float light_width {0.08f};

	enum : int {
		HARD_SHADOWS = 0,
		PCF,
		PCSS
	} algorithm {PCF};

	int sample_count {8};
	enum : int {
		REGULAR_GRID = 0,
		POISSON_DISK,
		ROTATED_POISSON,
	} filter_pattern {REGULAR_GRID};

	void draw(program const &, camera const &, std::vector<mesh> const &) const;
};