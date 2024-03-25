#include "Player.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype) : Character(filepath, aContext, filetype)
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