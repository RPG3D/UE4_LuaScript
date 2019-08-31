setlocal
set ANDROIDHOME=%ANDROID_HOME%
@rem if "%ANDROIDHOME%"=="" set ANDROIDHOME=D:/AndroidSDK
@rem set ADB=%ANDROIDHOME%\platform-tools\adb.exe
set ADB=%~dp0/../Tools/adb.exe

set DEVICE=
if not "%1"=="" set DEVICE=-s %1
for /f "delims=" %%A in ('%ADB% %DEVICE% shell "echo $EXTERNAL_STORAGE"') do @set STORAGE=%%A
@echo.


%ADB% %DEVICE% push LuaScript/. %STORAGE%/UE4Game/UE4_LuaScript/UE4_LuaScript/LuaScript

pause