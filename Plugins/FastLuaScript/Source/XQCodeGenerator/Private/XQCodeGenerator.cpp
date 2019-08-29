// Fill out your copyright notice in the Description page of Project Settings.

#include "XQCodeGenerator.h"
#include "Modules/ModuleManager.h"

#include "ModuleManager.h"


#if WITH_EDITOR

#include "Framework/Commands/Commands.h"
#include "CoreStyle.h"
#include "UICommandList.h"
#include "LevelEditor.h"
#include "MultiBoxBuilder.h"

#include "GenerateLua.h"

#endif


#define LOCTEXT_NAMESPACE "FXQCodeGeneratorModule"

#if WITH_EDITOR

class FGenerateLuaCmd : public TCommands<FGenerateLuaCmd>
{
public:
	FGenerateLuaCmd()
		:TCommands<FGenerateLuaCmd>(TEXT("GenerateLua"), NSLOCTEXT("Contexts", "GenerateLua", "A lua interface for Unreal Engine"), NAME_None, FCoreStyle::Get().GetStyleSetName())
	{

	}

	virtual void RegisterCommands() override
	{
		UI_COMMAND(GenerateLua, "Generate Lua code", "Generate lua code", EUserInterfaceActionType::Button, FInputGesture());
	}

	TSharedPtr<FUICommandInfo> GenerateLua = nullptr;
};

#endif

void FXQCodeGeneratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FString Cmd = FCommandLine::Get();
	if (Cmd.Contains(FString("-server"), ESearchCase::IgnoreCase))
	{
		return;
	}

#if WITH_EDITOR

	FGenerateLuaCmd::Register();

	LuaScriptCmdList = MakeShareable(new FUICommandList);

	LuaScriptCmdList->MapAction(
		FGenerateLuaCmd::Get().GenerateLua,
		FExecuteAction::CreateRaw(this, &FXQCodeGeneratorModule::HandleGenerateLua),
		FCanExecuteAction()
	);

	FLevelEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, LuaScriptCmdList, FMenuExtensionDelegate::CreateRaw(this, &FXQCodeGeneratorModule::AddMenuExtension));
		EditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

#endif
}

void FXQCodeGeneratorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}


void FXQCodeGeneratorModule::HandleGenerateLua()
{
#if WITH_EDITOR
	GenerateLua Inst;
	Inst.InitConfig();
	Inst.GeneratedCode();
#endif
}


#if WITH_EDITOR
void FXQCodeGeneratorModule::AddMenuExtension(FMenuBuilder& InBuilder)
{
	InBuilder.AddMenuEntry(FGenerateLuaCmd::Get().GenerateLua);
}
#endif

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_GAME_MODULE(FXQCodeGeneratorModule, XQCodeGenerator);

