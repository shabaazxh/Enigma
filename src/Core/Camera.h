#pragma once

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/Common.h"

namespace Enigma
{
	class Camera
	{
		public:
			Camera() = default;
			Camera(const glm::vec3 position, glm::vec3 direction, glm::vec3 up, Time& time, float aspect) : m_position{ position }, m_direction{ direction }, m_up{ up }, time{ time } {}

			void SetSpeed(float speed) { m_cameraSpeed = speed; }

			void SetPosition(glm::vec3 newpos) { m_position = newpos; }
			void SetDirection(glm::vec3 newdir) { m_direction = newdir; }
			void SetFoV(float fov) { m_transform.fov = fov; }
			void SetNearPlane(float nearPlane) { m_transform.nearPlane = nearPlane; }
			void SetFarPlane(float farPlane) { m_transform.farPlane = farPlane; }

			const CameraTransform& GetCameraTransform() const { return m_transform; }

			glm::vec3 smoothstep(const glm::vec3& v1, const glm::vec3& v2, float t)
			{
				t = glm::clamp(t, 0.0f, 1.0f);
				return glm::smoothstep(v1, v2, glm::vec3(t));
			}

			void Forward()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position += speed * m_direction;
				//glm::vec3 movement = glm::vec3(0, 0, 1.0);
				//m_velocity = smoothstep(m_velocity, movement, 0.2f);
				//m_position += m_velocity * speed;
			}

			void Back()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position -= speed * m_direction;  
			}

			void Right()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position += glm::normalize(glm::cross(m_direction, m_up)) * speed;
			}

			void Left()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position -= glm::normalize(glm::cross(m_direction, m_up)) * speed;
			}

			void Up()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position += speed * m_up;
			}

			void Down()
			{
				float speed = static_cast<float>(m_cameraSpeed * time.deltaTime);
				m_position -= speed * m_up;
			}

			void Update(uint32_t width, uint32_t height)
			{
				m_transform.model = glm::mat4(1.0f);
				m_transform.view = glm::lookAt(m_position, m_position + m_direction, m_up);
				m_transform.projection = glm::perspective(m_transform.fov, width / (float) height, m_transform.nearPlane, m_transform.farPlane);
				m_transform.projection[1][1] *= -1;
			}

			void LogPosition()
			{
				std::cout << "Position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << " Speed: " << m_cameraSpeed << std::endl;
			}

			glm::vec3 GetPosition() const { return m_position; }
			glm::vec3 GetDirection() const { return m_direction; }
			glm::vec3 GetUp() const { return m_up; }

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