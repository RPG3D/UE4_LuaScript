// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealMisc.h"
#include "LuaUnrealWrapper.h"
#include "PlatformFilemanager.h"
#include "AutomationTest.h"


static const int TestFlags = EAutomationTestFlags::ClientContext | EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;


DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FLuaUnrealWrapperLatentCommand, int32, InParam);
bool FLuaUnrealWrapperLatentCommand::Update()
{
	bool bTestComplete = true;
	if (InParam < 9999)
	{
		InParam++;
		bTestComplete = false;
	}
	return bTestComplete;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLuaUnrealWrapperTest, "LuaScriptPluginTest", TestFlags)


bool FLuaUnrealWrapperTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FLuaUnrealWrapperLatentCommand(123));
	return true;
}