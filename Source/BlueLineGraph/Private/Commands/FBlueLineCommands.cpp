// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Commands/FBlueLineCommands.h"
#include "Styles/FBlueLineStyle.h"  // From BlueLineCore

#define LOCTEXT_NAMESPACE "BlueLineGraph"

void FBlueLineCommands::RegisterCommands()
{
	UI_COMMAND(
		AutoFormatSelected, 
		"Soft Align (Magnet)", 
		"Aligns the selected nodes grid-relative to their input connections. Does not touch unselected nodes.", 
		EUserInterfaceActionType::Button, 
		FInputChord(EModifierKey::Shift, EKeys::Q) // Default: Shift + Q
	);

	UI_COMMAND(
		ToggleWireStyle, 
		"Toggle Wire Style", 
		"Instantly switches between BlueLine Manhattan wires and Standard Bezier splines.", 
		EUserInterfaceActionType::Button, 
		FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::W) // FIX: Changed to Shift+Alt+W to avoid conflict with "Possess or Eject Player"
	);

	UI_COMMAND(
		RigidifyConnections,
		"Rigidify Wires",
		"Inserts Reroute nodes between selected nodes to force 90-degree lines.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::R)
	);

	UI_COMMAND(
		CleanGraph,
		"Clean Graph",
		"Analyzes the entire graph and performs an intelligent, non-destructive reorganization.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift | EModifierKey::Alt, EKeys::C)
	);

	// Note: AutoTagGraph command is defined in BlueLineSmartTags module (Shift+T)
	// to avoid duplicate command registration conflicts

	// -------------------------------------------------------------------------
	// Multi-mode formatter
	// -------------------------------------------------------------------------

	UI_COMMAND(
		FormatStraight,
		"Format Straight",
		"Top-aligns all selected nodes in the exec chain to their primary parent.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift, EKeys::X)
	);

	UI_COMMAND(
		FormatCenter,
		"Format Center",
		"Vertically centers each node on the connecting pin of its parent.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift, EKeys::V)
	);

	UI_COMMAND(
		FormatCompact,
		"Format Compact",
		"Exec nodes keep their Y; data feeder nodes stack vertically per column.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift, EKeys::B)
	);

	UI_COMMAND(
		SelectConnectedGraph,
		"Select Connected Graph",
		"BFS-selects all nodes reachable from the current selection via any pin connection.",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Shift, EKeys::F)
	);
}

#undef LOCTEXT_NAMESPACE
