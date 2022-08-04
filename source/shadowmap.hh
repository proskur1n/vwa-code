#pragma once

#include <glad.h>
#include "camera.hh"
#include "program.hh"
#include "mesh.hh"

class ShadowMap {
	Camera camera;
	int resolution {1024};
	GLuint depthAttachment {0};
	GLuint framebuffer {0};
	Program program {"shaders/shadowPass.vert", "shaders/shadowPass.frag"};
public:
	ShadowMap(glm::vec3 const &position, glm::vec3 const &lookAt)
		: camera(position, lookAt)
	{
		// TODO
		camera.nearPlane = 9.0f;
		camera.updateProjectionMatrix();

		depthAttachment = createDepthTexture(resolution, resolution);
		framebuffer = createDepthOnlyFramebuffer(depthAttachment);
	}

	ShadowMap(ShadowMap const &) = delete;

	ShadowMap &operator=(ShadowMap const &) = delete;

	~ShadowMap()
	{
		glDeleteFramebuffers(1, &framebuffer);
		glDeleteTextures(1, &depthAttachment);
	}

	[[nodiscard]] Camera const &getCamera() const
	{
		return camera;
	}

	[[nodiscard]] Camera &getCamera()
	{
		return camera;
	}

	void renderShadowPass(std::vector<Mesh> const &meshes) const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glViewport(0, 0, resolution, resolution);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);

		program.use();
		program.set("uView", camera.viewMatrix);
		program.set("uProj", camera.projMatrix);

		for (auto const &mesh: meshes) {
			program.set("uModel", mesh.modelMatrix);
			glBindVertexArray(mesh.vao);
			glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
		}
	}

	[[nodiscard]] GLuint getDepthAttachment() const
	{
		return depthAttachment;
	}

private:
	[[nodiscard]] static GLuint createDepthTexture(int width, int height)
	{
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT,
		             GL_UNSIGNED_BYTE, nullptr);

		float border[4] = {1.0f, 0.0f, 0.0f, 0.0f};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

		return texture;
	}

	[[nodiscard]] static GLuint createDepthOnlyFramebuffer(GLuint depthTexture)
	{
		GLuint fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
		// Disable color attachment.
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			util::fatalError("Framebuffer is not complete");
		}
		return fb;
	}
};
