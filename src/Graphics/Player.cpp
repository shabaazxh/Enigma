#include "Player.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype) : Character(filepath, aContext, filetype)
	{
		model->player = true;
	}

	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale) : Character(filepath, aContext, filetype, trans, scale)
	{
		model->player = true;
	}

	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale, float x, float y, float z) : Character(filepath, aContext, filetype, trans, scale, x, y, z)
	{
		model->player = true;
	}

	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale, glm::mat4 rm) : Character(filepath, aContext, filetype, trans, scale, rm)
	{
		model->player = true;
	}

	Player::Player(const VulkanContext& aContext) : Character(NO_MODEL, aContext, -1)
	{
		noModel = true;
	}

	void Player::ManageInputs() {
		//ToDo;
	}
}