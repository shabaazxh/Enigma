#pragma once

#include "../Graphics/Player.h"
#include <vector>
#include <glm/glm.hpp>

namespace Enigma {

    //定义射线 define ray
    struct Ray {
        glm::vec3 origin;    // 射线的起点
        glm::vec3 direction; // 射线的方向，应当是单位向量

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
        // 检测角色与环境之间的碰撞
        static bool CheckCollision(Model& character, std::vector<Model>& environment);
        // 检测子弹与角色的碰撞
        collisionData RayIntersectsAABB(const Ray& ray, const AABB& aabb); 
        bool PlayerAABBvsAABB(Player& player, Model& obj);

    private:
        // 检测两个AABB之间的碰撞
    };
}