// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * FBlueLineGraphCleaner
 * 
 * High-IQ auto-organization for Blueprint graphs.
 * Uses analysis to perform a non-destructive layout.
 */
class FBlueLineGraphCleaner
{
public:
    /** organizational entry point */
    static void CleanActiveGraph();

    /** Performs a full organization of the provided graph */
    static void CleanGraph(UEdGraph* Graph);

private:
    /** Helper to find the active graph editor */
    static UEdGraph* GetActiveGraph();

    /** 
     * Uses a Genetic Algorithm to minimize wire crossings within an island.
     * This is an upgrade over the standard Barycenter heuristic.
     */
    static void EvolutionaryCrossingMinimizer(TMap<int32, TArray<UEdGraphNode*>>& RankGroups, UEdGraph* Graph);
};
