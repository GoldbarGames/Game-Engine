#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>

class Game;

class FileManager
{
public:
	mutable Game* game = nullptr;
	void Init(Game& g) const;
	virtual void SaveFile(const std::string& filename) const;
	virtual void LoadFile(const std::string& filename) const;
};

#endif