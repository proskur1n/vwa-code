#pragma once

#include <vector>
#include <filesystem>
#include "mesh.hh"

// Very basic Wavefront OBJ format parser. File content is not validated during
// parsing.
std::vector<Mesh> loadMeshesFromFile(std::filesystem::path const &);
