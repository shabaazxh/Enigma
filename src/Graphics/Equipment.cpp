#include "Equipment.h"

namespace Enigma
{
	Equipment::Equipment(glm::vec3 v, Model* m) {
		m->setRotationY(180);
		m->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
		m->equipment = true;
		m->setOffset(v);
		offset = v;
		model = m;
	}

	glm::vec3 Equipment::getOffset() {
		return offset;
	}

	Model* Equipment::getModel() {
		return model;
	}

	void Equipment::setOffset(glm::vec3 v) {
		offset = v;
		model->setOffset(v);
	}

	void Equipment::setModel(Model* m) {
		model = m;
	}
}