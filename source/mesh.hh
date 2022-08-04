#pragma once

#include <vector>
#include <utility>
#include <glad.h>
#include <glm/glm.hpp>
#include "obj_parser/vertex.hh"

class Mesh {
	GLuint vao {0};
	GLuint vbo {0};
	int vertexCount {-1};
public:
	explicit Mesh(std::vector<Vertex> const &vertices)
		: vertexCount(static_cast<int>(vertices.size()))
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		auto totalSize = static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex));
		glBufferData(GL_ARRAY_BUFFER, totalSize, vertices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		GLsizei stride = sizeof(Vertex);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (GLvoid *) offsetof(Vertex, pos));
		glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (GLvoid *) offsetof(Vertex, normal));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, stride, (GLvoid *) offsetof(Vertex, color));
	}

	Mesh(Mesh const &) = delete;

	Mesh &operator=(Mesh const &) = delete;

	Mesh(Mesh &&other) noexcept
	{
		*this = std::move(other);
	}

	Mesh &operator=(Mesh &&other) noexcept
	{
		if (this != &other) {
			glDeleteBuffers(1, &vbo);
			glDeleteVertexArrays(1, &vao);
			vbo = std::exchange(other.vbo, 0);
			vao = std::exchange(other.vao, 0);
			vertexCount = std::exchange(other.vertexCount, -1);
		}
		return *this;
	}

	~Mesh() noexcept
	{
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}

	[[nodiscard]] glm::mat4 const &getModelMatrix() const noexcept
	{
		// Implemented for future use.
		static constexpr glm::mat4 modelMatrix(1.0f);
		return modelMatrix;
	}

	void drawArrays() const noexcept
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}
};
