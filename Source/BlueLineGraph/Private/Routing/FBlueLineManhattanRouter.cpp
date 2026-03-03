// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Routing/FBlueLineManhattanRouter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "K2Node_Knot.h"
#include "ScopedTransaction.h"
#include "Framework/Application/SlateApplication.h"
#include "SGraphPanel.h" 
#include "EdGraphUtilities.h"
#include "BlueLineLog.h"

FBlueLineManhattanRouter::FConfig FBlueLineManhattanRouter::Config;

// Helper struct to persist pin identity across reconstructions
struct FPersistentPin
{
	UEdGraphNode* Node;
	FName PinName;
	EEdGraphPinDirection Direction;

	FPersistentPin(UEdGraphPin* Pin)
	{
		if (Pin)
		{
			Node = Pin->GetOwningNode();
			PinName = Pin->PinName;
			Direction = Pin->Direction;
		}
		else
		{
			Node = nullptr;
		}
	}

	UEdGraphPin* Get() const
	{
		if (!Node) return nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->PinName == PinName && Pin->Direction == Direction) return Pin;
		}
		return nullptr;
	}
};

void FBlueLineManhattanRouter::RigidifySelectedConnections()
{
	// 1. Context Search
	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (!FocusedWidget.IsValid()) return;

	TSharedPtr<SGraphPanel> GraphPanel;
	TSharedPtr<SWidget> CurrentWidget = FocusedWidget;

	int32 Depth = 0;
	while (CurrentWidget.IsValid() && Depth < 50)
	{
		if (CurrentWidget->GetType().ToString().Contains(TEXT("GraphPanel")))
		{
			GraphPanel = StaticCastSharedPtr<SGraphPanel>(CurrentWidget);
			break;
		}
		CurrentWidget = CurrentWidget->GetParentWidget();
		Depth++;
	}

	if (!GraphPanel.IsValid()) return;

	const FGraphPanelSelectionSet& Selection = GraphPanel->SelectionManager.GetSelectedNodes();
	if (Selection.Num() < 2) return;

	UEdGraph* Graph = nullptr;
	TArray<UEdGraphNode*> SelectedNodes;
	for (UObject* Obj : Selection)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
		{
			SelectedNodes.Add(Node);
			if (!Graph) Graph = Node->GetGraph();
		}
	}

	if (!Graph) return;

	// 2. Collection
	struct FConnectionReq { FPersistentPin Out; FPersistentPin In; };
	TArray<FConnectionReq> Requests;

	for (UEdGraphNode* Source : SelectedNodes)
	{
		for (UEdGraphPin* OutputPin : Source->Pins)
		{
			if (OutputPin->Direction != EGPD_Output) continue;

			for (UEdGraphPin* InputPin : OutputPin->LinkedTo)
			{
				UEdGraphNode* Target = InputPin->GetOwningNode();
				if (SelectedNodes.Contains(Target))
				{
					// Left-to-Right only
					if (Target->NodePosX > Source->NodePosX + 150)
					{
						Requests.Add({ FPersistentPin(OutputPin), FPersistentPin(InputPin) });
					}
				}
			}
		}
	}

	if (Requests.Num() == 0) return;

	// 3. Execution
	const FScopedTransaction Transaction(NSLOCTEXT("BlueLine", "Rigidify", "Rigidify Wires"));

	// FIX: Explicitly modify the graph to capture state for Undo
	Graph->Modify();

	bool bGraphModified = false;

	for (const FConnectionReq& Req : Requests)
	{
		// Refresh pointers immediately before use
		UEdGraphPin* SafeOut = Req.Out.Get();
		UEdGraphPin* SafeIn = Req.In.Get();

		if (SafeOut && SafeIn)
		{
			if (RouteConnection(SafeOut, SafeIn, Graph))
			{
				bGraphModified = true;
			}
		}
	}

	if (bGraphModified)
	{
		Graph->NotifyGraphChanged();
	}
}

bool FBlueLineManhattanRouter::RouteConnection(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, UEdGraph* Graph)
{
	// Store persistent handles
	FPersistentPin SafeOut(OutputPin);
	FPersistentPin SafeIn(InputPin);

	FVector2D Start = GetPinPos(OutputPin);
	FVector2D End = GetPinPos(InputPin);

	TArray<FVector2D> PathPoints;
	CalculateManhattanPath(Start, End, PathPoints);

	if (PathPoints.Num() < 3) return false;

	// Capture Pin Type info before potential reconstruction invalidates OutputPin
	const FEdGraphPinType ConnectionType = OutputPin->PinType;

	// 1. Create Knots
	TArray<UK2Node_Knot*> Knots;
	for (int32 i = 1; i < PathPoints.Num() - 1; ++i)
	{
		// FIX: Pass Type to Creation to ensure atomic state
		UK2Node_Knot* Knot = CreateRerouteNode(Graph, PathPoints[i], ConnectionType);
		if (Knot)
		{
			Knots.Add(Knot);
		}
	}

	if (Knots.Num() == 0) return false;

	const UEdGraphSchema* Schema = Graph->GetSchema();

	// 2. Internal Knot Wiring
	for (int32 i = 0; i < Knots.Num() - 1; ++i)
	{
		Schema->TryCreateConnection(Knots[i]->GetOutputPin(), Knots[i + 1]->GetInputPin());
	}

	// 3. Connect OUTPUT -> First Knot
	if (UEdGraphPin* CurrentOut = SafeOut.Get())
	{
		Schema->TryCreateConnection(CurrentOut, Knots[0]->GetInputPin());
	}
	else return false;

	// 4. Connect Last Knot -> INPUT
	if (UEdGraphPin* CurrentIn = SafeIn.Get())
	{
		Schema->TryCreateConnection(Knots.Last()->GetOutputPin(), CurrentIn);
	}
	else return false;

	// 5. Cleanup Old Connection
	UEdGraphPin* FinalOut = SafeOut.Get();
	UEdGraphPin* FinalIn = SafeIn.Get();

	if (FinalOut && FinalIn)
	{
		if (FinalOut->LinkedTo.Contains(FinalIn))
		{
			Schema->BreakSinglePinLink(FinalOut, FinalIn);
		}
	}

	return true;
}

void FBlueLineManhattanRouter::CalculateManhattanPath(const FVector2D& Start, const FVector2D& End, TArray<FVector2D>& OutPoints)
{
	OutPoints.Add(Start);

	float DeltaX = End.X - Start.X;
	float DeltaY = FMath::Abs(End.Y - Start.Y);

	// Proximity and Alignment Thresholds
	const float SafeBuffer = 100.0f;
	const float AlignmentThreshold = 10.0f; // If pins are within 10 units vertically, treat as straight line

	// 1. STRAIGHT LINE CASE: If nearly aligned vertically, just go straight to minimize knots
	if (DeltaY < AlignmentThreshold && DeltaX > 0)
	{
		// No knots needed, but we keep the system consistent by adding 
		// points that result in a straight line if knots are forced.
		// However, RouteConnection will only create knots for points 1 to N-1.
		// If we only have Start and End, 0 knots are created.
		OutPoints.Add(End);
		return;
	}

	// 2. TIGHT SPACE / BACKWARDS CASE: Not enough room for a clean Z-bend
	if (DeltaX < SafeBuffer)
	{
		// If backwards, we go out 40 units then loop around.
		// If tight but forwards, we just split the difference.
		float OutDist = (DeltaX < 0) ? 40.0f : FMath::Max(20.0f, DeltaX * 0.5f);
		float MidX = Start.X + OutDist;
		
		OutPoints.Add(FVector2D(MidX, Start.Y));
		OutPoints.Add(FVector2D(MidX, End.Y));
		OutPoints.Add(End);
		return;
	}

	// 3. STANDARD CASE: Clear Z-Bend (Manhattan)
	// We use the midpoint for the vertical transition
	float MidX = Start.X + (DeltaX * 0.5f);

	OutPoints.Add(FVector2D(MidX, Start.Y));
	OutPoints.Add(FVector2D(MidX, End.Y));
	OutPoints.Add(End);
}

UK2Node_Knot* FBlueLineManhattanRouter::CreateRerouteNode(UEdGraph* Graph, const FVector2D& Position, const FEdGraphPinType& PinType)
{
	FGraphNodeCreator<UK2Node_Knot> NodeCreator(*Graph);
	UK2Node_Knot* Knot = NodeCreator.CreateNode();

	Knot->NodePosX = (int32)Position.X;
	Knot->NodePosY = (int32)Position.Y;

	if (Config.bSnapToGrid)
	{
		Knot->SnapToGrid(Config.GridSnapSize);
	}

	Knot->AllocateDefaultPins();

	// FIX: Finalize first to generate Wildcards
	NodeCreator.Finalize();

	// FIX: IMMEDIATELY set the type so it is recorded in the transaction state correctly.
	// Doing this here ensures that when Undo restores the node, it has the specific type, 
	// preventing "Wildcard vs Concrete" mismatches upon re-connection.
	if (UEdGraphPin* InPin = Knot->GetInputPin())
	{
		InPin->PinType = PinType;
	}
	if (UEdGraphPin* OutPin = Knot->GetOutputPin())
	{
		OutPin->PinType = PinType;
	}

	return Knot;
}

void FBlueLineManhattanRouter::BreakSpecificLink(UEdGraphPin* Output, UEdGraphPin* Input)
{
	if (!Output || !Input) return;
	const UEdGraphSchema* Schema = Output->GetSchema();
	if (Schema) Schema->BreakSinglePinLink(Output, Input);
}

FVector2D FBlueLineManhattanRouter::GetPinPos(UEdGraphPin* Pin)
{
	if (!Pin) return FVector2D::ZeroVector;
	UEdGraphNode* Node = Pin->GetOwningNode();
	if (!Node) return FVector2D::ZeroVector;

	float XOffset = 0.0f;

	if (Pin->Direction == EGPD_Output)
	{
		// Use actual node width if available, with a sane minimum for visibility.
		// Knot nodes (reroutes) handled specifically to keep wires tight.
		float Width = (float)Node->NodeWidth;
		if (Node->IsA<UK2Node_Knot>())
		{
			// Knots are small dots, pins are effectively in the center (16,16)
			XOffset = 16.0f;
		}
		else
		{
			// For regular nodes, ensure we clear the body. 
			// If width isn't calculated yet (0), use a standard fallback.
			XOffset = (Width > 0) ? Width : 128.0f;
		}
	}
	else if (Node->IsA<UK2Node_Knot>())
	{
		// Input pin for a knot is also at the center
		XOffset = 16.0f;
	}

	// Y-Offset heuristic
	// Header is usually ~48 units. Standard pin height is ~24 units.
	float YOffset = 48.0f;
	const float PinHeight = 24.0f;
	const float HalfPinHeight = 12.0f;

	int32 VisibleIndex = 0;
	for (const UEdGraphPin* P : Node->Pins)
	{
		if (P == Pin)
		{
			break;
		}

		// Only count pins on the same side (Direction) that are actually visible
		if (P && P->Direction == Pin->Direction && !P->bHidden)
		{
			VisibleIndex++;
		}
	}

	YOffset += (VisibleIndex * PinHeight) + HalfPinHeight;

	return FVector2D(Node->NodePosX + XOffset, Node->NodePosY + YOffset);
}

int32 FBlueLineManhattanRouter::CleanupOrphanedRerouteNodes(UEdGraph* Graph)
{
	if (!Graph) return 0;

	const FScopedTransaction Transaction(NSLOCTEXT("BlueLine", "Cleanup", "Cleanup BlueLine Reroutes"));
	Graph->Modify();

	int32 Count = 0;
	TArray<UEdGraphNode*> NodesToDestroy;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UK2Node_Knot* Knot = Cast<UK2Node_Knot>(Node))
		{
			// Check connections
			UEdGraphPin* InPin = Knot->GetInputPin();
			UEdGraphPin* OutPin = Knot->GetOutputPin();

			bool bHasInput = InPin && InPin->LinkedTo.Num() > 0;
			bool bHasOutput = OutPin && OutPin->LinkedTo.Num() > 0;

			// Remove if completely disconnected or only one-way connected (dead end)
			// (Behavior: Clean anything that doesn't bridge two nodes)
			if (!bHasInput || !bHasOutput)
			{
				NodesToDestroy.Add(Knot);
			}
		}
	}

	for (UEdGraphNode* Node : NodesToDestroy)
	{
		Graph->RemoveNode(Node);
		Count++;
	}

	if (Count > 0)
	{
		Graph->NotifyGraphChanged();
	}

	return Count;
}
