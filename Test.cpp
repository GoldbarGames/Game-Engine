#include "Test.h"

#include <string>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include <memory>
#include <iostream>
#include "ImageDeleter.h"

using namespace std;

Test::Test()
{
}


Test::~Test()
{
}

void Test::RunTest()
{
	std::vector<std::string> names{ "assets/sprites/wdk_walk.png", "assets/bg/bg.png" };

	unordered_map<string, unique_ptr<SDL_Surface, ImageDeleter>> images;

	for (auto name : names)
	{
		unique_ptr<SDL_Surface, ImageDeleter> img{ IMG_Load(name.c_str()) };

		if (img.get() == NULL)
		{
			cout << "ERROR Loading Image " << name << endl;
		}
		else
		{
			// a unique_ptr cannot be copied so it must be moved instead
			images[name] = move(img);
		}
	}

	cout << "Loaded " << images.size() << " Images" << endl;
}


