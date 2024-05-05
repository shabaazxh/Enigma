#pragma once

#include "../Core/Camera.h"
#include "Model.h"
class Player;

namespace Enigma
{
	namespace Physics
	{
		inline bool isAABBColliding(AABB obj1, const glm::vec3 position, const AABB& obj2)
		{
			// Calculate the actual AABB of the player by incorporating the player's position
			obj1.min += position;
			obj1.max += position;

			// Check for collision along all three axes
			return (
				obj1.max.x >= obj2.min.x &&
				obj1.min.x <= obj2.max.x &&
				obj1.max.y >= obj2.min.y &&
				obj1.min.y <= obj2.max.y &&
				obj1.max.z >= obj2.min.z &&
				obj1.min.z <= obj2.max.z
				);
		}

		struct Ray {
			glm::vec3 origin;
			glm::vec3 direction;
		};

		inline Enigma::Physics::Ray RayCast(Camera* camera, float distance)
		{
			glm::mat4 invView = glm::inverse(camera->GetCameraTransform().view);
			return Enigma::Physics::Ray{ camera->GetPosition(), -glm::vec3(invView[2]) };
		}

		inline bool RayIntersectAABB(const Ray& ray, const Enigma::AABB meshAABB)
		{
			float Tnear = -std::numeric_limits<float>::infinity();
			float Tfar = std::numeric_limits<float>::infinity();

			// iterate over each pair of planes associated with X, Y, and Z
			for (int i = 0; i < 3; ++i) {
				float planeMin = meshAABB.min[i];
				float planeMax = meshAABB.max[i];
				float rayOrigin = ray.origin[i];
				float rayDirection = ray.direction[i];

				// ray is parallel to the plane
				if (rayDirection == 0) {
					// Ray origin not between the slabs
					if (rayOrigin < planeMin || rayOrigin > planeMax) {
						return false;
					}
				}
				else {
					// compute intersection distance of the planes
					float T1 = (planeMin - rayOrigin) / rayDirection;
					float T2 = (planeMax - rayOrigin) / rayDirection;

					// ensure T1 <= T2
					if (T1 > T2) {
						std::swap(T1, T2);
					}

					// update Tnear and Tfar
					Tnear = std::max(Tnear, T1);
					Tfar = std::min(Tfar, T2);

					// check for miss or box behind ray
					if (Tnear > Tfar || Tfar < 0) {
						return false;
					}
				}
			}

			return true;
		}
	}
}