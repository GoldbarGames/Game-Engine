#include "FileManager.h"
#include "Game.h"

void FileManager::Init(Game& g) const
{
	game = &g;
}

void FileManager::ResetVariables() const
{

}

void FileManager::SaveVariables() const
{

}

void FileManager::SaveFile(const std::string& filename) const
{
	
}

void FileManager::LoadFile(const std::string& filename) const
{
	
}

// This function is called by the Game after the level is initialized
// to manipulate objects in the level such as the player's position
void FileManager::AfterLoadLevelFromFile() const
{

}

bool FileManager::FileExists(const std::string& path) const
{
#ifndef EMSCRIPTEN
    fs::path fullPath = fs::current_path() / path;
    return fs::exists(fullPath);
#else
    return false; // Emscripten build: no real filesystem access
#endif
}
