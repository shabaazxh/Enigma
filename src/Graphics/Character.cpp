#include "Character.h"

namespace Enigma
{
	Character::Character(const std::string& filepath, const VulkanContext& aContext)
	{
		if (filepath != NO_MODEL) {
			model = new Model(filepath, aContext);
		}
		health = 100.f;
	}

	void Character::ManageAnimation() {
		//	ToDo
	}

	void Character::ManageCollisions() {
		//	ToDo
	}


	void Character::setTranslation(glm::vec3 t) {
		translation = t;
		if (!noModel) {
			model->translation = translation;
		}
	}

	void Character::setScale(glm::vec3 s) {
		scale = s;
		if (!noModel) {
			model->scale = scale;
		}
	}

	void Character::setRotationX(float angle) {
		rotationX = angle;
		if (!noModel) {
			model->rotationX = rotationX;
		}
	}

	void Character::setRotationY(float angle) {
		rotationY = angle;
		if (!noModel) {
			model->rotationY = rotationY;
		}
	}

	void Character::setRotationZ(float angle) {
		rotationZ = angle;
		if (!noModel) {
			model->rotationZ = rotationZ;
		}
	}

	void Character::setRotationMatrix(glm::mat4 rm) {
		rotMatrix = rm;
		if (!noModel) {
			model->rotMatrix = rotMatrix;
		}
	}

}

