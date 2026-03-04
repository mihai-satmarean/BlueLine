// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NameTypes.h"

class UEdGraph;
class UEdGraphNode;

/**
 * Result of a smart tag analysis
 */
struct FBlueLineSmartTagSuggestion
{
    FGameplayTag Tag;
    FLinearColor SuggestedColor = FLinearColor::White;
    float Confidence = 0.0f;
    FText Reason;
    
    bool operator<(const FBlueLineSmartTagSuggestion& Other) const
    {
        return Confidence > Other.Confidence; // Sort descending
    }
};

/**
 * FBlueLineSmartTagAnalyzer
 * 
 * The "IQ" behind Smart Tagging. Analyzes graph topology and node semantics
 * to suggest appropriate GameplayTags for organization and debugging.
 */
class BLUELINESMARTTAGS_API FBlueLineSmartTagAnalyzer
{
public:
    /**
     * Analyzes the given graph and returns a list of suggested tags with confidence scores.
     */
    static TArray<FBlueLineSmartTagSuggestion> SuggestTagsForGraph(UEdGraph* Graph);

    /**
     * Analyzes a specific node and its immediate neighbors.
     */
    static TArray<FBlueLineSmartTagSuggestion> SuggestTagsForNode(UEdGraphNode* Node);

    /**
     * The "Maximal IQ" Auto-Tagger.
     * Identifies logical clusters, classifies them, and wraps them in semantic Comment Boxes.
     * @param Graph - The graph to analyze
     * @param SelectedNodes - If non-empty, only tag clusters containing these nodes. Otherwise tags all clusters.
     */
    static void AutoTagGraph(UEdGraph* Graph, const TArray<UEdGraphNode*>& SelectedNodes = TArray<UEdGraphNode*>());

private:
    /** 
     * Represents a semantic category for nodes
     */
    enum class ENodeSemantic : uint8
    {
        Unknown,
        Movement,
        Combat,
        UI,
        Input,
        Networking,
        Audio,
        Visuals,
        Logic,
        Data,
        AI
    };

    /**
     * Internal structure to track semantic weights during analysis
     */
    struct FSemanticContext
    {
        TMap<ENodeSemantic, float> Weights;
        void AddWeight(ENodeSemantic Semantic, float Weight)
        {
            Weights.FindOrAdd(Semantic) += Weight;
        }
    };

    /**
     * Core heuristic: maps a node to a semantic category and weight
     */
    static void AnalyzeNodeSemantics(UEdGraphNode* Node, FSemanticContext& Context);

    /**
     * Maps semantic categories to actual GameplayTags (configurable via settings in future)
     */
    static FGameplayTag MapSemanticToTag(ENodeSemantic Semantic);

    /**
     * Gets the brand color for a semantic category
     */
    static FLinearColor GetColorForSemantic(ENodeSemantic Semantic);

    /**
     * Helper to get a human-readable reason for a suggestion
     */
    static FText GetReasonForSemantic(ENodeSemantic Semantic, float Confidence);
};
