#include "program.h"
#include "util.h"
#include <vector>

void program::use() const {
    glUseProgram(handle);
}

GLint program::location(char const *name) const {
    return glGetUniformLocation(handle, name);
}

void program::uniform(char const *name, vec3 const v) const {
    glUniform3f(location(name), v[0], v[1], v[2]);
}

void program::uniform(char const *name, mat4x4 const m) const {
    glUniformMatrix4fv(location(name), 1, false, (float*)m);
}

void program::uniform(char const *name, GLint i) const {
    glUniform1i(location(name), i);
}

void program::uniform(char const *name, GLfloat f) const {
    glUniform1f(location(name), f);
}

program::program(char const *vs_path, char const *fs_path) {
    auto vs = compile_shader(GL_VERTEX_SHADER, vs_path);
    auto fs = compile_shader(GL_FRAGMENT_SHADER, fs_path);
    handle = link_program(vs, fs);
    glDetachShader(handle, vs);
    glDetachShader(handle, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

GLuint program::compile_shader(GLenum type, char const *path) {
    auto shader_source = read_text_file(path);
    auto shader = glCreateShader(type);
	GLchar const *raw[] = { shader_source.c_str() };
	glShaderSource(shader, 1, raw, nullptr);
	glCompileShader(shader);

    GLint length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length) {
        std::vector<char> info(length);
        glGetShaderInfoLog(shader, length, nullptr, info.data());
        printf("shader info log (%s):\n%s", path, info.data());
    }

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		die("shader compilation error (%s):\n", path);
	}

	return shader;
}

GLuint program::link_program(GLuint vs, GLuint fs) {
    auto prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

    GLint length;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
    if (length) {
        std::vector<char> info(length);
        glGetProgramInfoLog(prog, length, nullptr, info.data());
        printf("program info log :\n%s", info.data());
    }

	GLint status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status) {
        die("could not link program\n");
	}

	return prog;
}

// TODO too long line
void shadow_renderer::draw(program const &prog, camera const &cam, std::vector<mesh> const &objects) const {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);

    prog.use();
    prog.uniform("u_view", cam.view_matrix);
	prog.uniform("u_proj", cam.proj_matrix);

    glClear(GL_DEPTH_BUFFER_BIT);
    
    for (auto &obj : objects) {
		prog.uniform("u_model", obj.model_matrix);
		obj.draw();
	}
}

void color_renderer::draw(program const &prog, camera const &cam, std::vector<mesh> const &objects) const {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);

	prog.use();
	prog.uniform("u_view", cam.view_matrix);
	prog.uniform("u_proj", cam.proj_matrix);
    prog.uniform("u_light_view", shadow_camera->view_matrix);
    prog.uniform("u_light_proj", shadow_camera->proj_matrix);
	prog.uniform("u_light.ambient", ambient_light);
	prog.uniform("u_light.diffuse", diffuse_light);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadow_fbo->attachments.depth);
    prog.uniform("u_light.depth_buffer", 0);
    prog.uniform("u_light.position", shadow_camera->position);
    prog.uniform("u_light.near_plane", shadow_camera->near_plane);
    prog.uniform("u_light.far_plane", shadow_camera->far_plane);
    prog.uniform("u_light.width", light_width); // TODO
    prog.uniform("u_light.filter_radius", filter_radius); // TODO
    prog.uniform("u_light.algorithm", algorithm);
    prog.uniform("u_light.sample_count", sample_count);
    prog.uniform("u_light.frustum_width", shadow_camera->get_frustum_width());
    prog.uniform("u_light.filter_pattern", filter_pattern);

    glClearStencil(-1);
    glClearColor(ambient_light[0], ambient_light[1], ambient_light[2], 1.0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	for (auto &obj : objects) {
		prog.uniform("u_model", obj.model_matrix);
		prog.uniform("u_color", obj.color);
        glStencilFunc(GL_ALWAYS, obj.get_index(), ~0);
        obj.draw();
	}
}