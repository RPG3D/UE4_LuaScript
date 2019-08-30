--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO('GameplayStatics')

PlayerController = PlayerController or Unreal.LuaGetUnrealCDO('PlayerController')

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	local G_GameInstance = Unreal.LuaGetGameInstance()
	
	G_GameInstance:RunLuaCode('print(12321324)')
	
	local PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0) or PlayerController
	
	local Plane = Unreal.LuaNewStruct('Plane')
	local x = Plane:Get_X()
	print(x)
	Plane:Set_X(1111)
	x = Plane:Get_X()
	print(x)
	
	print(Plane:Get_Z())
end


