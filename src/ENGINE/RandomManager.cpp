#include "RandomManager.h"
#include <stdlib.h>
#include <time.h>

RandomManager::RandomManager()
{
	rng = std::default_random_engine();
}

RandomManager::~RandomManager()
{

}

void RandomManager::Seed()
{
	randomSeed = (int)time(0);
	srand(randomSeed);
	rng = std::default_random_engine{ (unsigned int)randomSeed };
}

void RandomManager::Seed(int seed)
{
	randomSeed = seed;
	srand(randomSeed);
}

int RandomManager::RandomRange(int min, int max)
{
	return (rand() % (max-min)) + min;
}

int RandomManager::RandomInt(int max)
{
	return (rand() % max);
}