--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
KismetStringLibrary = KismetStringLibrary or Unreal.LuaGetUnrealCDO("KismetStringLibrary")
GameplayStatics = GameplayStatics or Unreal.LuaGetUnrealCDO("GameplayStatics")
BlueprintPathsLibrary = BlueprintPathsLibrary or Unreal.LuaGetUnrealCDO("BlueprintPathsLibrary")


AssetRegistryHelpers = AssetRegistryHelpers or Unreal.LuaGetUnrealCDO("AssetRegistryHelpers")

local Timer = require("Timer")

GTimer = Timer:new("GTimer")

inspect = require("inspect")


function Main()

	local val = AssetRegistryHelpers:GetAssetRegistry()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))

	--reference global c++ variable
    GameInstance = Unreal.LuaGetGameInstance()

	ProjectDir = BlueprintPathsLibrary:ProjectDir()

	local TestVector = KismetMathLibrary:MakeVector(123, 444, 23)
	print(TestVector:GetY())
	TestVector:SetY(41241)
	print(TestVector:GetY())

end

--hotfix, function only
function Hotfix(InModuleName)

	local OldModule = package.loaded[InModuleName]
	if(OldModule == nil) then
		print("hotfix failed, old module is nil: " .. InModuleName .. " all exist module:")
	end

	package.loaded[InModuleName] = nil
	package.preload[InModuleName] = nil
	local OK, m = pcall(require, InModuleName)
	if(OK == false)then
		package.loaded[InModuleName] = Oldmodule
		print("hotfix rollback")
		return
	end

	for k, v in pairs(package.loaded[InModuleName]) do
		if(type(v) == "function") then
			OldModule[k] = v
		end
	end

	package.loaded[InModuleName] = OldModule

	--fix UE4 delegate bind
	Unreal.LuaHotfixDelegateBind()

	print("Hotfix OK: " .. InModuleName)
	if(OldModule.OnHotfix) then
		OldModule.OnHotfix()
	end

	return OldModule

end

--global timer
function Unreal.Ticker(InDeltaTime)
    GTimer:Tick(InDeltaTime)
end

