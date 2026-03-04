// Copyright (c) 2026 GregOrigin. All Rights Reserved.

using UnrealBuildTool;

public class BlueLineCore : ModuleRules
{
    public BlueLineCore(ReadOnlyTargetRules Target) : base(Target)
    {
        // Use Shared PCHs for faster compilation
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",          // Needed for UDataAsset and Runtime Debug Drawing (HUD)
                "GameplayTags",
                "DeveloperSettings", // Required for UBlueLineEditorSettings
                "AssetRegistry"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "Projects",         // Required for IPluginManager (style system)
                "GraphEditor",      // Required for FGraphSelectionManager in BlueLineContextUtils
                "BlueprintGraph",   // Required for FBlueLineGraphAnalyzer (K2 nodes)
                "Json",             // Required for settings export/import
                "JsonUtilities"     // Required for JSON serialization
            }
        );
    }
}
