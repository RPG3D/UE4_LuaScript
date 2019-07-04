// Fill out your copyright notice in the Description page of Project Settings.

#include "TestInstance.h"
#include "Engine/Engine.h"
#include "UObjectGlobals.h"
#include "LuaUnrealWrapper.h"


FLuaUnrealWrapper* UTestInstance::LuaWrapper = nullptr;


void UTestInstance::Init()
{
	Super::Init();
}

void UTestInstance::Shutdown()
{
	Super::Shutdown();

	if (LuaWrapper)
	{
		LuaWrapper->Reset();
	}
}

void UTestInstance::OnStart()
{
	RunGameScript();
}

void UTestInstance::RunGameScript()
{
	if (LuaWrapper == nullptr)
	{
		LuaWrapper = FLuaUnrealWrapper::Create(this);
	}
	LuaWrapper->Init(this);

	LuaWrapper->RunMainFunction();
}

bool UTestInstance::RunLuaCode(const FString& InCode)
{
	FString Ret = LuaWrapper->DoLuaCode(InCode);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Ret);
	return true;
}