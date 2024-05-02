#pragma once

#include "../Graphics/Player.h"
#include <vector>
#include <glm/glm.hpp>

namespace Enigma {

    //�������� define ray
    struct Ray {
        glm::vec3 origin;    // ���ߵ����
        glm::vec3 direction; // ���ߵķ���Ӧ���ǵ�λ����

        Ray(glm::vec3 origin, glm::vec3 direction) : origin(origin), direction(glm::normalize(direction))
        {
            //direction = direction * 0.0001f;
        }
    };

    struct collisionData {
        bool intersects;
        float t;
    };

    class CollisionDetector {
    public:
        // ����ɫ�뻷��֮�����ײ
        static bool CheckCollision(Model& character, std::vector<Model>& environment);
        // ����ӵ����ɫ����ײ
        collisionData RayIntersectsAABB(const Ray& ray, const AABB& aabb); 
        bool PlayerAABBvsAABB(Player& player, Model& obj);

    private:
        // �������AABB֮�����ײ
    };
}