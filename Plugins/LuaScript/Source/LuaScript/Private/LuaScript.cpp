// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LuaScript.h"
#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"

#if WITH_EDITOR

#include "Framework/Commands/Commands.h"
#include "CoreStyle.h"
#include "UICommandList.h"
#include "LevelEditor.h"
#include "MultiBoxBuilder.h"

#endif

#include "LuaUnrealWrapper.h"

#define LOCTEXT_NAMESPACE "FLuaScriptModule"

#if WITH_EDITOR

class FLuaScriptCmd : public TCommands<FLuaScriptCmd>
{
public:
	FLuaScriptCmd()
		:TCommands<FLuaScriptCmd>(TEXT("LuaScript"), NSLOCTEXT("Contexts", "LuaScript", "A lua interface for Unreal Engine"), NAME_None, FCoreStyle::Get().GetStyleSetName())
	{

	}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(ResetLua, "Reset Lua state", "restart lua state", EUserInterfaceActionType::Button, FInputGesture());
	}

	TSharedPtr<FUICommandInfo> ResetLua = nullptr;
};

#endif

void FLuaScriptModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FString Cmd = FCommandLine::Get();
	if (Cmd.Contains(FString("-server"), ESearchCase::IgnoreCase))
	{
		return;
	}

#if WITH_EDITOR

	FLuaScriptCmd::Register();

	LuaScriptCmdList = MakeShareable(new FUICommandList);

	LuaScriptCmdList->MapAction(
		FLuaScriptCmd::Get().ResetLua,
		FExecuteAction::CreateRaw(this, &FLuaScriptModule::OnResetlLua),
		FCanExecuteAction()
	);

	FLevelEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, LuaScriptCmdList, FMenuExtensionDelegate::CreateRaw(this, &FLuaScriptModule::AddMenuExtension));
		EditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

#endif
}

void FLuaScriptModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}


void FLuaScriptModule::OnResetlLua()
{
	OnRestartLuaWrapper.Broadcast();
}


#if WITH_EDITOR
void FLuaScriptModule::AddMenuExtension(FMenuBuilder& InBuilder)
{
	InBuilder.AddMenuEntry(FLuaScriptCmd::Get().ResetLua);
}
#endif

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLuaScriptModule, LuaScript)

DEFINE_LOG_CATEGORY(LogLuaScript);