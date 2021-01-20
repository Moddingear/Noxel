//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NoxelTarget : TargetRules
{
	public NoxelTarget(TargetInfo Target) : base(Target)
    {
        DefaultBuildSettings = BuildSettingsVersion.V2;
        Type = TargetType.Game;
        ExtraModuleNames.Add("Noxel");
        ExtraModuleNames.Add("NoxelRenderer");
        ExtraModuleNames.Add("NObjects");
    }
}
