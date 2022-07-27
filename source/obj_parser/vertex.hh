#pragma once

#include <glm/vec3.hpp>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	// Storing color for each vertex is not very memory efficient, but it
	// works for this simple application.
	glm::vec3 color;
};
