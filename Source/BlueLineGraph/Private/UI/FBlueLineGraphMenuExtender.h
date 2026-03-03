// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FExtender;
class FUICommandList;
class FMenuBuilder;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
struct FGraphContextMenuArguments; // specific UE5 struct if available, else we use signatures

class FBlueLineGraphMenuExtender
{
public:
	static void Register();
	static void Unregister();

private:
	// The Delegate Callback
	static TSharedRef<FExtender> OnExtendContextMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bIsDebug);

	// Menu Builders
	static void AddGraphMenuEntries(FMenuBuilder& MenuBuilder, UEdGraph* Graph);
	static void AddPinMenuEntries(FMenuBuilder& MenuBuilder, const UEdGraphPin* Pin);

	// Action Callbacks
	static void ExecuteStraightenConnection(UEdGraphPin* Pin);
	static void ExecuteStraightenAll(UEdGraph* Graph);
	static void ExecuteCleanup(UEdGraph* Graph);

};
