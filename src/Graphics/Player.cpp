#include "Player.h"
#include "../Core/Camera.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, const VulkanContext& aContext, int filetype) : Character(filepath, aContext, filetype)
	{
		model->player = true; // why do we need this, it doesnt make sense
	}

	Player::Player(const VulkanContext& aContext) : Character(NO_MODEL, aContext, -1)
	{
		noModel = true; // this is not needed either 
	}

	void Player::ManageInputs() {
		//ToDo;
	}
}