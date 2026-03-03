// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h" 

class UEdGraphNode;
class UEdGraph;
class UK2Node_Knot;

class FBlueLineManhattanRouter
{
public:
	// Shift+R Entry Point
	static void RigidifySelectedConnections();

	// Menu Entry Point (Public)
	static bool RouteConnection(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, UEdGraph* Graph);

	// Cleanup Entry Point (Public)
	static int32 CleanupOrphanedRerouteNodes(UEdGraph* Graph);

	struct FConfig
	{
		float HorizontalStubLength = 50.0f;
		float VerticalOffset = 80.0f;
		float GridSnapSize = 16.0f;
		bool bSnapToGrid = true;
	};

	static FConfig Config;

private:
	// Internal Helpers
	static void CalculateManhattanPath(const FVector2D& Start, const FVector2D& End, TArray<FVector2D>& OutPoints);
	static FVector2D GetPinPos(UEdGraphPin* Pin);
	static UK2Node_Knot* CreateRerouteNode(UEdGraph* Graph, const FVector2D& Position, const FEdGraphPinType& PinType);
	static void BreakSpecificLink(UEdGraphPin* Output, UEdGraphPin* Input);
};
