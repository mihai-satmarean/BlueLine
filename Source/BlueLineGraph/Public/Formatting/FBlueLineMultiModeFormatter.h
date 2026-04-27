// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraphNode;
class UEdGraph;
class SGraphPanel;

/**
 * EFormatMode
 *
 * The three layout modes ported from Auto Node Arranger's documented algorithm.
 *
 * Straight: All nodes in an exec chain are top-aligned to their parent.
 *           Best for event-driven chains that flow left-to-right cleanly.
 *
 * Center:   Each node is vertically centered on the pin it connects to.
 *           Best for data-heavy graphs with many parallel branches.
 *
 * Compact:  Data-input nodes are stacked vertically in columns.
 *           Exec chain nodes keep their Y; data feeders pack tightly below them.
 *           Best for large graphs that need maximum density.
 */
enum class EFormatMode : uint8
{
    Straight,
    Center,
    Compact,
};

/**
 * FBlueLineMultiModeFormatter
 *
 * Multi-mode Blueprint graph formatter.
 *
 * - Straight  (Shift+X)  — top-align all exec-chain nodes
 * - Center    (Shift+V)  — center-align nodes to their connecting pin
 * - Compact   (Shift+B)  — minimum vertical spread; data nodes stack in columns
 *
 * Also provides SelectConnectedGraph (Shift+F) — BFS expansion from current selection.
 */
class BLUELINEGRAPH_API FBlueLineMultiModeFormatter
{
public:
    /** Entry points bound to keyboard shortcuts */
    static void FormatStraight();
    static void FormatCenter();
    static void FormatCompact();

    /**
     * SelectConnectedGraph
     *
     * BFS from the current selection. Expands via all pin connections (exec + data).
     * Equivalent to Auto Node Arranger's "Select Connected Graph" (Shift+F).
     */
    static void SelectConnectedGraph();

    /** Core formatting logic — called by the three entry points */
    static void FormatWithMode(EFormatMode Mode);

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /**
     * Estimates a node's height in graph units.
     * UE only populates NodeHeight for Comment and a handful of custom nodes.
     * For everything else we estimate from the number of visible pins.
     */
    static int32 EstimateNodeHeight(const UEdGraphNode* Node);

    /**
     * Estimates a node's width in graph units.
     */
    static int32 EstimateNodeWidth(const UEdGraphNode* Node);

    /**
     * Returns the primary parent of Node (the connected node furthest to the left).
     * Exec pins take priority over data pins.
     */
    static UEdGraphNode* FindPrimaryParent(UEdGraphNode* Node, const TSet<UEdGraphNode*>& NodeSet);

    /**
     * X-alignment pass: ensures each node is at least HorizontalSpacing pixels
     * to the right of its primary parent. Never pulls nodes backwards.
     */
    static void AlignX(TArray<UEdGraphNode*>& Nodes, float HorizontalSpacing);

    /**
     * Straight Y-pass: top-align each node to its primary parent.
     */
    static void AlignY_Straight(TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet);

    /**
     * Center Y-pass: center each node vertically on the pin it connects to.
     * Approximates pin offset from node top using pin index in the pin list.
     */
    static void AlignY_Center(TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet);

    /**
     * Compact Y-pass: exec nodes keep their Y; data-only feeder nodes are
     * stacked vertically in the column they occupy with minimum gap.
     */
    static void AlignY_Compact(TArray<UEdGraphNode*>& Nodes, const TSet<UEdGraphNode*>& NodeSet, float VerticalSpacing);

    /**
     * Collision resolution: push overlapping nodes downward.
     * Runs multiple passes until no overlaps remain (capped at MaxPasses).
     */
    static void ResolveCollisions(TArray<UEdGraphNode*>& Nodes, int32 MaxPasses = 4);

    /**
     * Returns true if Node has at least one connected exec output pin,
     * i.e. it is part of an execution flow chain.
     */
    static bool HasExecOutput(const UEdGraphNode* Node);

    /**
     * BFS: returns all nodes reachable from Seeds via any pin connection.
     */
    static TSet<UEdGraphNode*> BFSConnected(const TArray<UEdGraphNode*>& Seeds, UEdGraph* Graph);
};
