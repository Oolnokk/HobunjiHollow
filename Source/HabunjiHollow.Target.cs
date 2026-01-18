// Copyright Habunji Hollow Team. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HabunjiHollowTarget : TargetRules
{
	public HabunjiHollowTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("HabunjiHollow");
	}
}
