#pragma once
#include"Common.h"
#include"Canvas.h"
#include<vector>
class SaveLoad
{	
public:
	bool Save(const wchar_t* path, const Canvas& canvas);
	bool Load(const wchar_t* path, Canvas& canvas);
};

