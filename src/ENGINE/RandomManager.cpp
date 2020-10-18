#include "RandomManager.h"
#include <stdlib.h>
#include <time.h>

RandomManager::RandomManager()
{

}

RandomManager::~RandomManager()
{

}

void RandomManager::Seed()
{
	randomSeed = (int)time(0);
	srand(randomSeed);
}

void RandomManager::Seed(int seed)
{
	randomSeed = seed;
	srand(randomSeed);
}

int RandomManager::RandomRange(int min, int max)
{
	return (rand() % max) + min;
}

int RandomManager::RandomInt(int max)
{
	return (rand() % max);
}