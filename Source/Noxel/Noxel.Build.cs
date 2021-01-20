//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

using UnrealBuildTool;
using System.IO;



public class Noxel : ModuleRules
{

    public Noxel(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "JsonUtilities", "RuntimeMeshComponent", "UMG" });

        PrivateDependencyModuleNames.AddRange(new string[] {"Json", "RuntimeMeshComponent", "Slate", "SlateCore", "NoxelRenderer" });
    }
}
