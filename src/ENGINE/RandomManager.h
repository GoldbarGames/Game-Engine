#ifndef RANDOMMANAGER_H
#define RANDOMMANAGER_H
#pragma once

#include "leak_check.h"
#include <vector>
#include <algorithm>
#include <random>

class KINJO_API RandomManager
{
private:
	std::default_random_engine rng;
	int randomSeed = 0;
public:
	void Seed(int seed);
	void Seed();
	int RandomInt(int max);
	int RandomRange(int min, int max);
	RandomManager();
	~RandomManager();

	template <typename T, typename A> void Shuffle(std::vector<T, A>& a)
	{
		std::shuffle(a.begin(), a.end(), rng);
		return;
	}
};

#endif