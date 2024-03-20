#include "Player.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, const VulkanContext& aContext) : Character(filepath, aContext)
	{
		model->player = true;
	}

	Player::Player(const VulkanContext& aContext) : Character(NO_MODEL, aContext)
	{
		noModel = true;
	}

	void Player::ManageInputs() {
		//ToDo;
	}
}