#ifndef RANDOMMANAGER_H
#define RANDOMMANAGER_H

#pragma once
class RandomManager
{
private:
	int randomSeed = 0;
public:
	void Seed(int seed);
	void Seed();
	int RandomInt(int max);
	int RandomRange(int min, int max);
	RandomManager();
	~RandomManager();
};

#endif