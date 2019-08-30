--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO('GameplayStatics')

PlayerController = PlayerController or Unreal.LuaGetUnrealCDO('PlayerController')

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	local Vec = KismetMathLibrary:MakeVector(1, 2, 3)
	
	local G_GameInstance = Unreal.LuaGetGameInstance()
	
	G_GameInstance:RunLuaCode('print(12321324)')
	
	local PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0) or PlayerController
	print(Vec:Get_Y())
	Vec:Set_Z(222)
	PlayerCtrl:K2_SetActorLocation(Vec, false, nil, true)
	
	Tags = 
	{
		'lxq', 'unreal', 'game'
	}
	
	PlayerCtrl:Set_Tags(Tags)
	
	local NewTags = PlayerCtrl:Get_Tags()
	print(#NewTags)
	
end

