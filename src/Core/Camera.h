#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/Common.h"
#include <GLFW/glfw3.h>

namespace Enigma
{
	class Camera
	{
		public:

			Camera() = default;
			Camera(const glm::vec3 position, glm::vec3 direction, glm::vec3 up, Time& time, float aspect) : m_position{ position }, m_direction{ direction }, m_up { up }, time{ time }
			{
			}

			void SetSpeed(float speed) { m_cameraSpeed = speed; }

			void SetPosition(glm::vec3 newpos) { m_position = newpos; }
			void SetDirection(glm::vec3 newdir) { m_direction = newdir; }
			void SetFoV(float fov) { m_transform.fov = fov; }
			void SetNearPlane(float nearPlane) { m_transform.nearPlane = nearPlane; }
			void SetFarPlane(float farPlane) { m_transform.farPlane = farPlane; }

			const CameraTransform& GetCameraTransform() const { return m_transform; }


			void Update(GLFWwindow* window, uint32_t width, uint32_t height)
			{
				Update(width, height);
				glm::vec3 movementAmount = glm::vec3(0.0f, 0.0f, 0.0f);

				if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
				{
					SetSpeed(100.0f);
				}
				else {
					SetSpeed(50.0f);
				}

				if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				{
					glm::vec3 forward = glm::normalize(m_direction);
					movementAmount += forward;
				}

				else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				{
					glm::vec3 forward = glm::normalize(m_direction);
					movementAmount -= forward;
				}

				if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				{
					glm::vec3 left = glm::normalize(glm::cross(m_direction, m_up));
					movementAmount -= left;
				}

				else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				{
					glm::vec3 right = glm::normalize(glm::cross(m_direction, m_up));
					movementAmount += right;
				}

				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				auto movement = movementAmount * speed;
				m_position += movement;
			}

			void Update(uint32_t width, uint32_t height)
			{
				m_transform.model = glm::mat4(1.0f);
				m_transform.view = glm::lookAt(m_position, m_position + m_direction, m_up);
				m_transform.projection = glm::perspective(m_transform.fov, width / (float) height, m_transform.nearPlane, m_transform.farPlane);
				m_transform.projection[1][1] *= -1;
				m_transform.cameraPosition = m_position;
			}

			void LogPosition() const
			{
				std::cout << "Position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << " Speed: " << m_cameraSpeed << std::endl;
			}

			glm::vec3 GetPosition() const { return m_position; }
			glm::vec3 GetDirection() const { return m_direction; }
			glm::vec3 GetUp() const { return m_up; }


			double yaw = 90.0f;
			double pitch = 0.0f;
		private:
			Time& time;
			CameraTransform m_transform;
			glm::vec3 m_position;
			glm::vec3 m_direction;
			glm::vec3 m_up;
			float m_cameraSpeed = 50.0f;
			glm::vec3 m_velocity = glm::vec3(0);

	};
}