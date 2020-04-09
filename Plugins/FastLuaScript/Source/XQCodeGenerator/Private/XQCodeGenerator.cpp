// Fill out your copyright notice in the Description page of Project Settings.

#include "XQCodeGenerator.h"
#include "Modules/ModuleManager.h"


#if WITH_EDITOR

#include "Framework/Commands/Commands.h"
#include "Styling/CoreStyle.h"
#include "Framework/Commands/UICommandList.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Interfaces/IMainFrameModule.h"

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


static void OnMainFrameInitCompleted(TSharedPtr<SWindow> InWin, bool InShowOpenProject)
{
	FXQCodeGeneratorModule* TempModule = (FXQCodeGeneratorModule*)FModuleManager::Get().GetModule(FName("XQCodeGenerator"));
	if (TempModule == nullptr)
	{
		return;
	}

	TempModule->HandleGenerateLua();
}

void FXQCodeGeneratorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FString Cmd = FCommandLine::Get();
	if (Cmd.Contains(FString("-server"), ESearchCase::IgnoreCase))
	{
		return;
	}

#if WITH_EDITOR
	if (Cmd.Contains(FString("-FastLua"), ESearchCase::IgnoreCase))
	{
		IMainFrameModule::Get().OnMainFrameCreationFinished().AddStatic(OnMainFrameInitCompleted);
	}

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

	FString Cmd = FCommandLine::Get();

	GenerateLua Inst;
	Inst.InitConfig();

	FString GenerateOneClass;
	FParse::Value(*Cmd, *FString("-OneClass="), GenerateOneClass, true);
	if (GenerateOneClass.IsEmpty())
	{
		Inst.GeneratedCode();
	}
	else
	{
		UStruct* Cls = FindObject<UStruct>(ANY_PACKAGE, *GenerateOneClass);
		if (UClass* IsCls = Cast<UClass>(Cls))
		{
			Inst.GenerateCodeForClass(IsCls);
		}
		
	}

	if (Cmd.Contains(FString("-FastLua"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("Generate lua code completed!, editor is exiting..."));
		FGenericPlatformMisc::RequestExit(false);
	}
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

