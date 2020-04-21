--this file is the entry file for LuaScript plugin(UE4)


Timer = Timer or require('Timer')

G_Timer = Timer:new('GTimer')

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO('KismetSystemLibrary')

function LuaTick(InDeltaTime)
	G_Timer:Tick(InDeltaTime)
end

function Main()

print = Unreal.PrintLog

	print(("----Lua Ram: %.2fMB----"):format(collectgarbage("count") / 1024))
	G_Timer:SetTimer('MainDelayInit', 1, 0.1, DelayInit, nil)
end

function DelayInit()
	local co = coroutine.create(
		function()
			KismetSystemLibrary:Delay(GameInstance, 2.0)
			print(222)
		end
	)
	coroutine.resume(co)
	print(111)
end


