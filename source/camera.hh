#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

struct Camera {
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec3 position;
	float yaw {0.0f}; // Rotation around the Y-axis.
	float pitch {0.0f}; // Rotation around the X-axis.
	float nearPlane {3.0f};
	float farPlane {50.0f};
	float fovY {glm::half_pi<float>()};
	float aspectRatio {1.0f};

	// NOTE: Do not forget to call setAspectRatio() afterwards. Otherwise,
	// the projection matrix will probably not be correct.
	Camera(glm::vec3 const &position, glm::vec3 const &target)
		: viewMatrix(glm::lookAt(position, target, Y_AXIS)),
		  position(position)
	{
		updateProjectionMatrix();
		glm::vec3 dir = glm::normalize(target - position);
		pitch = std::asin(dir.y);
		yaw = std::atan2(dir.x, dir.z) + glm::pi<float>();
	}

	void setAspectRatio(int width, int height)
	{
		aspectRatio = float(width) / float(height);
		updateProjectionMatrix();
	}

	void updateProjectionMatrix()
	{
		projMatrix = glm::perspective(fovY, aspectRatio, nearPlane, farPlane);
	}

	// Takes user input in camera space and adjusts camera's position,
	// rotation and viewMatrix in world coordinates.
	void applyUserInput(glm::vec3 const &displacement, glm::vec2 const &mouseDelta)
	{
		static constexpr glm::mat4 identity = glm::mat4(1.0f);
		glm::mat4 localToWorld = glm::rotate(identity, yaw, Y_AXIS);

		position += glm::mat3(localToWorld) * displacement;
		pitch -= mouseDelta.y;
		yaw -= mouseDelta.x;

		viewMatrix = glm::rotate(identity, -pitch, X_AXIS);
		viewMatrix = glm::rotate(viewMatrix, -yaw, Y_AXIS);
		viewMatrix = glm::translate(viewMatrix, -position);
	}

private:
	static constexpr glm::vec3 X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
	static constexpr glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
};
