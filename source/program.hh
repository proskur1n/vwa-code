#pragma once

#include <glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <string_view>
#include "util.hh"

class Program {
	GLuint handle {0};
	mutable std::unordered_map<std::string_view, GLint> cache;
public:
	Program(char const *vertexShaderPath, char const *fragmentShaderPath)
	{
		GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderPath);
		GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderPath);
		handle = linkProgram(vs, fs);
		glDetachShader(handle, vs);
		glDetachShader(handle, fs);
		glDeleteShader(vs);
		glDeleteShader(fs);
	}

	Program(Program const &) = delete;

	void operator=(Program const &) = delete;

	~Program()
	{
		glDeleteProgram(handle);
	}

	void use() const
	{
		glUseProgram(handle);
	}

	// Setters for uniform variables. Do not forget to call use()
	// before using these functions.

	void set(char const *name, glm::vec3 const &v) const
	{
		glUniform3f(getUniformLocation(name), v.x, v.y, v.z);
	}

	void set(char const *name, glm::mat4 const &m) const
	{
		glUniformMatrix4fv(getUniformLocation(name), 1, false, glm::value_ptr(m));
	}

	void set(char const *name, int i) const
	{
		glUniform1i(getUniformLocation(name), i);
	}

	void set(char const *name, float f) const
	{
		glUniform1f(getUniformLocation(name), f);
	}

	void setTexture(char const *name, int unit, GLuint texture) const
	{
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(getUniformLocation(name), unit);
	}

private:
	GLint getUniformLocation(char const *name) const
	{
		// Note that, at least on my hardware, this is actually slower than simply
		// calling glGetUniformLocation. I only do this to keep track of which
		// warnings have already been printed.
		std::string_view sv = name;
		if (cache.count(sv) == 0) {
			GLint loc = glGetUniformLocation(handle, name);
			if (loc < 0) {
				std::cout << "Uniform variable does not exist: " << name << '\n';
			}
			cache[sv] = loc;
		}
		return cache[sv];
	}

	static GLuint compileShader(GLenum type, char const *path)
	{
		std::string source = util::readFileAsString(path);
		char const *raw[] = {source.c_str()};

		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, raw, nullptr);
		glCompileShader(shader);

		int length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			std::string info(length - 1, '\0');
			glGetShaderInfoLog(shader, length, nullptr, info.data());
			std::cout << info << '\n';
		}

		int status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (!status) {
			util::fatalError("Could not compile shader: ", path);
		}

		return shader;
	}

	static GLuint linkProgram(GLuint vs, GLuint fs)
	{
		GLuint prog = glCreateProgram();
		glAttachShader(prog, vs);
		glAttachShader(prog, fs);
		glLinkProgram(prog);

		int length;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
		if (length > 0) {
			std::string info(length - 1, '\0');
			glGetProgramInfoLog(prog, length, nullptr, info.data());
			std::cout << info << '\n';
		}

		int status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		if (!status) {
			util::fatalError("Could not link program");
		}

		return prog;
	}
};
