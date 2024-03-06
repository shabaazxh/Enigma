#include "Player.h"

namespace Enigma
{
	Player::Player(const std::string& filepath, Allocator& aAllocator, const VulkanContext& aContext) : Character(filepath, aAllocator, aContext)
	{
		LoadModel(filepath);
		GetBoundingBox();
	}

	void Player::ManageInputs() {
		//ToDo;
	}
}