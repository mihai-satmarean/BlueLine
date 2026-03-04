// Copyright (c) 2026 GregOrigin. All Rights Reserved.

using UnrealBuildTool;

public class BlueLineGraph : ModuleRules
{
    public BlueLineGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "BlueLineCore", // Runtime Logic + Shared Styles
                "GameplayTags"
            }
        );
        
        // Redundant: BlueLineCore is already a PublicDependency, UBT resolves include paths automatically.
        // PublicIncludePaths.Add(System.IO.Path.Combine(PluginDirectory, "Source", "BlueLineCore", "Public"));

        // Allow includes from BlueLineSmartTags without creating circular dependency
        PrivateIncludePathModuleNames.AddRange(
            new string[]
            {
                "BlueLineSmartTags"
            }
        );
        
        // Include path for FBlueLineSmartTagCommands.h (used in menu)
        PrivateIncludePaths.Add(System.IO.Path.Combine(PluginDirectory, "Source", "BlueLineSmartTags", "Public"));

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "InputCore",
                "EditorFramework",
                "UnrealEd",
                "GraphEditor",
                "BlueprintGraph",
                // "EditorStyle", // Removed: deprecated since UE 5.1, code already uses FAppStyle
                "KismetWidgets",
                "DeveloperSettings",
                "Projects",
                "Kismet",
                "ToolMenus",
                "ContentBrowser",
                "AssetTools",
                "DesktopPlatform",
                "GameplayTagsEditor", // <--- CRITICAL FIX: Required for Pin Picker Widget
                "Json"
            }
        );
    }
}
