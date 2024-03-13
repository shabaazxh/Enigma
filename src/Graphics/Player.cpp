#include "Player.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, const VulkanContext& aContext) : Character(filepath, aContext)
	{
		player = true;
	}

	void Player::ManageInputs() {
		//ToDo;
	}
}