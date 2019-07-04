// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"


DECLARE_STATS_GROUP(TEXT("LuaScript"), STATGROUP_LuaScript, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("FindClassMetatable"), STAT_FindClassMetatable, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("FindStructMetatable"), STAT_FindStructMetatable, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("PushToLua"), STAT_PushToLua, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("FetchFromLua"), STAT_FetchFromLua, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("LuaCallBP"), STAT_LuaCallBP, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("DelegateCallLua"), STAT_DelegateCallLua, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("LuaTick"), STAT_LuaTick, STATGROUP_LuaScript, );

DECLARE_MEMORY_STAT_EXTERN(TEXT("LuaMemory"), STAT_LuaMemory, STATGROUP_LuaScript, )