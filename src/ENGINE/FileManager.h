#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include "leak_check.h"
class Game;

class KINJO_API FileManager
{
public:
	mutable Game* game = nullptr;
	void Init(Game& g) const;
	virtual void SaveFile(const std::string& filename) const;
	virtual void LoadFile(const std::string& filename) const;
	virtual void AfterLoadLevelFromFile() const;
	virtual void SaveVariables() const;
	virtual void ResetVariables() const;
	bool FileExists(const std::string& path) const;
};

#endif