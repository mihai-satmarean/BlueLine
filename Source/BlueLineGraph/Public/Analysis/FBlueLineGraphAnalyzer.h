// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

/**
 * FBlueLineGraphAnalyzer
 * 
 * Analyzes Blueprint graph structure to understand:
 * - Execution flow chains
 * - Data flow patterns
 * - Node dependencies and clusters
 * - Graph complexity metrics
 * - Wire crossing counts
 * - Layout quality
 * 
 * Provides the intelligence for the Clean Graph utility.
 */
class BLUELINEGRAPH_API FBlueLineGraphAnalyzer
{
public:
    /**
     * Analysis results for a graph
     */
    struct FAnalysisResult
    {
        // Structure metrics
        int32 TotalNodes = 0;
        int32 TotalConnections = 0;
        int32 ExecutionChains = 0;
        int32 DataClusters = 0;
        int32 IsolatedNodes = 0;

        // Quality metrics
        int32 WireCrossings = 0;
        int32 LongConnections = 0;
        float AverageConnectionLength = 0.0f;
        float GraphDensity = 0.0f;
        
        // Complexity score (0-100, higher = more complex)
        float ComplexityScore = 0.0f;
        
        // Readability score (0-100, higher = more readable)
        float ReadabilityScore = 0.0f;

        // Issues detected
        TArray<FString> Issues;
        TArray<FString> Suggestions;
    };

    /**
     * Node layer classification (for columnar layout)
     */
    enum class ENodeLayer : uint8
    {
        Input,      // Entry points (events, function inputs)
        Processing, // Middle logic
        Output,     // Exit points (return nodes, outputs)
        Utility     // Pure functions, getters, constants
    };

    /**
     * Node cluster - group of related nodes
     */
    struct FNodeCluster
    {
        TArray<UEdGraphNode*> Nodes;
        FVector2D CenterPoint;
        FBox2D Bounds;
        ENodeLayer Layer;
        int32 Depth = 0; // Distance from input layer
    };

    /**
     * Analyze a graph and return detailed metrics
     */
    static FAnalysisResult AnalyzeGraph(UEdGraph* Graph);

    /**
     * Classify nodes into layers for columnar layout
     */
    static TMap<UEdGraphNode*, ENodeLayer> ClassifyNodeLayers(UEdGraph* Graph);

    /**
     * Detect node clusters (groups of related nodes)
     */
    static TArray<FNodeCluster> DetectNodeClusters(UEdGraph* Graph);

    /**
     * Find execution chains (connected exec pin sequences)
     */
    static TArray<TArray<UEdGraphNode*>> FindExecutionChains(UEdGraph* Graph);

    /**
     * Count wire crossings in current layout
     */
    static int32 CountWireCrossings(UEdGraph* Graph);

    /**
     * Calculate graph bounds
     */
    static FBox2D CalculateGraphBounds(UEdGraph* Graph);

    /**
     * Detect problematic patterns based on analysis metrics
     */
    static TArray<FString> DetectIssues(const FAnalysisResult& Analysis);

    /**
     * Generate improvement suggestions
     */
    static TArray<FString> GenerateSuggestions(const FAnalysisResult& Analysis);

private:
    // Helper functions
    static ENodeLayer DetermineNodeLayer(UEdGraphNode* Node);
    static bool IsInputNode(UEdGraphNode* Node);
    static bool IsOutputNode(UEdGraphNode* Node);
    static bool IsPureNode(UEdGraphNode* Node);
    
    // Calculates depth from the nearest input node (BFS)
    static int32 CalculateNodeDepth(UEdGraphNode* Node, UEdGraph* Graph);
    
    static bool DoLinesIntersect(const FVector2D& A1, const FVector2D& A2, const FVector2D& B1, const FVector2D& B2);
    static FVector2D GetNodePosition(UEdGraphNode* Node);
    static float CalculateConnectionLength(UEdGraphPin* OutputPin, UEdGraphPin* InputPin);
};
