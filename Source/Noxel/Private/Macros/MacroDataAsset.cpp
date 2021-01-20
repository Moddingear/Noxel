//Copyright 2016-2020 Gabriel Zerbib (Moddingear). All rights reserved.

#include "Macros/MacroDataAsset.h"

TArray<FMacroInfo> UMacroDataAsset::getMacros()
{
	TArray<FMacroInfo> ret;
	for (int i = 0; i < Macros.Num(); i++)
	{
		if (Macros[i].editorOnly) {
			#if !WITH_EDITOR
				continue;
			#endif
		}
		ret.Add(Macros[i]);
	}
	return ret;
}
