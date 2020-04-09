

#pragma once


#include "CoreMinimal.h"

struct lua_State;

//use magic number to detect if the force convert is valid
enum class ELuaWrapperType
{
	Object = 2019,
	Struct,
	Delegate,
	Array,
	Map,
	Set,
	Interface
};


class ILuaWrapper
{
public:

	virtual ~ILuaWrapper()
	{

	}

};
