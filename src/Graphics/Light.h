#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Enigma
{
	enum class LightType
	{
		DIRECTIONAL,
		POINT,
		SPOT
	};
	class Light
	{
		public:
			Light() = default;

			glm::vec4 m_position;
			glm::vec4 m_direction;
			glm::vec4 m_color;
			LightType m_type;
			float m_intensity;
			glm::mat4 LightSpaceMatrix;
			float theta = 0.f; // horizontal
			float phi = 0.f; // vertical 
			float view = 130.137f;
			float m_near = 1.0f;
			float m_far = 435.497f;
	};

	inline Light CreateDirectionalLight(const glm::vec4& pos, const glm::vec4& color, float intensity)
	{
		auto position = glm::vec3(pos.x, pos.y, pos.z);
		Light ret = {};

		ret.m_position = pos;
		ret.m_color = color;
		ret.m_type = LightType::DIRECTIONAL;
		ret.m_intensity = intensity;

		auto dir = glm::vec3(0.1, 0.1f, 0.14f);
		ret.m_direction = glm::vec4(dir.x, dir.y, dir.z, 1.0);

		//glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, ret.m_near, ret.m_far);
		glm::mat4 proj = glm::ortho(-ret.view, ret.view, -ret.view, ret.view, ret.m_near, ret.m_far);
		glm::mat4 view = glm::lookAt(position, glm::vec3(0.1, 0.1f, 0.14f), glm::vec3(0.0f, 1.0f, 0.0f));

		ret.LightSpaceMatrix = proj * view;

		return ret;
	}


}
