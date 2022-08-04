#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/vec3.hpp>

namespace util {

template<typename... Args>
[[noreturn]] void fatalError(Args... args)
{
	throw (std::string(args) + ...);
}

inline std::string readFileAsString(char const *path)
{
	std::ifstream file(path);
	if (!file) {
		fatalError("Could not open file: ", path);
	}
	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
}

inline void print(glm::vec3 const &v)
{
	std::cout << v.x << ' ' << v.y << ' ' << v.z << '\n';
}

}
