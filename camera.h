#pragma once

#include <glad/glad.h>
#include <linmath/linmath.h>

struct camera {
    mat4x4 view_matrix;
    mat4x4 proj_matrix;

    vec3 position {0.0f, 0.0f, 0.0f};
    vec2 rotation {0.0f, 0.0f};

    float near_plane {0.2f};
    float far_plane {20.0f};
    float fov_y {1.57f};

    camera() {
        calc_view();
        calc_projection(1.0f);
    };

    camera(vec3 pos, vec3 target) {
        mat4x4_look_at(view_matrix, pos, target, vec3{0.0, 1.0, 0.0});
        vec3_dup(position, pos);
        rotation[0] = atan2f(view_matrix[0][2], view_matrix[2][2]);
        rotation[1] = -asinf(view_matrix[1][2]);
        calc_projection(1.0f); // TODO aspect
    }

    // TODO wrong aspect for default framebuffer
    // TODO constructor with fbo

    float get_frustum_width() const {
        return 2.0f * tanf(fov_y / 2.0f) * near_plane;
    }

    void calc_view() {
        mat4x4_identity(view_matrix);
	    mat4x4_rotate_X(view_matrix, view_matrix, -rotation[1]);
	    mat4x4_rotate_Y(view_matrix, view_matrix, -rotation[0]);
	    mat4x4_translate_in_place(view_matrix, -position[0], -position[1], -position[2]);
    }

    void calc_projection(float aspect) {
        mat4x4_perspective(proj_matrix, fov_y, aspect, near_plane, far_plane);
    }
};