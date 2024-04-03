#include "Enemy.h"

namespace Enigma {
	Enemy::Enemy(const std::string& filepath, const VulkanContext& context, int filetype) : Character(filepath, context, filetype)
	{
		
	}

	void Enemy::ManageAI(std::vector<Character*> character, Model* obj, Player* player)
	{
		if (player->moved) {
			updateNavmesh(character);
			for (int i = 0; i < character.size(); i++) {
				addToNavmesh(character[i], obj);
			}
			for (int i = 0; i < navmesh.vertices.size(); i++) {
				for (int j = 0; j < navmesh.edges[i].size(); j++) {
					printf("%i: %i\n", i, navmesh.edges[i][j].vertex2);
				}
			}
			printf("\n");
			pathToEnemy = findDirection();
			player->moved = false;
		}
		moveInDirection();
	}

	void Enemy::addToNavmesh(Character* character, Model* obj) {
		navmesh.vertices.push_back(character->translation);
		navmesh.edges.resize(navmesh.vertices.size());
		CollisionDetector cd;
		for (int node = 0; node < navmesh.vertices.size() - 1; node++) {
			for (int edge = 0; edge < navmesh.edges[node].size(); edge++) {
				if (navmesh.edges[node][edge].vertex2 == navmesh.vertices.size() - 1) {
					navmesh.edges[node].erase(navmesh.edges[node].begin() + edge);
				}
			}

			glm::vec3 direction = character->translation - navmesh.vertices[node];
			bool intersects = false;
			float weight = vec3Length(direction);
			Ray ray = Ray(character->translation, direction);

			for (int mesh = 0; mesh < obj->meshes.size(); mesh++) {
				if (obj->meshes[mesh].meshName != "Floor") {
					bool furtherAway = isMeshFurtherAway(direction, character->translation, obj->meshes[mesh].meshAABB);
					if (furtherAway == false) {
						intersects = cd.RayIntersectsAABB(ray, obj->meshes[mesh].meshAABB);
					}
					if (intersects == true) {
						break;
					}
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


	void Enemy::updateNavmesh(std::vector<Character*> character) {
		int numberOfAddedNodes = character.size();
		for (int i = 0; i < numberOfAddedNodes; i++) {
			navmesh.vertices.erase(navmesh.vertices.begin() + navmesh.numberOfBaseNodes + i - 1);
			navmesh.edges[navmesh.numberOfBaseNodes + i - 1].clear();
		}
	}

	std::vector<int> Enemy::findDirection() {
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
		std::vector<int> path;
		path.push_back(currentVertex);
		while (currentVertex != startVertex) {
			int nextVertex = graph.edgeFrom[currentVertex];
			path.push_back(nextVertex);
			if (nextVertex == startVertex) {
				break;
			}
			else {
				currentVertex = nextVertex;
			}
		}
		currentNode = 0;
		std::reverse(path.begin(), path.end());
		for (int i = 0; i < path.size(); i++) {
			printf("%i, ", path[i]);
		}
		printf("\n");

		return path;
	}

	void Enemy::moveInDirection() {
		glm::vec3 direction = navmesh.vertices[pathToEnemy[currentNode]] - this->translation;
		float distFromCurrentNode = vec3Length(direction);
		direction = glm::normalize(direction);
		if (distFromCurrentNode < 0.2f && currentNode < pathToEnemy.size()) {
			currentNode++;
		}
		else if(pathToEnemy[currentNode] != pathToEnemy.back()){
			this->setTranslation(this->translation + direction * 0.1f);
		}
		else if (distFromCurrentNode > 1.f) {
			this->setTranslation(this->translation + direction * 0.1f);
		}
	}

	bool Enemy::isMeshFurtherAway(glm::vec3 dir, glm::vec3 origin, AABB meshAABB) {
		glm::vec3 originMinDis = origin - meshAABB.min;
		glm::vec3 originMaxDis = origin - meshAABB.max;
		float minDisLen = vec3Length(originMinDis);
		float maxDisLen = vec3Length(originMaxDis);
		float dirLen = vec3Length(dir);
		if (dirLen > maxDisLen && dirLen > minDisLen) {
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