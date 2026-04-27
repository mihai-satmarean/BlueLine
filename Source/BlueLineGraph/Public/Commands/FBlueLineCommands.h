// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

/**
 * FBlueLineCommands
 * 
 * Defines the keyboard shortcuts and menu actions for the BlueLine Graph module.
 * 
 * To remap these in the Editor:
 * Go to Editor Preferences -> Keyboard Shortcuts -> BlueLine Graph
 */
class FBlueLineCommands : public TCommands<FBlueLineCommands>
{
public:
	/** Constructor: Sets up the context name and parent style */
	FBlueLineCommands()
		: TCommands<FBlueLineCommands>(
			TEXT("BlueLineGraph"), // Context name for keybindings
			NSLOCTEXT("Contexts", "BlueLineGraph", "BlueLine Graph"), // User facing name
			NAME_None, // Parent context
			FAppStyle::GetAppStyleSetName() // Icon style set
		)
	{}

	/** Register commands with the system */
	virtual void RegisterCommands() override;

	/**
	 * Command: Auto-Format Selected
	 * Feature: The "Soft" formatter.
	 * Action: Aligns selected nodes to the grid relative to their inputs/outputs.
	 * Solves: The "Diff War" weakness (only moves what you touch).
	 */
	TSharedPtr<FUICommandInfo> AutoFormatSelected; // Shift + Q
	TSharedPtr<FUICommandInfo> RigidifyConnections; // Shift + R
	TSharedPtr<FUICommandInfo> CleanGraph; // Shift + C
	// Note: AutoTagGraph command is defined in BlueLineSmartTags module to avoid duplication

	/**
	 * Command: Toggle Wire Style
	 * Feature: Instantly switch between Manhattan (BlueLine) and Spline (Vanilla) wires.
	 * Useful for debugging or if the "Circuit Board" look is confusing in a specific messy graph.
	 * Key: Shift+W (Changed from F8 to avoid conflict with Engine's "Possess or Eject Player")
	 */
	TSharedPtr<FUICommandInfo> ToggleWireStyle;

	// -------------------------------------------------------------------------
	// Multi-mode formatter (ported from Auto Node Arranger algorithm)
	// -------------------------------------------------------------------------

	/**
	 * Format Straight (Shift+X)
	 * Top-aligns all selected nodes in the exec chain to their primary parent.
	 * Best for linear event-driven chains.
	 */
	TSharedPtr<FUICommandInfo> FormatStraight;

	/**
	 * Format Center (Shift+V)
	 * Vertically centers each node on the connecting pin of its parent.
	 * Best for data-heavy graphs with parallel branches.
	 */
	TSharedPtr<FUICommandInfo> FormatCenter;

	/**
	 * Format Compact (Shift+B)
	 * Exec nodes keep their Y; data feeder nodes stack vertically per column.
	 * Best for dense graphs where space is at a premium.
	 */
	TSharedPtr<FUICommandInfo> FormatCompact;

	/**
	 * Select Connected Graph (Shift+F)
	 * BFS expansion from the current selection via all pin connections.
	 * Selects everything reachable. Equivalent to ANA's "Select Connected Graph".
	 */
	TSharedPtr<FUICommandInfo> SelectConnectedGraph;
};
