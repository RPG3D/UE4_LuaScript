--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")

MapProcessAPI = MapProcessAPI or Unreal.LuaGetUnrealCDO("MapProcessAPI")

local Timer = require("Timer")

GTimer = Timer:new("GTimer")

inspect = require("inspect")

RestartInterval = 15

MapProcessAPI = MapProcessAPI or Unreal.LuaGetUnrealCDO('MapProcessAPI')
function PostEditorInit()
	MapProcessAPI:RestartLuaDelay(RestartInterval)
end

function Main()

	print(("----------EditorLua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	Unreal.RegisterTickFunction(LuaTick)
	
	--每隔RestartInterval秒重启Lua
	print('每隔' .. tostring(RestartInterval) .. '秒将会重启Lua')
	GTimer:SetTimer('PostEditorInit', RestartInterval, 1, PostEditorInit, nil)
	
	local NewData = Unreal.LuaNewStruct('Matrix')
	local XPlane = NewData:GetXPlane()
	local W = XPlane:GetW()
	
end

--global timer
function LuaTick(InDeltaTime)
    GTimer:Tick(InDeltaTime)
end

