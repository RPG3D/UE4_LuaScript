--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO('GameplayStatics')

WidgetBlueprintLibrary = WidgetBlueprintLibrary or Unreal.LuaGetUnrealCDO('WidgetBlueprintLibrary')

PlayerController = PlayerController or Unreal.LuaGetUnrealCDO('PlayerController')

Timer = Timer or require('Timer')

GTimer = Timer:new('GTimer')

MainTable = MainTable or {}

function MainTable.Tick(InDeltaTime)
	GTimer:Tick(InDeltaTime)
end

function MainTable.UnbindEvent()
	

end


function MainTable:HandleUIEvent(InParam)
	print('OnUIEvent')

	print(InParam:GetX(), InParam:GetY())
	
	--G_GameInstance:Get_OnUIEvent():Unbind(MainTable.DelegateObj)
	MainTable.DelegateObj:Unbind(MainTable.DelegateObj)
end

function MainTable.PostInit()
	print('PostInit')
	
	local PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0)
	
	local UIClass = Unreal.LuaLoadClass(G_GameInstance, '/Game/MainUI.MainUI_C')
	
	MainTable.UIInst = WidgetBlueprintLibrary:Create(G_GameInstance, UIClass, PlayerCtrl)
	MainTable.UIInst:AddToViewport(0)
	
	MainTable.DelegateObj = G_GameInstance:GetOnUIEvent():Bind(MainTable.HandleUIEvent, MainTable)
end

function Main()


	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	Unreal.RegisterTickFunction(MainTable.Tick)
	
	G_GameInstance = Unreal.LuaGetGameInstance()

	GTimer:SetTimer('PostInit', 3, 1, MainTable.PostInit, nil)
end




