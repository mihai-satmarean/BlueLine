// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * FBlueLineStyle
 * 
 * Manages the Slate styling resources (Icons, Brushes, Fonts) for the BlueLine plugin.
 * Located in BlueLineCore so all modules (Graph, SmartTags, Level) can access it.
 * 
 * Usage:
 * const FSlateBrush* Icon = FBlueLineStyle::Get().GetBrush("BlueLine.AutoFormat");
 */
class BLUELINECORE_API FBlueLineStyle
{
public:

	/** Register the style set and load resources */
	static void Initialize();

	/** Unregister the style set and free memory */
	static void Shutdown();

	/** Reloads textures from disk (Useful for iteration) */
	static void ReloadTextures();

	/** Access the singleton instance */
	static const ISlateStyle& Get();

	/** Access the singleton name */
	static FName GetStyleSetName();

private:

	/** Creates the style set definition */
	static TSharedRef<class FSlateStyleSet> Create();

	/** Singleton instance */
	static TSharedPtr<class FSlateStyleSet> StyleInstance;
};
