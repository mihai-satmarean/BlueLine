// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Formatting/FBlueLineMultiModeFormatter.h"

#include "BlueLineCore/Public/Settings/UBlueLineEditorSettings.h"
#include "BlueLineCore/Public/Utils/BlueLineContextUtils.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "SGraphPanel.h"
#include "GraphEditor.h"
#include "ScopedTransaction.h"
#include "K2Node_Knot.h"

#define LOCTEXT_NAMESPACE "BlueLineMultiModeFormatter"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int32 PIN_HEIGHT_ESTIMATE    = 22;   // px per visible pin row
static constexpr int32 NODE_HEADER_HEIGHT     = 36;   // px for node title bar
static constexpr int32 NODE_FOOTER_HEIGHT     = 8;    // px bottom padding
static constexpr int32 DEFAULT_NODE_WIDTH     = 160;  // fallback when NodeWidth == 0
static constexpr int32 COMPACT_COLUMN_GAP     = 20;   // extra px between columns in Compact

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::FormatStraight()  { FormatWithMode(EFormatMode::Straight); }
void FBlueLineMultiModeFormatter::FormatCenter()    { FormatWithMode(EFormatMode::Center);   }
void FBlueLineMultiModeFormatter::FormatCompact()   { FormatWithMode(EFormatMode::Compact);  }

void FBlueLineMultiModeFormatter::FormatWithMode(EFormatMode Mode)
{
    TSharedPtr<SGraphPanel> Panel = FBlueLineContextUtils::GetFocusedGraphPanel();
    if (!Panel.IsValid()) return;

    const FGraphPanelSelectionSet& RawSelection = Panel->SelectionManager.GetSelectedNodes();
    if (RawSelection.Num() < 2) return;

    const UBlueLineEditorSettings* Settings = GetDefault<UBlueLineEditorSettings>();
    const float HSpacing = Settings ? Settings->HorizontalSpacing : 300.f;
    const float VSpacing = Settings ? Settings->VerticalSpacing   : 120.f;
    const int32 GridSnap = Settings ? Settings->GridSnapSize      : 16;

    // Collect only UEdGraphNodes (skip comments and non-node objects)
    TArray<UEdGraphNode*> Nodes;
    TSet<UEdGraphNode*>   NodeSet;
    UEdGraph* Graph = nullptr;

    for (UObject* Obj : RawSelection)
    {
        if (UEdGraphNode* N = Cast<UEdGraphNode>(Obj))
        {
            Nodes.Add(N);
            NodeSet.Add(N);
            if (!Graph) Graph = N->GetGraph();
            N->Modify();
        }
    }

    if (Nodes.Num() < 2 || !Graph) return;

    // Sort left-to-right so parent lookup always works
    Nodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B)
    {
        return A.NodePosX < B.NodePosX;
    });

    FScopedTransaction Transaction(
        Mode == EFormatMode::Straight ? LOCTEXT("FormatStraight", "BlueLine: Format Straight") :
        Mode == EFormatMode::Center   ? LOCTEXT("FormatCenter",   "BlueLine: Format Center")   :
                                        LOCTEXT("FormatCompact",  "BlueLine: Format Compact")
    );

    // --- X pass (same for all modes) ---
    AlignX(Nodes, HSpacing);

    // --- Y pass (mode-specific) ---
    switch (Mode)
    {
    case EFormatMode::Straight: AlignY_Straight(Nodes, NodeSet);              break;
    case EFormatMode::Center:   AlignY_Center  (Nodes, NodeSet);              break;
    case EFormatMode::Compact:  AlignY_Compact (Nodes, NodeSet, VSpacing);    break;
    }

    // --- Collision pass ---
    ResolveCollisions(Nodes);

    // --- Grid snap ---
    for (UEdGraphNode* N : Nodes)
    {
        N->SnapToGrid(GridSnap);
    }

    Graph->NotifyGraphChanged();
}

// ---------------------------------------------------------------------------
// SelectConnectedGraph — BFS from current selection
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::SelectConnectedGraph()
{
    TSharedPtr<SGraphPanel> Panel = FBlueLineContextUtils::GetFocusedGraphPanel();
    if (!Panel.IsValid()) return;

    const FGraphPanelSelectionSet& Raw = Panel->SelectionManager.GetSelectedNodes();
    if (Raw.Num() == 0) return;

    TArray<UEdGraphNode*> Seeds;
    UEdGraph* Graph = nullptr;

    for (UObject* Obj : Raw)
    {
        if (UEdGraphNode* N = Cast<UEdGraphNode>(Obj))
        {
            Seeds.Add(N);
            if (!Graph) Graph = N->GetGraph();
        }
    }

    if (!Graph) return;

    TSet<UEdGraphNode*> Connected = BFSConnected(Seeds, Graph);

    // Apply selection
    Panel->SelectionManager.ClearSelectionSet();
    for (UEdGraphNode* N : Connected)
    {
        Panel->SelectionManager.SetNodeSelection(N, true);
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

int32 FBlueLineMultiModeFormatter::EstimateNodeHeight(const UEdGraphNode* Node)
{
    if (!Node) return 100;
    if (Node->NodeHeight > 0) return Node->NodeHeight;

    // Count visible (non-hidden) pins
    int32 PinCount = 0;
    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && !Pin->bHidden)
        {
            PinCount++;
        }
    }

    // Rough estimate: header + one row per visible pin + footer
    // Pins are distributed equally between input and output sides so
    // height is driven by whichever side has more pins.
    int32 MaxSidePins = FMath::DivideAndRoundUp(PinCount, 2);
    return NODE_HEADER_HEIGHT + (MaxSidePins * PIN_HEIGHT_ESTIMATE) + NODE_FOOTER_HEIGHT;
}

int32 FBlueLineMultiModeFormatter::EstimateNodeWidth(const UEdGraphNode* Node)
{
    if (!Node) return DEFAULT_NODE_WIDTH;
    return (Node->NodeWidth > 0) ? Node->NodeWidth : DEFAULT_NODE_WIDTH;
}

UEdGraphNode* FBlueLineMultiModeFormatter::FindPrimaryParent(
    UEdGraphNode* Node, const TSet<UEdGraphNode*>& NodeSet)
{
    if (!Node) return nullptr;

    UEdGraphNode* ExecParent = nullptr;
    UEdGraphNode* DataParent = nullptr;

    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin || Pin->Direction != EGPD_Input || Pin->LinkedTo.Num() == 0) continue;

        UEdGraphNode* Candidate = Pin->LinkedTo[0]->GetOwningNode();
        if (!Candidate || !NodeSet.Contains(Candidate)) continue;
        if (Candidate->NodePosX >= Node->NodePosX) continue; // must be to our left

        const bool bIsExec = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
        if (bIsExec)
        {
            ExecParent = Candidate;
            break; // exec wins immediately
        }
        if (!DataParent)
        {
            DataParent = Candidate;
        }
    }

    return ExecParent ? ExecParent : DataParent;
}

bool FBlueLineMultiModeFormatter::HasExecOutput(const UEdGraphNode* Node)
{
    if (!Node) return false;
    for (const UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin && Pin->Direction == EGPD_Output &&
            Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
            Pin->LinkedTo.Num() > 0)
        {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// X alignment (shared across all modes)
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::AlignX(TArray<UEdGraphNode*>& Nodes, float HorizontalSpacing)
{
    for (UEdGraphNode* Node : Nodes)
    {
        if (!Node) continue;

        // Find leftmost connected parent inside the selection (already sorted)
        int32 BestParentRight = INT32_MIN;

        for (const UEdGraphPin* Pin : Node->Pins)
        {
            if (!Pin || Pin->Direction != EGPD_Input || Pin->LinkedTo.Num() == 0) continue;
            UEdGraphNode* Parent = Pin->LinkedTo[0]->GetOwningNode();
            if (!Parent) continue;
            if (Parent->NodePosX >= Node->NodePosX) continue;

            int32 ParentRight = Parent->NodePosX + EstimateNodeWidth(Parent);
            BestParentRight = FMath::Max(BestParentRight, ParentRight);
        }

        if (BestParentRight != INT32_MIN)
        {
            int32 TargetX = BestParentRight + (int32)HorizontalSpacing;
            if (Node->NodePosX < TargetX)
            {
                Node->NodePosX = TargetX;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Straight Y: top-align each node to its primary parent
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::AlignY_Straight(
    TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet)
{
    for (UEdGraphNode* Node : Nodes)
    {
        UEdGraphNode* Parent = FindPrimaryParent(Node, NodeSet);
        if (Parent)
        {
            Node->NodePosY = Parent->NodePosY;
        }
    }
}

// ---------------------------------------------------------------------------
// Center Y: vertically center each node on the pin it connects to
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::AlignY_Center(
    TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet)
{
    for (UEdGraphNode* Node : Nodes)
    {
        if (!Node) continue;

        UEdGraphNode* Parent = nullptr;
        int32 MyPinIndex     = 0;  // index of the connected input pin on Node
        int32 ParentPinIndex = 0;  // index of the connected output pin on Parent

        for (int32 pi = 0; pi < Node->Pins.Num(); ++pi)
        {
            const UEdGraphPin* Pin = Node->Pins[pi];
            if (!Pin || Pin->Direction != EGPD_Input || Pin->LinkedTo.Num() == 0) continue;

            UEdGraphNode* Candidate = Pin->LinkedTo[0]->GetOwningNode();
            if (!Candidate || !NodeSet.Contains(Candidate)) continue;
            if (Candidate->NodePosX >= Node->NodePosX) continue;

            const bool bIsExec = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
            if (!Parent || bIsExec)
            {
                Parent = Candidate;
                MyPinIndex = pi;

                // Find the index of LinkedTo[0] in the parent's pin list
                const UEdGraphPin* ParentPin = Pin->LinkedTo[0];
                for (int32 pj = 0; pj < Parent->Pins.Num(); ++pj)
                {
                    if (Parent->Pins[pj] == ParentPin)
                    {
                        ParentPinIndex = pj;
                        break;
                    }
                }

                if (bIsExec) break;
            }
        }

        if (!Parent) continue;

        // Approximate Y of the parent's output pin
        // (header height + pin_index * pin_height + half pin height)
        const int32 ParentPinY = Parent->NodePosY
            + NODE_HEADER_HEIGHT
            + (ParentPinIndex * PIN_HEIGHT_ESTIMATE)
            + (PIN_HEIGHT_ESTIMATE / 2);

        // Approximate Y of our input pin (same formula)
        const int32 MyPinLocalY = NODE_HEADER_HEIGHT
            + (MyPinIndex * PIN_HEIGHT_ESTIMATE)
            + (PIN_HEIGHT_ESTIMATE / 2);

        // Target: our pin aligns with parent's pin
        Node->NodePosY = ParentPinY - MyPinLocalY;
    }
}

// ---------------------------------------------------------------------------
// Compact Y: exec nodes keep Y; data feeders stack vertically per column
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::AlignY_Compact(
    TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet, float VerticalSpacing)
{
    // Group nodes by their X column (±100px tolerance)
    const int32 ColTolerance = 100;
    TMap<int32, TArray<UEdGraphNode*>> Columns;

    for (UEdGraphNode* Node : Nodes)
    {
        bool bFound = false;
        for (auto& Pair : Columns)
        {
            if (FMath::Abs(Node->NodePosX - Pair.Key) <= ColTolerance)
            {
                Pair.Value.Add(Node);
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            Columns.Add(Node->NodePosX, { Node });
        }
    }

    // Per column: exec nodes keep their Y; data-only nodes are stacked
    for (auto& ColPair : Columns)
    {
        TArray<UEdGraphNode*>& ColNodes = ColPair.Value;

        // Sort within column top-to-bottom by current Y
        ColNodes.Sort([](const UEdGraphNode& A, const UEdGraphNode& B)
        {
            return A.NodePosY < B.NodePosY;
        });

        // First pass: anchor exec nodes (keep Y), collect data-only nodes
        TArray<UEdGraphNode*> DataNodes;
        int32 LowestExecY = INT32_MIN;

        for (UEdGraphNode* N : ColNodes)
        {
            if (HasExecOutput(N))
            {
                LowestExecY = FMath::Max(LowestExecY,
                    N->NodePosY + EstimateNodeHeight(N) + (int32)VerticalSpacing);
            }
            else
            {
                DataNodes.Add(N);
            }
        }

        // Second pass: stack data nodes starting just below the lowest exec node
        // If no exec node in this column, start at Y=0 of the first data node.
        int32 CursorY = (LowestExecY != INT32_MIN)
            ? LowestExecY
            : (DataNodes.Num() > 0 ? DataNodes[0]->NodePosY : 0);

        for (UEdGraphNode* N : DataNodes)
        {
            N->NodePosY = CursorY;
            CursorY += EstimateNodeHeight(N) + (int32)(VerticalSpacing * 0.5f);
        }
    }
}

// ---------------------------------------------------------------------------
// Collision resolution
// ---------------------------------------------------------------------------

void FBlueLineMultiModeFormatter::ResolveCollisions(TArray<UEdGraphNode*>& Nodes, int32 MaxPasses)
{
    const UBlueLineEditorSettings* Settings = GetDefault<UBlueLineEditorSettings>();
    const int32 Padding = Settings ? Settings->CollisionPadding : 20;

    for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
    {
        bool bAnyPush = false;

        for (int32 i = 0; i < Nodes.Num(); ++i)
        {
            for (int32 j = i + 1; j < Nodes.Num(); ++j)
            {
                UEdGraphNode* A = Nodes[i];
                UEdGraphNode* B = Nodes[j];
                if (!A || !B) continue;

                const int32 AW = EstimateNodeWidth(A);
                const int32 BW = EstimateNodeWidth(B);
                const int32 AH = EstimateNodeHeight(A);
                const int32 BH = EstimateNodeHeight(B);

                // AABB overlap
                const bool bXOverlap =
                    A->NodePosX < B->NodePosX + BW + Padding &&
                    A->NodePosX + AW + Padding > B->NodePosX;
                const bool bYOverlap =
                    A->NodePosY < B->NodePosY + BH + Padding &&
                    A->NodePosY + AH + Padding > B->NodePosY;

                if (bXOverlap && bYOverlap)
                {
                    // Push the lower node further down
                    UEdGraphNode* Top    = (A->NodePosY <= B->NodePosY) ? A : B;
                    UEdGraphNode* Bottom = (Top == A) ? B : A;
                    const int32 TopH    = EstimateNodeHeight(Top);

                    Bottom->NodePosY = Top->NodePosY + TopH + Padding;
                    bAnyPush = true;
                }
            }
        }

        if (!bAnyPush) break; // converged early
    }
}

// ---------------------------------------------------------------------------
// BFS for SelectConnectedGraph
// ---------------------------------------------------------------------------

TSet<UEdGraphNode*> FBlueLineMultiModeFormatter::BFSConnected(
    const TArray<UEdGraphNode*>& Seeds, UEdGraph* Graph)
{
    TSet<UEdGraphNode*> Visited;
    TQueue<UEdGraphNode*> Queue;

    for (UEdGraphNode* S : Seeds)
    {
        if (S && !Visited.Contains(S))
        {
            Visited.Add(S);
            Queue.Enqueue(S);
        }
    }

    while (!Queue.IsEmpty())
    {
        UEdGraphNode* Current = nullptr;
        Queue.Dequeue(Current);

        for (const UEdGraphPin* Pin : Current->Pins)
        {
            if (!Pin) continue;
            for (const UEdGraphPin* Linked : Pin->LinkedTo)
            {
                if (!Linked) continue;
                UEdGraphNode* Neighbor = Linked->GetOwningNode();
                if (Neighbor && !Visited.Contains(Neighbor))
                {
                    Visited.Add(Neighbor);
                    Queue.Enqueue(Neighbor);
                }
            }
        }
    }

    return Visited;
}

#undef LOCTEXT_NAMESPACE
