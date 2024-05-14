#include "Enemy.h"

float accum = 0;

namespace Enigma {
	Enemy::Enemy(const std::string& filepath, const VulkanContext& context, int filetype) : Character(filepath, context, filetype)
	{
		model->enemy = true;
	}

	Enemy::Enemy(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale) : Character(filepath, aContext, filetype, trans, scale)
	{
		model->enemy = true;
	}

	Enemy::Enemy(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale, float x, float y, float z) : Character(filepath, aContext, filetype, trans, scale, x, y, z)
	{
		model->enemy = true;
	}

	Enemy::Enemy(const std::string& filepath, const VulkanContext& aContext, int filetype, glm::vec3 trans, glm::vec3 scale, glm::mat4 rm) : Character(filepath, aContext, filetype, trans, scale, rm)
	{
		model->enemy = true;
	}

	void Enemy::ManageAI(std::vector<Character*> characters, Model* obj, Player* player)
	{
		//public class to manage AIs
		updateNavmesh(characters);
		addPlayerToNavmesh(player, obj);
		for (int i = 0; i < characters.size(); i++) {
			addToNavmesh(characters[i], obj);
		}
		pathToEnemy = findDirection(player);
		moveInDirection();
	}

	void Enemy::addPlayerToNavmesh(Player* player, Model* obj) {
		//add the characters position to the navmesh as a vertex
		player->navmeshPosition = navmesh.vertices.size();
		navmesh.vertices.push_back(glm::vec3(player->GetPosition().x, 0.1f, player->GetPosition().z));
		navmesh.edges.resize(navmesh.vertices.size());
		CollisionDetector cd;
		//loop through the other vertices of the mesh
		for (int node = 0; node < navmesh.vertices.size() - 1; node++) {
			//remove previous connections to the character node being added
			for (int edge = 0; edge < navmesh.edges[node].size(); edge++) {
				if (navmesh.edges[node][edge].vertex2 == navmesh.vertices.size() - 1) {
					navmesh.edges[node].erase(navmesh.edges[node].begin() + edge);
				}
			}

			//get information for ray from the added node to the current node in loop
			glm::vec3 direction = player->GetPosition() - navmesh.vertices[node];
			float weight = vec3Length(direction);
			Ray ray = Ray(player->GetPosition(), direction);
			collisionData colData;
			direction = normalize(direction);

			//check if the ray intersects any of the meshes
			for (int mesh = 0; mesh < obj->meshes.size(); mesh++) {
				if (obj->meshes[mesh].meshName != "Floor") {
					colData = cd.RayIntersectsAABB(ray, obj->meshes[mesh].meshAABB);
					if (colData.intersects == true && std::abs(colData.t) < weight) {
						break;
					}
				}
			}

			//if the ray does not intersect a mesh add that edge to the navmesh
			if (colData.intersects == false) {
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

	void Enemy::addToNavmesh(Character* character, Model* obj) {
		//add the characters position to the navmesh as a vertex
		character->navmeshPosition = navmesh.vertices.size();
		navmesh.vertices.push_back(character->getTranslation());
		navmesh.edges.resize(navmesh.vertices.size());
		CollisionDetector cd;
		//loop through the other vertices of the mesh
		for (int node = 0; node < navmesh.vertices.size() - 1; node++) {
			//remove previous connections to the character node being added
			for (int edge = 0; edge < navmesh.edges[node].size(); edge++) {
				if (navmesh.edges[node][edge].vertex2 == navmesh.vertices.size() - 1) {
					navmesh.edges[node].erase(navmesh.edges[node].begin() + edge);
				}
			}

			//get information for ray from the added node to the current node in loop
			glm::vec3 direction = character->getTranslation() - navmesh.vertices[node];
			float weight = vec3Length(direction);
			Ray ray = Ray(character->getTranslation(), direction);
			collisionData colData;
			direction = normalize(direction);

			//check if the ray intersects any of the meshes
			for (int mesh = 0; mesh < obj->meshes.size(); mesh++) {
				if (obj->meshes[mesh].meshName != "Floor") {
					colData = cd.RayIntersectsAABB(ray, obj->meshes[mesh].meshAABB);
					if (colData.intersects == true && std::abs(colData.t) < weight) {
						break;
					}
				}
			}

			//if the ray does not intersect a mesh add that edge to the navmesh
			if (colData.intersects == false) {
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


	void Enemy::updateNavmesh(std::vector<Character*> characters) {
		int numberOfAddedNodes = characters.size();
		for (int i = 0; i < numberOfAddedNodes; i++) {
			navmesh.vertices.erase(navmesh.vertices.begin() + navmesh.vertices.size() - 1);
		}
		navmesh.vertices.erase(navmesh.vertices.begin() + navmesh.vertices.size() - 1);
		navmesh.edges.resize(navmesh.vertices.size());
	}

	std::vector<int> Enemy::findDirection(Player* player) {
		int graphVertices = navmesh.vertices.size();
		int startVertex = this->navmeshPosition;
		int endVertex = player->navmeshPosition;
		std::vector<int> Visited;
		std::queue<int> toVisit;
		dijkstraData graph;

		//resize dijkstra's graph to have the same amount of vertices as navmesh
		graph.distance.resize(graphVertices);
		graph.edgeFrom.resize(graphVertices);
		//fill dijkstra's graphs distance with very large value to imitate infinity
		std::fill(graph.distance.begin(), graph.distance.end(), 1000000000.f);
		//make start vertex the current vertex
		int currentVertex = startVertex;
		//mark start vertex as visited
		Visited.push_back(startVertex);
		//make inital vertex have no distance
		graph.distance[currentVertex] = 0;
		graph.edgeFrom[currentVertex] = currentVertex;

		//add first vertex to visit so it enters the loop
		toVisit.push(currentVertex);
		while (!toVisit.empty()) {
			//make current vertex the first element in the stack
			currentVertex = toVisit.front();
			//loop through it's neighbours
			for (int i = 0; i < navmesh.edges[currentVertex].size(); i++) {
				//if the neighbour vertex hasn't been visted
				//get the distance and update the dijkstra's graphs data
				//add the neighbour to the toVisit stack
				if (notVisited(navmesh.edges[currentVertex][i].vertex2, Visited)) {
					float dist = navmesh.edges[currentVertex][i].weight + graph.distance[currentVertex];
					if (dist < graph.distance[navmesh.edges[currentVertex][i].vertex2]) {
						graph.edgeFrom[navmesh.edges[currentVertex][i].vertex2] = currentVertex;
						graph.distance[navmesh.edges[currentVertex][i].vertex2] = dist;
						toVisit.push(navmesh.edges[currentVertex][i].vertex2);
					}
				}
			}
			//remove the current vertex from toVisit and add to visited
			toVisit.pop();
			Visited.push_back(currentVertex);
		}

		//go backwards from the end vertex finding the shortest path to the start vertex
		//add the path of vertices to the path vector
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

		return path;
	}

	void Enemy::moveInDirection() {
		//get direction from enemy position to next vertex in path
		glm::vec3 direction = navmesh.vertices[pathToEnemy[currentNode]] - this->getTranslation();
		//get distance
		float distFromCurrentNode = vec3Length(direction);
		//normalize direction to the vertex
		direction = glm::normalize(direction);
		//if enemy is close to the next vertex update vertex in path
		//else move the enemy along the path
		glm::mat4 rm;
		if (distFromCurrentNode < 0.5f && currentNode < pathToEnemy.size()) {
			currentNode++;
			float angle = acos(direction.z) + asin(direction.x);
			rm = glm::inverse(glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f) - direction, glm::vec3(0.f, 1.f, 0.f)));
			this->setRotationMatrix(rm);
		}
		else if (pathToEnemy[currentNode] != pathToEnemy.back()) {
			this->setTranslation(this->getTranslation() + direction * 0.1f);
			rm = glm::inverse(glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f) - direction, glm::vec3(0.f, 1.f, 0.f)));
			this->setRotationMatrix(rm);
		}
		else if (distFromCurrentNode > 1.f) {
			this->setTranslation(this->getTranslation() + direction * 0.1f);
			rm = glm::inverse(glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f) - direction, glm::vec3(0.f, 1.f, 0.f)));
			this->setRotationMatrix(rm);
		}
		else {
			this->setTranslation(this->getTranslation());
			rm = glm::inverse(glm::lookAt(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f) - direction, glm::vec3(0.f, 1.f, 0.f)));
			this->setRotationMatrix(rm);
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