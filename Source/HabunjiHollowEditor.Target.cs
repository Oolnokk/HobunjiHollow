// Copyright Habunji Hollow Team. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HabunjiHollowEditorTarget : TargetRules
{
	public HabunjiHollowEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("HabunjiHollow");
	}
}
