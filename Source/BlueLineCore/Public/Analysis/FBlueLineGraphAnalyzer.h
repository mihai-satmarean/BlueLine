// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

/**
 * FBlueLineGraphAnalyzer
 * 
 * Analyzes Blueprint graph structure to extract meaningful metrics and patterns.
 * This is the "brain" behind BlueLine's intelligent graph operations.
 * 
 * Key Capabilities:
 * - Execution flow chain detection (follows exec pin connections)
 * - Data flow pattern analysis (variable dependencies)
 * - Node clustering (identifies logical groups)
 * - Complexity scoring (0-100 scale)
 * - Wire crossing detection (layout quality metric)
 * - Layer classification (Input/Processing/Output/Utility)
 * 
 * Usage Example:
 * @code
 * UEdGraph* MyGraph = ...;
 * FBlueLineGraphAnalyzer::FAnalysisResult Result = 
 *     FBlueLineGraphAnalyzer::AnalyzeGraph(MyGraph);
 * 
 * UE_LOG(LogTemp, Log, TEXT("Graph has %d nodes, complexity: %.1f"), 
 *     Result.TotalNodes, Result.ComplexityScore);
 * @endcode
 * 
 * @see FBlueLineGraphCleaner for layout operations using these metrics
 * @see FBlueLineSmartTagAnalyzer for semantic analysis
 */
class BLUELINECORE_API FBlueLineGraphAnalyzer
{
public:
    /**
     * Comprehensive analysis results for a Blueprint graph.
     * 
     * Metrics are divided into structural (what exists), quality (how good it is),
     * and actionable (what to do about it) categories.
     */
    struct FAnalysisResult
    {
        //---------------------------------------------------------------------
        // Structure Metrics - Raw counts of graph elements
        //---------------------------------------------------------------------
        
        /** Total number of nodes in the graph (excluding comments) */
        int32 TotalNodes = 0;
        
        /** Total number of pin connections between nodes */
        int32 TotalConnections = 0;
        
        /** Number of distinct execution chains (separated exec flows) */
        int32 ExecutionChains = 0;
        
        /** Number of data-only clusters (pure function groupings) */
        int32 DataClusters = 0;
        
        /** Nodes with no connections (orphaned) */
        int32 IsolatedNodes = 0;

        //---------------------------------------------------------------------
        // Quality Metrics - Layout and readability indicators
        //---------------------------------------------------------------------
        
        /** Number of wire intersections (lower is better) */
        int32 WireCrossings = 0;
        
        /** Connections spanning >1000 pixels (indicates poor organization) */
        int32 LongConnections = 0;
        
        /** Average pixel distance of all connections */
        float AverageConnectionLength = 0.0f;
        
        /** Ratio of actual connections to maximum possible (0-1) */
        float GraphDensity = 0.0f;
        
        //---------------------------------------------------------------------
        // Scores - Normalized 0-100 scales for quick assessment
        //---------------------------------------------------------------------
        
        /**
         * Overall complexity score (0-100, higher = more complex).
         * Based on node count, connection density, and nesting depth.
         * >70: Consider refactoring into subsystems
         * >90: Likely needs immediate attention
         */
        float ComplexityScore = 0.0f;
        
        /**
         * Readability score (0-100, higher = more readable).
         * Based on wire crossings, alignment, and clustering quality.
         * <30: Use Clean Graph (Shift+C)
         * <50: Consider manual organization
         */
        float ReadabilityScore = 0.0f;

        //---------------------------------------------------------------------
        // Actionable Feedback - Specific issues and recommendations
        //---------------------------------------------------------------------
        
        /** List of detected problems (e.g., "Long wire detected") */
        TArray<FString> Issues;
        
        /** List of recommended actions (e.g., "Use Rigidify on nodes 5-10") */
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
     * @param Graph - The graph to analyze
     * @param SelectedNodes - If non-empty, only return clusters containing at least one of these nodes
     * @return Array of detected clusters
     */
    static TArray<FNodeCluster> DetectNodeClusters(UEdGraph* Graph, const TArray<UEdGraphNode*>& SelectedNodes = TArray<UEdGraphNode*>());

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
    
    /** Calculate bounds and center point for a cluster */
    static void CalculateClusterBounds(FNodeCluster& Cluster);
};
