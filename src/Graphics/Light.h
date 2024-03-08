#pragma once

#include <glm/glm.hpp>

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

			glm::vec3 m_position;
			glm::vec3 m_direction;
			glm::vec3 m_color;
			LightType m_type;
			float m_intensity;
	};

	inline Light CreateDirectionalLight(const glm::vec3& pos, const glm::vec3& color, float intensity)
	{
		Light ret{};

		ret.m_position = pos;
		ret.m_color = color;
		ret.m_type = LightType::DIRECTIONAL;
		ret.m_intensity = intensity;

		return ret;
	}
}
