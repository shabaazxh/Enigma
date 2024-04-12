#include "Character.h"

namespace Enigma
{
	Character::Character(const std::string& filepath, const VulkanContext& aContext, int filetype)
	{
		if (filepath != NO_MODEL) {
			model = new Model(filepath, aContext, filetype);
		}
		health = 100.f;
	}

	Character::Character(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 s) {
		if (filepath != NO_MODEL) {
			model = new Model(filepath, aContext, filetype);
		}
		health = 100.f;
		this->setTranslation(trans);
		this->setScale(s);
	}

	Character::Character(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 s, float x, float y, float z) {
		if (filepath != NO_MODEL) {
			model = new Model(filepath, aContext, filetype);
		}
		health = 100.f;
		this->setTranslation(trans);
		this->setScale(s);
		this->setRotationX(x);
		this->setRotationY(y);
		this->setRotationZ(z);
	}

	Character::Character(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 s, glm::mat4 rm) {
		if (filepath != NO_MODEL) {
			model = new Model(filepath, aContext, filetype);
		}
		health = 100.f;
		this->setTranslation(trans);
		this->setScale(s);
		this->setRotationMatrix(rm);
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
			model->setTranslation(t);
		}
	}

	void Character::setScale(glm::vec3 s) {
		scale = s;
		if (!noModel) {
			model->setScale(s);
		}
	}

	void Character::setRotationX(float angle) {
		rotationX = angle;
		if (!noModel) {
			model->setRotationX(angle);
		}
	}

	void Character::setRotationY(float angle) {
		rotationY = angle;
		if (!noModel) {
			model->setRotationY(angle);
		}
	}

	void Character::setRotationZ(float angle) {
		rotationZ = angle;
		if (!noModel) {
			model->setRotationZ(angle);
		}
	}

	void Character::setRotationMatrix(glm::mat4 rm) {
		rotMatrix = rm;
		if (!noModel) {
			model->setRotationMatrix(rm);
		}
	}

}

