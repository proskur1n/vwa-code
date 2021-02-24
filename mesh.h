#pragma once

#include <glad/glad.h>
#include <linmath/linmath.h>

class mesh {
    GLint index {-1}; /* Used for object picking */

    GLuint vao {0};
    GLsizei vertex_count {0};
public:
    mat4x4 model_matrix;
    vec3 color {0.7, 0.7, 0.7};

    mesh() {
        mat4x4_identity(model_matrix);
    }

    GLint get_index() const {
        return index;
    }

    void draw() const {
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertex_count);
    }

    static mesh load_stl(char const *path, vec3 position);
};