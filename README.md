# LuaScript
Simple Is Power, Call UE4 Function Via Runtime Refelection.
note: add -FastLua to Visual Studio Command-Line, then F5, the static bind code will generated 


## about
  
  some global function in FastLuaHelper.h; example:
  
    GameInstance = Unreal.LuaGetGameInstance()
    MyUIBPClass = Unreal.LuaLoadClass(GameInstance, "/Game/UI/DebugUI.DebugUI_C")
    then call create widget instance and add to viewport
  
  Unreal.LuaGetUnrealCDO() is used for getting Class Default Object instance, example: 
  
    KismetSystemLibrary = KismetSystemLibrary or Unreal.LuaGetUnrealCDO("KismetSystemLibrary")
    
    KismetSystemLibrary:GetDisplayName(GameInstance)
    
   
 for delegate:
 
     GameUIHandler = {} 
     
     function GameUIHandler:OnStartGame()
         print('StartGame')
     end
     
    local MyBtn = MyUMGHelper:FindWidgetInUMG(DebugUIInstance, "TestBtn")
    --param is: lua function, lua table[option]
    MyBtn.OnClicked:Bind(GameUIHandler.OnStartGame, GameUIHandler)
    
 for struct 
 
    KismetMathLibrary = KismetMathLibrary or Unreal.LuaGetUnrealCDO("KismetMathLibrary")
    local TestVector = KismetMathLibrary:MakeVector(123, 444, 23)
	  print(TestVector.Y)
	  TestVector.Y = 41241
	  print(TestVector.Y)
	  
    or:(not Hight Performance!, but useful when no c++ function:Make_SomeStruct())
    local TestVec = Unreal.LuaNewStruct("Vector")
      
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
    
