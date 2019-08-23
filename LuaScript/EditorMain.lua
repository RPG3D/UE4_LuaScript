--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")

local Timer = require("Timer")

GTimer = Timer:new("GTimer")

inspect = require("inspect")


function Main()

	print(("----------EditorLua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))

end

--global timer
function Unreal.Ticker(InDeltaTime)
    GTimer:Tick(InDeltaTime)
end

