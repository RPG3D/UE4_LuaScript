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

function MainTable:HandleUIEvent(InParam)
	print('OnUIEvent' .. InParam:GetX())

	--G_GameInstance:SetOnUIEvent(MainTable.d)
end

function MainTable.CustomHandle(InParam)
	print('CustomHandle')
	
end

function MainTable.PostInit()
	print('PostInit')
	
	MainTable.PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0)
	
	local UIClass = Unreal.LuaLoadClass(G_GameInstance, '/Game/MainUI.MainUI_C')
	
	MainTable.UIInst = WidgetBlueprintLibrary:Create(G_GameInstance, UIClass, MainTable.PlayerCtrl)
	MainTable.UIInst:AddToViewport(0)
	MainTable.PlayerCtrl:SetbShowMouseCursor(true)
	MainTable.DelegateObj = G_GameInstance:GetOnUIEvent():Bind(MainTable.HandleUIEvent, MainTable)

	G_GameInstance:SetOnUIEvent(G_GameInstance:GetOnUIEvent())
end

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	Unreal.RegisterTickFunction(MainTable.Tick)
	G_GameInstance = Unreal.LuaGetGameInstance()

	GTimer:SetTimer('PostInit', 2, 1, MainTable.PostInit, nil)

	local t = 'OnUIEvent__DelegateSignature'
	--MainTable.d = Unreal.LuaNewDelegate(t, "TestInstance")
	
	--MainTable.q = Unreal.LuaNewDelegate('OnButtonClickedEvent__DelegateSignature')
	
	--MainTable.d:Bind(MainTable.CustomHandle)
end




