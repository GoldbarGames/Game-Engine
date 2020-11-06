#ifndef MYFILEMANAGER_H
#define MYFILEMANAGER_H
#pragma once

#include "../ENGINE/FileManager.h"

class MyFileManager : public FileManager
{
public:
	void SaveFile(const std::string& filename) const;
	void LoadFile(const std::string& filename) const;
};

#endif