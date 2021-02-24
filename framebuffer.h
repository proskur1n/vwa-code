#pragma once

#include <glad/glad.h>
#include "util.h"

class framebuffer {
    GLuint handle {0};
public:

    // TODO private fields ?
    int width;
    int height;
    struct {
        GLuint color {0};
        GLuint depth {0};
    } attachments;

    framebuffer() :
        width(1),
        height(1) {};

    framebuffer(int _width, int _height) :
        width(_width),
        height(_height) {}

    float get_aspect_ratio() const {
        return float(width) / float(height);
    }

    void bind_for_rendering() {
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glViewport(0, 0, width, height);
    }

    static framebuffer make_shadow_buffer(int width, int height) {
        GLuint depth_attachment;
        glGenTextures(1, &depth_attachment);
        glBindTexture(GL_TEXTURE_2D, depth_attachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

        float border[4] = {1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        // TODO only 2 wraps are needed (which ones ?)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        // TODO are min and mag filters needed ?
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        GLuint handle;
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
            depth_attachment, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            die("framebuffer is not complete\n");
        }

        framebuffer fbo(width, height);
        fbo.handle = handle;
        fbo.attachments.depth = depth_attachment;
        return fbo;
    }
};
