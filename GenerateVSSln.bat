
set EngineBaseDir="D:\Program Files\Epic Games\UE_4.25\Engine"
SET CommandPath=%EngineBaseDir%"\Binaries\DotNET\UnrealBuildTool.exe"
SET CommandParam="-projectfiles "

FOR %%i IN (*.uproject) DO (
    %CommandPath% -projectfiles -project=%~dp0%%i
)


pause