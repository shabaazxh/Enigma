#include "Enemy.h"

namespace Enigma {
	Enemy::Enemy(const std::string& filepath, const VulkanContext& context, int filetype) : Character(filepath, context, filetype)
	{
		
	}

	void Enemy::ManageAI(Character* character, Model* obj)
	{
		addToNavmesh(character, obj);
		addToNavmesh(this, obj);
		glm::vec3 dir = findDirection();
		moveInDirection(dir);
	}

	void Enemy::addToNavmesh(Character* character, Model* obj) {
		navmesh.vertices.push_back(character->translation);
		navmesh.edges.resize(navmesh.vertices.size());
		CollisionDetector cd;
		for (int node = 0; node < navmesh.vertices.size() - 1; node++) {
			glm::vec3 direction = character->translation - navmesh.vertices[node];
			bool intersects = false;
			float weight = vec3Length(direction);
			Ray ray = Ray(character->translation, direction);

			for (int mesh = 0; mesh < obj->meshes.size(); mesh++) {
				bool furtherAway = isMeshFurtherAway(direction, character->translation, obj->meshes[mesh].meshAABB);
				if(furtherAway == false) {
					intersects = cd.RayIntersectsAABB(ray, obj->meshes[mesh].meshAABB);
				}
				if (intersects == true) {
					break;
				}
			}

			if (intersects == false) {
				Edge edge1;
				Edge edge2;
				edge1.vertex2 = node;
				edge2.vertex2 = navmesh.vertices.size() - 1;
				edge1.weight = weight;
				edge2.weight = weight;
				navmesh.edges[navmesh.vertices.size() - 1].push_back(edge1);
				navmesh.edges[node].push_back(edge2);
			}
		}
	}

	glm::vec3 Enemy::findDirection() {
		int graphVertices = navmesh.vertices.size();
		int startVertex = graphVertices - 1;
		int endVertex = graphVertices - 2;
		std::vector<int> Visited;
		std::queue<int> toVisit;
		dijkstraData graph;
		graph.distance.resize(graphVertices);
		graph.edgeFrom.resize(graphVertices);
		std::fill(graph.distance.begin(), graph.distance.end(), 1000000000.f);
		int currentVertex = startVertex;
		Visited.push_back(startVertex);
		graph.distance[currentVertex] = 0;
		graph.edgeFrom[currentVertex] = currentVertex;

		toVisit.push(currentVertex);
		while (!toVisit.empty()) {
			currentVertex = toVisit.front();
			for (int i = 0; i < navmesh.edges[currentVertex].size(); i++) {
				if (notVisited(navmesh.edges[currentVertex][i].vertex2, Visited)) {
					float dist = navmesh.edges[currentVertex][i].weight + graph.distance[currentVertex];
					if (dist < graph.distance[navmesh.edges[currentVertex][i].vertex2]) {
						graph.edgeFrom[navmesh.edges[currentVertex][i].vertex2] = currentVertex;
						graph.distance[navmesh.edges[currentVertex][i].vertex2] = dist;
						toVisit.push(navmesh.edges[currentVertex][i].vertex2);
					}
				}
			}
			toVisit.pop();
			Visited.push_back(currentVertex);
		}

		currentVertex = endVertex;
		while (currentVertex != startVertex) {
			int nextVertex = graph.edgeFrom[currentVertex];
			if (nextVertex == startVertex) {
				break;
			}
			else {
				currentVertex = nextVertex;
			}
		}

		glm::vec3 direction = navmesh.vertices[startVertex] - navmesh.vertices[currentVertex];

		return direction;
	}

	void Enemy::moveInDirection(glm::vec3 dir) {
		this->setTranslation(this->translation + (dir * 0.001f));
	}

	bool Enemy::isMeshFurtherAway(glm::vec3 dir, glm::vec3 origin, AABB meshAABB) {
		glm::vec3 originMinDis = origin - meshAABB.min;
		glm::vec3 originMaxDis = origin - meshAABB.max;
		if ((dir.x < originMinDis.x && dir.x < originMaxDis.x) ||
			(dir.y < originMinDis.y && dir.y < originMaxDis.y) ||
			(dir.z < originMinDis.z && dir.z < originMaxDis.z)
			) {
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Enemy::notVisited(int node, std::vector<int> visited) {
		for (int i = 0; i < visited.size(); i++) {
			if (visited[i] == node) {
				return false;
			}
		}
		return true;
	}
}