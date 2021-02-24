#pragma once

#include <fstream>
#include <vector>
#include <string>
#include <cstdio>

// auto const PI = 3.1415926535897932384626433f;

template <typename ...Args>
void die(char const *format, Args ...args) {
    fprintf(stderr, format, args...);
    exit(EXIT_FAILURE);
}

inline std::string read_text_file(char const *path) {
	std::ifstream file(path);
	if (!file) {
		die("could not open file: %s\n", path);
	}
	return {
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()};
}

inline std::vector<char> read_binary_file(char const *path) {
	// TODO make one functions for reading all files
	std::ifstream file(path, std::ios_base::binary);
	if (!file) {
		die("could not open file: %s\n", path);
	}
	return {
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()};
}
