// Collision.cpp
#include "Collision.h"

namespace Enigma {
    bool CollisionDetector::CheckCollision(Model& character, std::vector<Model>& environment) {
        for (auto& object : environment) {
            if (AABBvsAABB(character, object)) {
                return true; // 发现碰撞
            }
        }
        return false; // 未发现碰撞
    }

    bool CollisionDetector::AABBvsAABB(Model& obj1, Model& obj2) {
        // 假设Model类有方法获取AABB的最小点(min)和最大点(max)
        auto obj1Min = obj1.GetAABBMin();
        auto obj1Max = obj1.GetAABBMax();
        auto obj2Min = obj2.GetAABBMin();
        auto obj2Max = obj2.GetAABBMax();

        // 检查所有轴上的重叠 detect all the axis
        if (obj1Max.x < obj2Min.x || obj1Min.x > obj2Max.x) return false;
        if (obj1Max.y < obj2Min.y || obj1Min.y > obj2Max.y) return false;
        if (obj1Max.z < obj2Min.z || obj1Min.z > obj2Max.z) return false;

        return true; // AABBs重叠
    }

    //detect the ray and object
    bool CollisionDetector::RayIntersectsAABB(const Ray& ray, const AABB& aabb) {
        float tmin = (aabb.min.x - ray.origin.x) / ray.direction.x;
        float tmax = (aabb.max.x - ray.origin.x) / ray.direction.x;

        if (tmin > tmax) std::swap(tmin, tmax);

        float tymin = (aabb.min.y - ray.origin.y) / ray.direction.y;
        float tymax = (aabb.max.y - ray.origin.y) / ray.direction.y;

        if (tymin > tymax) std::swap(tymin, tymax);

        if ((tmin > tymax) || (tymin > tmax))
            return false;

        if (tymin > tmin)
            tmin = tymin;

        if (tymax < tmax)
            tmax = tymax;

        float tzmin = (aabb.min.z - ray.origin.z) / ray.direction.z;
        float tzmax = (aabb.max.z - ray.origin.z) / ray.direction.z;

        if (tzmin > tzmax) std::swap(tzmin, tzmax);

        if ((tmin > tzmax) || (tzmin > tmax))
            return false;

        return true;
    }
}

/*调用#include "Core/Collision/CollisionDetector.h"

// 假设有character和environment对象
bool collisionDetected = CollisionDetector::CheckCollision(character, environment);
if (collisionDetected) {
    // use the collision detection 使用碰撞检测（角色与环境）
}

bool bulletHit = CollisionDetector::CheckBulletCollision(bullet, character);
if (bulletHit) {
    // 处理子弹击中角色的逻辑，比如减少生命值等
}

Enigma::Ray bulletRay(bulletOrigin, bulletDirection);
for (const auto& target : targets) { // 假设targets是一系列模型
    if (Enigma::RayIntersectsAABB(bulletRay, target.GetModelAABB())) {
        // 处理碰撞逻辑，例如减少生命值等  response the collision logic,like reduce health bar
    }
}
*/