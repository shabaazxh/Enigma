#pragma once

#include "../Graphics/Model.h"  // 假设Model类中包含了物体的位置、大小等信息
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#include <glm/glm.hpp>

namespace Enigma {

        //定义射线 define ray
    struct Ray {
        glm::vec3 origin;    // 射线的起点
        glm::vec3 direction; // 射线的方向，应当是单位向量

        Ray(glm::vec3& origin, glm::vec3& direction) : origin(origin), direction(glm::normalize(direction)) 
        {
            //direction = direction * 0.0001f;
        }
    };

    class CollisionDetector {
        public:
            // 检测角色与环境之间的碰撞
            static bool CheckCollision(Model& character, std::vector<Model>& environment);
            // 检测子弹与角色的碰撞
            static bool CheckBulletCollision(Model& bullet, Model& character) {
                return AABBvsAABB(bullet, character);
            };
            bool RayIntersectsAABB(const Ray& ray, const AABB& aabb);

        private:
            // 检测两个AABB之间的碰撞
            static bool AABBvsAABB(Model & obj1, Model & obj2);
    };
}