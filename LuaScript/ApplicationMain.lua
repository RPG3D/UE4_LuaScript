--this file is the entry file for LuaScript plugin(UE4)


Timer = Timer or require('Timer')

G_Timer = Timer:new('GTimer')

UEMath = UEMath or require('UEMath')

function G_LuaTick(InDeltaTime)
	G_Timer:Tick(InDeltaTime)
end

function PostInit()

	G_GameInstance = Unreal.LuaGetGameInstance()
	local PlayerCtrl = GameplayStatics:GetPlayerController(G_GameInstance, 0)
	
	print(tostring(PlayerCtrl:GetbShowMouseCursor()))
	PlayerCtrl:SetbShowMouseCursor(true)
	print(tostring(PlayerCtrl:GetbShowMouseCursor()))
end

function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	Unreal.RegisterTickFunction(G_LuaTick)
	
end




