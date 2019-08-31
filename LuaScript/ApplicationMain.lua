--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO('GameplayStatics')

PlayerController = PlayerController or Unreal.LuaGetUnrealCDO('PlayerController')


MainTable = MainTable or {}

function MainTable.HandleUIEvent()

end

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	local G_GameInstance = Unreal.LuaGetGameInstance()
	
	G_GameInstance:RunLuaCode('print(12321324)')
	
	local PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0) or PlayerController
	
	local btn = Unreal.LuaNewObject(PlayerCtrl, 'Button')
	local btnEvent = btn:Get_OnClicked()
	
	btnEvent:Bind('UniqueName', MainTable, 'HandleUIEvent')
	
	local img = Unreal.LuaNewObject(PlayerCtrl, 'Image')
	local imgEvent = img:Get_OnMouseButtonDownEvent()
	
	Unreal.RegisterTickFunction(MainTable.HandleUIEvent)
end


