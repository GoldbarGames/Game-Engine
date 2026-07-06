#include "Octree.h"
#include "Entity.h"
#include "globals.h"
#include <cmath>

Octree::Octree()
{
	for (int i = 0; i < 8; i++)
		children[i] = nullptr;
}

Octree::Octree(const glm::vec3& c, const glm::vec3& hs, int d)
{
	for (int i = 0; i < 8; i++)
		children[i] = nullptr;
	center = c;
	halfSize = hs;
	depth = d;
}

Octree::~Octree()
{
	Reset();
}

void Octree::SetBounds(const glm::vec3& c, const glm::vec3& hs)
{
	center = c;
	halfSize = hs;
}

void Octree::Reset()
{
	entities.clear();
	maxSubtreeRadius = 0.0f;
	for (int i = 0; i < 8; i++)
	{
		if (children[i] != nullptr)
		{
			delete_it(children[i]);
		}
	}
}

int Octree::ChildIndexForPoint(const glm::vec3& p) const
{
	int index = 0;
	if (p.x >= center.x) index |= 1;
	if (p.y >= center.y) index |= 2;
	if (p.z >= center.z) index |= 4;
	return index;
}

void Octree::Split()
{
	glm::vec3 quarter = halfSize * 0.5f;
	for (int i = 0; i < 8; i++)
	{
		glm::vec3 offset(
			(i & 1) ? quarter.x : -quarter.x,
			(i & 2) ? quarter.y : -quarter.y,
			(i & 4) ? quarter.z : -quarter.z);
		children[i] = new Octree(center + offset, quarter, depth + 1);
	}
}

void Octree::Insert(Entity* newEntity)
{
	float r = newEntity->boundsRadius3D;
	if (r > maxSubtreeRadius)
		maxSubtreeRadius = r;

	// Leaf with room (or maximum depth): keep it here
	if (children[0] == nullptr)
	{
		if (entities.size() < MAX_ENTITIES || depth >= MAX_DEPTH)
		{
			entities.push_back(newEntity);
			return;
		}

		// Split and redistribute
		Split();
		std::vector<Entity*> old = entities;
		entities.clear();
		for (Entity* e : old)
		{
			children[ChildIndexForPoint(e->position)]->Insert(e);
		}
	}

	children[ChildIndexForPoint(newEntity->position)]->Insert(newEntity);
}

bool Octree::SphereIntersectsNode(const glm::vec3& c, float r) const
{
	// Sphere vs AABB, with the AABB expanded by the largest entity radius
	// stored beneath this node (so boundary-straddlers are not skipped)
	float expand = r + maxSubtreeRadius;
	glm::vec3 d(0, 0, 0);
	for (int axis = 0; axis < 3; axis++)
	{
		float dist = fabsf(c[axis] - center[axis]) - halfSize[axis];
		if (dist > 0) d[axis] = dist;
	}
	return (d.x * d.x + d.y * d.y + d.z * d.z) <= expand * expand;
}

void Octree::RetrieveSphere(const glm::vec3& sphereCenter, float sphereRadius,
	std::vector<Entity*>& out) const
{
	if (!SphereIntersectsNode(sphereCenter, sphereRadius))
		return;

	for (Entity* e : entities)
	{
		glm::vec3 diff = e->position - sphereCenter;
		float reach = sphereRadius + e->boundsRadius3D;
		if (glm::dot(diff, diff) <= reach * reach)
			out.push_back(e);
	}

	if (children[0] != nullptr)
	{
		for (int i = 0; i < 8; i++)
			children[i]->RetrieveSphere(sphereCenter, sphereRadius, out);
	}
}

void Octree::RetrieveFrustum(const glm::mat4& vp, std::vector<Entity*>& out) const
{
	// Gribb-Hartmann plane extraction (works with any projection,
	// including the engine's Y-flipped one). Plane = (a,b,c,d) with
	// a*x + b*y + c*z + d >= 0 meaning inside.
	glm::vec4 planes[6];
	planes[0] = glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]); // left
	planes[1] = glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]); // right
	planes[2] = glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]); // bottom
	planes[3] = glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]); // top
	planes[4] = glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]); // near
	planes[5] = glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]); // far

	for (int i = 0; i < 6; i++)
	{
		float len = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
		if (len > 0.0f)
			planes[i] /= len;
	}

	RetrieveFrustumPlanes(planes, out);
}

void Octree::RetrieveFrustumPlanes(const glm::vec4* planes,
	std::vector<Entity*>& out) const
{
	// Node AABB (expanded by subtree max radius) vs frustum: conservative
	// p-vertex test; reject only when fully outside a plane
	for (int p = 0; p < 6; p++)
	{
		glm::vec3 n(planes[p]);
		float e = halfSize.x * fabsf(n.x) + halfSize.y * fabsf(n.y) + halfSize.z * fabsf(n.z);
		float s = glm::dot(n, center) + planes[p].w;
		if (s + e + maxSubtreeRadius < 0)
			return;  // whole node (plus its largest entity) outside this plane
	}

	for (Entity* e : entities)
	{
		bool inside = true;
		for (int p = 0; p < 6; p++)
		{
			float dist = glm::dot(glm::vec3(planes[p]), e->position) + planes[p].w;
			if (dist < -e->boundsRadius3D)
			{
				inside = false;
				break;
			}
		}
		if (inside)
			out.push_back(e);
	}

	if (children[0] != nullptr)
	{
		for (int i = 0; i < 8; i++)
			children[i]->RetrieveFrustumPlanes(planes, out);
	}
}

int Octree::CountEntities() const
{
	int count = (int)entities.size();
	if (children[0] != nullptr)
	{
		for (int i = 0; i < 8; i++)
			count += children[i]->CountEntities();
	}
	return count;
}
