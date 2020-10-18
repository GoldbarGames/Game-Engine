#ifndef MYENTITYFACTORY_H
#define MYENTITYFACTORY_H
#pragma once

#include "../ENGINE/EntityFactory.h"

class MyEntityFactory : public EntityFactory
{
public:
	MyEntityFactory();
    static MyEntityFactory* Get()
    {
        static MyEntityFactory instance;
        return &instance;
    }
};

#endif