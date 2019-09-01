// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FXQCodeGeneratorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void HandleGenerateLua();
protected:


#if WITH_EDITOR
	void AddMenuExtension(class FMenuBuilder& InBuilder);

	TSharedPtr<class FUICommandList> LuaScriptCmdList = nullptr;
#endif

};