// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

class FLuaScriptModule : public IModuleInterface
{
public:

	DECLARE_MULTICAST_DELEGATE(FOnRestartLuaWrapper);

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	FOnRestartLuaWrapper OnRestartLuaWrapper;
protected:
	void OnResetlLua();

#if WITH_EDITOR
	void AddMenuExtension(class FMenuBuilder& InBuilder);
#endif

	TSharedPtr<class FUICommandList> LuaScriptCmdList = nullptr;
};


DECLARE_LOG_CATEGORY_EXTERN(LogLuaScript, Log, All);