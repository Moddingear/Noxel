@echo off
rmdir /s /q .\Intermediate
rmdir /s /q .\Binaries
REM rmdir /s /q .\Plugins\RuntimeMeshComponent\Intermediate
REM rmdir /s /q .\Plugins\RuntimeMeshComponent\Binaries
timeout 1
D:/EpicGames/UE_4.22/Engine/Binaries/DotNET/UnrealBuildTool.exe Development Win64 -Project="E:/Documents/Unreal Projects/Noxel 4.22/Noxel.uproject" -TargetType=Editor -Progress -NoHotReloadFromIDE
timeout 1