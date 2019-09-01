// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"


DECLARE_STATS_GROUP(TEXT("LuaScript"), STATGROUP_LuaScript, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("ObjectIndex"), STAT_ObjectIndex, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("ObjectNewIndex"), STAT_ObjectNewIndex, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("StructIndex"), STAT_StructIndex, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("StructNewIndex"), STAT_StructNewIndex, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("PushToLua"), STAT_PushToLua, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("FetchFromLua"), STAT_FetchFromLua, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("LuaCallBP"), STAT_LuaCallBP, STATGROUP_LuaScript, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("DelegateCallLua"), STAT_DelegateCallLua, STATGROUP_LuaScript, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("LuaTick"), STAT_LuaTick, STATGROUP_LuaScript, );

DECLARE_MEMORY_STAT_EXTERN(TEXT("LuaMemory"), STAT_LuaMemory, STATGROUP_LuaScript, )