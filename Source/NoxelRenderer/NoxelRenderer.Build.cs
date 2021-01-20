//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

using UnrealBuildTool;

public class NoxelRenderer : ModuleRules
{
    public NoxelRenderer(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RuntimeMeshComponent"});
    }
}