//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class NoxelEditorTarget : TargetRules
{
	public NoxelEditorTarget(TargetInfo Target) : base(Target)
    {
	    DefaultBuildSettings = BuildSettingsVersion.V2;
		Type = TargetType.Editor;
        ExtraModuleNames.Add("Noxel");
        ExtraModuleNames.Add("NObjects");
        ExtraModuleNames.Add("NoxelRenderer");
    }
}
