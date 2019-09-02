--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO('GameplayStatics')

WidgetBlueprintLibrary = WidgetBlueprintLibrary or Unreal.LuaGetUnrealCDO('WidgetBlueprintLibrary')

PlayerController = PlayerController or Unreal.LuaGetUnrealCDO('PlayerController')

Timer = Timer or require('Timer')

GTimer = Timer:new('GTimer')

UEMath = UEMath or require('UEMath')

MainTable = MainTable or {}

function MainTable.Tick(InDeltaTime)
	GTimer:Tick(InDeltaTime)
end

cnt = 0
function MainTable.OnClicked(InParam)
	print(InParam:GetX())
	cnt = cnt + 1
	if cnt == 3 then
		G_GameInstance:SetOnUIEvent(MainTable.d)
	end
end

function MainTable.CustomHandle(InParam)
	print('CustomHandle')
	print(InParam:GetY())
	
end

function MainTable.PostInit()
	print('PostInit')
	
	MainTable.PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0)
	
	local UIClass = Unreal.LuaLoadClass(G_GameInstance, '/Game/MainUI.MainUI_C')
	
	MainTable.UIInst = WidgetBlueprintLibrary:Create(G_GameInstance, UIClass, MainTable.PlayerCtrl)
	MainTable.UIInst:AddToViewport(0)
	MainTable.PlayerCtrl:SetbShowMouseCursor(true)
	
	G_GameInstance:GetOnUIEvent():Bind(MainTable.OnClicked)

end

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	Unreal.RegisterTickFunction(MainTable.Tick)
	G_GameInstance = Unreal.LuaGetGameInstance()

	GTimer:SetTimer('PostInit', 2, 1, MainTable.PostInit, nil)

	MainTable.d = Unreal.LuaNewDelegate('OnUIEvent', "TestInstance", true)
	MainTable.d:Bind(MainTable.CustomHandle)
end




