#pragma once

#include "../Graphics/Model.h"  // ����Model���а����������λ�á���С����Ϣ
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/glm.hpp>

namespace Enigma {

        //�������� define ray
    struct Ray {
        glm::vec3 origin;    // ���ߵ����
        glm::vec3 direction; // ���ߵķ���Ӧ���ǵ�λ����

        Ray(glm::vec3& origin, glm::vec3& direction) : origin(origin), direction(glm::normalize(direction)) 
        {
            //direction = direction * 0.0001f;
        }
    };

    class CollisionDetector {
        public:
            // ����ɫ�뻷��֮�����ײ
            static bool CheckCollision(Model& character, std::vector<Model>& environment);
            // ����ӵ����ɫ����ײ
            static bool CheckBulletCollision(Model& bullet, Model& character) {
                return AABBvsAABB(bullet, character);
            };
            bool RayIntersectsAABB(const Ray& ray, const AABB& aabb);

        private:
            // �������AABB֮�����ײ
            static bool AABBvsAABB(Model & obj1, Model & obj2);
    };
}