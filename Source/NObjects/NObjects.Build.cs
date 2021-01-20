//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

using UnrealBuildTool;

public class NObjects : ModuleRules
{
    public NObjects(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "JsonUtilities", "RuntimeMeshComponent", "UMG", "Noxel" });

        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "RuntimeMeshComponent", "Slate", "SlateCore" });
    }
}