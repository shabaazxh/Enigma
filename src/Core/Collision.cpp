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
    collisionData CollisionDetector::RayIntersectsAABB(const Ray& ray, const AABB& aabb) {
        collisionData data;

        float tmin = (aabb.min.x - ray.origin.x) / ray.direction.x;
        float tmax = (aabb.max.x - ray.origin.x) / ray.direction.x;

        if (tmin > tmax) std::swap(tmin, tmax);

        float tymin = (aabb.min.y - ray.origin.y) / ray.direction.y;
        float tymax = (aabb.max.y - ray.origin.y) / ray.direction.y;

        if (tymin > tymax) std::swap(tymin, tymax);

        if ((tmin > tymax) || (tymin > tmax)) {
            data.intersects = false;
            data.t = 0;
            return data;
        }

        if (tymin > tmin)
            tmin = tymin;

        if (tymax < tmax)
            tmax = tymax;

        float tzmin = (aabb.min.z - ray.origin.z) / ray.direction.z;
        float tzmax = (aabb.max.z - ray.origin.z) / ray.direction.z;

        if (tzmin > tzmax) std::swap(tzmin, tzmax);

        if ((tmin > tzmax) || (tzmin > tmax)) {
            data.intersects = false;
            data.t = 0;
            return data;
        }

        data.intersects = true;
        data.t = tmin;
        return data;
    }
}