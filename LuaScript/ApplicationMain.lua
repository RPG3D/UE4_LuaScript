--this file is the entry file for LuaScript plugin(UE4)


--global UE4 singleton toolkit

KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")


function Main()

	print(("----------Lua Ram: %.2fMB----------"):format(collectgarbage("count") / 1024))
	
	KismetMathLibrary:MakeVector(1, 2, 3)
	
	local G_GameInstance = Unreal.LuaGetGameInstance()
	
	G_GameInstance:RunLuaCode(print(12321324))

end

