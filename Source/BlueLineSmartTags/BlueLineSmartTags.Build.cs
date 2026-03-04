// Copyright (c) 2026 GregOrigin. All Rights Reserved.

using UnrealBuildTool;

public class BlueLineSmartTags : ModuleRules
{
    public BlueLineSmartTags(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "BlueLineCore" // Runtime Core + Shared Styles
            }
        );
        
        // Redundant: BlueLineCore is already a PublicDependency, UBT resolves include paths automatically.
        // PublicIncludePaths.Add(System.IO.Path.Combine(PluginDirectory, "Source", "BlueLineCore", "Public"));

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "InputCore",
                "MainFrame",        // Required to bind commands to the editor mainframe
                "GraphEditor",      // Required for SGraphEditor access
                "BlueprintGraph",   // Required for UK2Node access
                "KismetCompiler",   // Required for some K2Node linkage
                "EditorFramework", // Required for modern UE5 editor styling
                "UnrealEd",        // Essential for Editor-only logic
                "PropertyEditor",  // Required to register IPropertyTypeCustomization
                // "EditorStyle", // Removed: deprecated since UE 5.1, code already uses FAppStyle
                "GameplayTagsEditor", // To interact with existing Tag UI logic
                "ToolMenus"        // Required for modern context menu extensions
            }
        );
    }
}
