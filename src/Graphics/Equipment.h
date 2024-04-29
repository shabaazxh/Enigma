#pragma once

#include "Model.h"

namespace Enigma
{
	class Equipment
	{
		private:
			glm::vec3 offset;
			Model* model;

		public:	
			Equipment(glm::vec3 v, Model* m);
			glm::vec3 getOffset();
			Model* getModel();
			void setOffset(glm::vec3 v);
			void setModel(Model* m);
	};
}

