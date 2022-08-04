#include <fstream>
#include <string>
#include <unordered_map>
#include <glm/vec3.hpp>
#include "parser.hh"
#include "mesh.hh"
#include "util.hh"

static glm::vec3 readVector3(std::istream &stream)
{
	glm::vec3 v;
	stream >> v.x;
	stream >> v.y;
	stream >> v.z;
	return v;
}

static bool isNumeric(int c)
{
	return (c >= '0' && c <= '9') || c == '-';
}

struct Material {
	glm::vec3 diffuse {0.8f, 0.8f, 0.8f};
};

using MaterialLibrary = std::unordered_map<std::string, Material>;

static MaterialLibrary parseMtlLibrary(std::filesystem::path const &path)
{
	MaterialLibrary result;
	std::string materialName;
	std::ifstream file(path);
	if (!file.is_open()) {
		util::fatalError("Could not open MTL file: ", path);
	}

	while (file.good()) {
		std::string token;
		file >> token;
		if (token == "newmtl") {
			file >> materialName;
		} else if (token == "Kd") {
			result[materialName].diffuse = readVector3(file);
		}
		file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		file >> std::ws;
	}

	if (file.fail()) {
		util::fatalError("Could not parse material library: ", path);
	}

	return result;
}

static Vertex readVertex(
	std::istream &stream,
	std::vector<glm::vec3> const &positions,
	std::vector<glm::vec3> const &normals,
	Material const &material)
{
	size_t indices[3] {0, 0, 0};
	for (int i = 0; i < 3; ++i) {
		if (i == 0 || isNumeric(stream.peek())) {
			stream >> indices[i];
		}
		if (stream.peek() == '/') {
			stream.ignore(1);
		}
	}
	if (indices[0] < 0) {
		indices[0] += positions.size();
	}
	if (indices[2] < 0) {
		indices[2] += normals.size();
	}
	return {positions[indices[0]], normals[indices[2]], material.diffuse};
}


std::vector<Mesh> loadMeshesFromFile(std::filesystem::path const &path)
{
	// Append a dummy value, because OBJ indexes begin at 1.
	std::vector<glm::vec3> positions(1);
	std::vector<glm::vec3> normals(1);
	std::vector<Mesh> meshes;
	std::vector<Vertex> meshVertices;
	MaterialLibrary materials;
	Material currentMaterial;

	std::ifstream file(path);
	if (!file.is_open()) {
		util::fatalError("Could not open OBJ file: ", path);
	}

	while (file.good()) {
		std::string token;
		file >> token;
		if (token == "v") {
			positions.push_back(readVector3(file));
		} else if (token == "vn") {
			normals.push_back(readVector3(file));
		} else if (token == "f") {
			// This assumes convex polygons.
			Vertex first = readVertex(file, positions, normals, currentMaterial);
			Vertex last = readVertex(file, positions, normals, currentMaterial);

			file >> std::ws;
			while (file.good() && isNumeric(file.peek())) {
				Vertex curr = readVertex(file, positions, normals, currentMaterial);

				meshVertices.push_back(first);
				meshVertices.push_back(last);
				meshVertices.push_back(curr);

				last = curr;
				file >> std::ws;
			}
		} else if (token == "mtllib") {
			std::string lib;
			file >> lib;
			materials = parseMtlLibrary(path.parent_path().append(lib));
		} else if (token == "usemtl") {
			std::string name;
			file >> name;
			currentMaterial = materials[name];
		} else if (token == "o") {
			if (!meshVertices.empty()) {
				meshes.emplace_back(meshVertices);
				meshVertices.clear();
			}
		}
		if (token != "f") {
			file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
		file >> std::ws;
	}
	if (!meshVertices.empty()) {
		meshes.emplace_back(meshVertices);
	}

	if (file.fail()) {
		util::fatalError("Could not parse file: ", path);
	}

	return meshes;
}
