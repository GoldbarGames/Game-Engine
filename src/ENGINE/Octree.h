#ifndef OCTREE_H
#define OCTREE_H
#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "leak_check.h"

class Entity;

// 3D spatial partition - the perspective-camera counterpart of QuadTree.
// Rebuilt per frame (Reset + Insert), point-based insertion by entity
// position with per-node radius tracking so large entities near node
// boundaries are never missed by queries.
//
// Queries:
//   RetrieveSphere  - entities whose bounds intersect a sphere (collision,
//                     proximity, activation ranges)
//   RetrieveFrustum - entities visible in a camera frustum (render culling);
//                     pass camera.projection * camera.CalculateViewMatrix()
//
// Entity 3D bounds come from Entity::boundsRadius3D (world units; 0 treats
// the entity as a point).
class KINJO_API Octree
{
private:
	Octree* children[8];

	glm::vec3 center = glm::vec3(0, 0, 0);
	glm::vec3 halfSize = glm::vec3(0, 0, 0);
	int depth = 1;

	// Largest bounds radius in this node's subtree; queries expand node
	// tests by this so boundary-straddling entities are found
	float maxSubtreeRadius = 0.0f;

	void Split();
	int ChildIndexForPoint(const glm::vec3& p) const;
	bool SphereIntersectsNode(const glm::vec3& c, float r) const;

public:
	std::vector<Entity*> entities;

	size_t MAX_ENTITIES = 16;
	static const int MAX_DEPTH = 6;

	Octree();
	Octree(const glm::vec3& center, const glm::vec3& halfSize, int depth);
	~Octree();

	// Define the world region this tree covers (call before first Insert)
	void SetBounds(const glm::vec3& center, const glm::vec3& halfSize);

	void Reset();
	void Insert(Entity* newEntity);

	void RetrieveSphere(const glm::vec3& sphereCenter, float sphereRadius,
		std::vector<Entity*>& out) const;
	void RetrieveFrustum(const glm::mat4& viewProjection,
		std::vector<Entity*>& out) const;

	// Internal recursive step used by RetrieveFrustum (planes precomputed)
	void RetrieveFrustumPlanes(const glm::vec4* planes,
		std::vector<Entity*>& out) const;

	int CountEntities() const;
};

#endif
