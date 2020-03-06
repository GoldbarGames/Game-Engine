#include "globals.h"

std::string GetDrawingLayerName(DrawingLayer layer)
{
	switch (layer)
	{
	case BACK:
		return "BACK";
	case FRONT:
		return "FRONT";
	case OBJECT:
		return "OBJECT";
	case COLLISION:
		return "COLLISION";
	case COLLISION2:
		return "COLLISION2";
	case MIDDLE:
		return "MIDDLE";
	default:
		return "";
	}
}

