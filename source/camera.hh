#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

struct Camera {
	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;
	glm::vec3 position;
	float nearPlane {2.0f};
	float farPlane {50.0f};
	float fovY {glm::half_pi<float>() * 0.75f};
	float aspectRatio {1.0f};

	// NOTE: Do not forget to call setAspectRatio() afterwards. Otherwise,
	// the projection matrix will probably not be correct.
	Camera(glm::vec3 const &position, glm::vec3 const &target)
		: viewMatrix(glm::lookAt(position, target, Y_AXIS)),
		  position(position)
	{
		updateProjectionMatrix();
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
		auto [pitch, yaw] = getRotation();
		glm::mat4 localToWorld = glm::rotate(identity, yaw, Y_AXIS);

		position += glm::mat3(localToWorld) * displacement;
		viewMatrix = glm::rotate(identity, -(pitch - mouseDelta.y), X_AXIS);
		viewMatrix = glm::rotate(viewMatrix, -(yaw - mouseDelta.x), Y_AXIS);
		viewMatrix = glm::translate(viewMatrix, -position);
	}

	[[nodiscard]] float getFrustumWidth() const
	{
		return 2.0f * std::tan(fovY / 2.0f) * nearPlane;
	}

	[[nodiscard]] glm::vec3 getForwardVector() const
	{
		return {-viewMatrix[0][2], -viewMatrix[1][2], -viewMatrix[2][2]};
	}

	// Moves camera on a circular path around the world's origin.
	void animate(float deltaTime)
	{
		float const PACE = 0.3f;
		float const MAGNITUDE = 3.0f;
		float const HEIGHT = 20.0f;

		float angle = std::atan2(position.z, position.x);
		float newAngle = angle + deltaTime * PACE;
		float length = std::hypot(position.x, position.z);

		position = glm::vec3(
			std::cos(newAngle) * length,
			HEIGHT + std::cos(angle * 3) * MAGNITUDE,
			std::sin(newAngle) * length
		);
		viewMatrix = glm::lookAt(position, glm::vec3(0.0f), Y_AXIS);
	}

private:
	[[nodiscard]] glm::vec2 getRotation() const
	{
		glm::vec3 f = getForwardVector();
		// Rotation around the x-axis.
		float pitch = std::asin(f.y);
		// Rotation around the y-axis.
		float yaw = std::atan2(f.x, f.z) + glm::pi<float>();
		return {pitch, yaw};
	}

	static constexpr glm::vec3 X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
	static constexpr glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
};
