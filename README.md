# LuaScript
Simple Is Power, both Runtime refelection & auto generate static bind code are supported!
note: add -FastLua to Visual Studio Command-Line, then F5, the static bind code will generated 

ignore generated code in version control!
git example:

    Plugins/FastLuaScript/Source/FastLuaScript/Generated/*

modify the Plugins/FastLuaScript/Config/ModuleToExport.txt to your game modules

## about
  LuaScript is unreal reflection based Lua API for UE4. All BlueprintCallable function & All UPROPERTY property & All Dynamic Delegate can be access in Lua.
  
  some global function, you can find in FastLuaHelper.h; example:
  
    GameInstance = Unreal.LuaGetGameInstance()
    MyUIBPClass = Unreal.LuaLoadClass(GameInstance, "/Game/UI/DebugUI.DebugUI_C")
    then call create widget instance and add to viewport
  
  Unreal.LuaGetUnrealCDO() is used for getting Class Default Object instance, example: 
  
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
      
all Unreal.LuaXXX functions, see FastLuaHelper.h

    Unreal.LuaGetGameInstance();
    Unreal.LuaLoadObject();
    Unreal.LuaLoadClass();
    Unreal.LuaGetUnrealCDO();//get unreal class default object
    Unreal.LuaNewObject();
    Unreal.LuaNewStruct();
    Unreal.LuaNewDelegate();
    Unreal.RegisterTickFunction();
	
more document will be added... if I have time.
    
