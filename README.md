# LuaScript
Simple Is Power, both Runtime reflectin & auto generate static bind code are supported!
note: add -FastLua to Visual Studio Command-Line, then F5, the static bind code will generated 
## about
  LuaScript is unreal runtime reflection based Lua API for UE4. All BlueprintCallable function & All UPROPERTY property & All Dynamic Delegate can be access in Lua.
  
  some global function, you can find in UnrealMisc.h; example:
  
    GameInstance = Unreal.LuaGetGameInstance()
    MyUIBPClass = Unreal.LuaLoadClass(GameInstance, "/Game/UI/DebugUI.DebugUI_C")
    then call create widget instance and add to viewport
  
  Unreal.LuaGetUnrealCDO() is used for get Class Default Object instance, example: 
  
    KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
    
    KismetSystemLibrary:GetDisplayName(GameInstance)
    
   
 for delegate:
 
     GameUIHandler = {}
     
     function GameUIHandler.OnStartGame()
         print('StartGame')
     end
 
    local MyBtn = MyUMGHelper:FindWidgetInUMG(DebugUIInstance, "TestBtn")
    --param is: unique name, lua table, function name
    MyBtn:GetOnClicked():Bind("UniqueNameUsedForUnbind", GameUIHandler, 'OnStartGame')
    
 for struct 
 
    KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
    local TestVector = KismetMathLibrary:MakeVector(123, 444, 23)
	  print(TestVector:GetY())
	  TestVector:SetY(41241)
	  print(TestVector:GetY())
      
      
more document will be added... if I have time.
    
