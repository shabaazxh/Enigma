// Collision.cpp
#include "Collision.h"

namespace Enigma {
    bool CollisionDetector::CheckCollision(Model& character, std::vector<Model>& environment) {
        for (auto& object : environment) {
            if (AABBvsAABB(character, object)) {
                return true; // ������ײ
            }
        }
        return false; // δ������ײ
    }

    bool CollisionDetector::AABBvsAABB(Model& obj1, Model& obj2) {
        // ����Model���з�����ȡAABB����С��(min)������(max)
        auto obj1Min = obj1.GetAABBMin();
        auto obj1Max = obj1.GetAABBMax();
        auto obj2Min = obj2.GetAABBMin();
        auto obj2Max = obj2.GetAABBMax();

        // ����������ϵ��ص� detect all the axis
        if (obj1Max.x < obj2Min.x || obj1Min.x > obj2Max.x) return false;
        if (obj1Max.y < obj2Min.y || obj1Min.y > obj2Max.y) return false;
        if (obj1Max.z < obj2Min.z || obj1Min.z > obj2Max.z) return false;

        return true; // AABBs�ص�
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
class Grid {
public:
    std::unordered_map<int, std::vector<Model*>> cells;

    int computeGridKey(const glm::vec3& position) {
        int x = static_cast<int>(floor(position.x / cellSize));
        int z = static_cast<int>(floor(position.z / cellSize));
        return x + 31 * z;  // һ���򵥵�ɢ�к���
    }

    void addModel(Model* model) {
        int key = computeGridKey(model->getPosition());
        cells[key].push_back(model);
    }

    std::vector<Model*>& getModelsInCell(int key) {
        return cells[key];
    }
private:
    float cellSize = 10.0f;  // ����Ԫ�Ĵ�С������ʵ���������
};

// ��ײ��⺯��
bool AABBvsAABB(const AABB& a, const AABB& b) {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
        (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
        (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool checkCollision(Model& character, Grid& grid) {
    int gridKey = grid.computeGridKey(character.getPosition());
    auto& models = grid.getModelsInCell(gridKey);

    for (auto model : models) {
        if (AABBvsAABB(character.getAABB(), model->getAABB())) {
            return true;  // ������ײ
        }
    }

    return false;  // δ������ײ
}


/*����#include "Core/Collision/CollisionDetector.h"

// ������character��environment����
bool collisionDetected = CollisionDetector::CheckCollision(character, environment);
if (collisionDetected) {
    // use the collision detection ʹ����ײ��⣨��ɫ�뻷����
}

bool bulletHit = CollisionDetector::CheckBulletCollision(bullet, character);
if (bulletHit) {
    // �����ӵ����н�ɫ���߼��������������ֵ��
}

Enigma::Ray bulletRay(bulletOrigin, bulletDirection);
for (const auto& target : targets) { // ����targets��һϵ��ģ��
    if (Enigma::RayIntersectsAABB(bulletRay, target.GetModelAABB())) {
        // ������ײ�߼��������������ֵ��  response the collision logic,like reduce health bar
    }
}
*/