#pragma once

#include "../Graphics/Model.h"  // 假设Model类中包含了物体的位置、大小等信息
#include <vector>
#include <glm/glm.hpp>

namespace Enigma {

    //定义射线 define ray
    struct Ray {
        glm::vec3 origin;    // 射线的起点
        glm::vec3 direction; // 射线的方向，应当是单位向量

        Ray(const glm::vec3& origin, const glm::vec3& direction) : origin(origin), direction(glm::normalize(direction)) {}
    };
}

class CollisionDetector {
public:
    // 检测角色与环境之间的碰撞
    static bool CheckCollision(const Model& character, const std::vector<Model>& environment);
    // 检测子弹与角色的碰撞
    static bool CheckBulletCollision(const Model& bullet, const Model& character) {
        return AABBvsAABB(bullet, character);

private:
    // 检测两个AABB之间的碰撞
    static bool AABBvsAABB(const Model& obj1, const Model& obj2);
};