// Collision.cpp
#include "Collision.h"

namespace Enigma {

    bool CollisionDetector::PlayerAABBvsAABB(Player& player, Model& obj) {
        glm::vec4 playerAABBmax = glm::vec4(player.model->meshes[0].meshAABB.max, 1.f);
        glm::vec4 playerAABBmin = glm::vec4(player.model->meshes[0].meshAABB.min, 1.f);
        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0), player.getTranslation());
        //modelMatrix = glm::rotate(modelMatrix, glm::radians(player.getYRotation()), glm::vec3(0, 1, 0));
        //modelMatrix = glm::rotate(modelMatrix, glm::radians(player.getXRotation()), glm::vec3(1, 0, 0));
        //modelMatrix = glm::rotate(modelMatrix, glm::radians(player.getZRotation()), glm::vec3(0, 0, 1));
        modelMatrix = glm::scale(modelMatrix, player.getScale());
        glm::vec4 playerAABBmaxTrans = modelMatrix * playerAABBmax;
        glm::vec4 playerAABBminTrans = modelMatrix * playerAABBmin;

        for (int i = 0; i < obj.meshes.size(); i++) {
            if (obj.meshes[i].meshAABB.min.x <= playerAABBmaxTrans.x &&
                obj.meshes[i].meshAABB.max.x >= playerAABBminTrans.x &&
                obj.meshes[i].meshAABB.min.y <= playerAABBmaxTrans.y &&
                obj.meshes[i].meshAABB.max.y >= playerAABBminTrans.y &&
                obj.meshes[i].meshAABB.min.z <= playerAABBmaxTrans.z &&
                obj.meshes[i].meshAABB.max.z >= playerAABBminTrans.z
                ) {
                return true;
            }
        }
        return false;
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