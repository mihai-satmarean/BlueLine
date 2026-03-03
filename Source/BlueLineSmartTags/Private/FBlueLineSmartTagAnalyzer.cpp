// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "FBlueLineSmartTagAnalyzer.h"
#include "Analysis/FBlueLineGraphAnalyzer.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Variable.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Timeline.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Knot.h"
#include "EdGraphNode_Comment.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "BlueLineSmartTagAnalyzer"

TArray<FBlueLineSmartTagSuggestion> FBlueLineSmartTagAnalyzer::SuggestTagsForGraph(UEdGraph* Graph)
{
    TArray<FBlueLineSmartTagSuggestion> Suggestions;
    if (!Graph) return Suggestions;

    FSemanticContext Context;

    // 1. Cluster-based Analysis
    TArray<FBlueLineGraphAnalyzer::FNodeCluster> Clusters = FBlueLineGraphAnalyzer::DetectNodeClusters(Graph);
    
    for (const auto& Cluster : Clusters)
    {
        FSemanticContext ClusterContext;
        for (UEdGraphNode* Node : Cluster.Nodes)
        {
            AnalyzeNodeSemantics(Node, ClusterContext);
        }

        // Merge cluster context into global context with cluster-size weighting
        float ClusterMultiplier = FMath::Sqrt((float)Cluster.Nodes.Num());
        for (auto& It : ClusterContext.Weights)
        {
            Context.AddWeight(It.Key, It.Value * ClusterMultiplier);
        }
    }

    // 2. Fallback to individual node analysis for unclustered logic
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        AnalyzeNodeSemantics(Node, Context);
    }

    // 3. Convert weights to suggestions
    for (auto& It : Context.Weights)
    {
        if (It.Value > 1.0f) // Threshold
        {
            FBlueLineSmartTagSuggestion Suggestion;
            Suggestion.Tag = MapSemanticToTag(It.Key);
            Suggestion.SuggestedColor = GetColorForSemantic(It.Key);
            
            // Normalize confidence (rough heuristic)
            Suggestion.Confidence = FMath::Clamp(It.Value / 20.0f, 0.1f, 0.95f);
            Suggestion.Reason = GetReasonForSemantic(It.Key, Suggestion.Confidence);
            
            Suggestions.Add(Suggestion);
        }
    }

    Suggestions.Sort();
    return Suggestions;
}

TArray<FBlueLineSmartTagSuggestion> FBlueLineSmartTagAnalyzer::SuggestTagsForNode(UEdGraphNode* Node)
{
    TArray<FBlueLineSmartTagSuggestion> Suggestions;
    if (!Node) return Suggestions;

    FSemanticContext Context;
    AnalyzeNodeSemantics(Node, Context);

    for (auto& It : Context.Weights)
    {
        FBlueLineSmartTagSuggestion Suggestion;
        Suggestion.Tag = MapSemanticToTag(It.Key);
        Suggestion.SuggestedColor = GetColorForSemantic(It.Key);
        Suggestion.Confidence = FMath::Clamp(It.Value / 5.0f, 0.1f, 1.0f);
        Suggestion.Reason = GetReasonForSemantic(It.Key, Suggestion.Confidence);
        Suggestions.Add(Suggestion);
    }

    Suggestions.Sort();
    return Suggestions;
}

void FBlueLineSmartTagAnalyzer::AutoTagGraph(UEdGraph* Graph)
{
    if (!Graph) return;

    const FScopedTransaction Transaction(LOCTEXT("AutoTagTransaction", "Auto-Tag Graph"));
    Graph->Modify();

    // 1. Detect Clusters using the Graph Analyzer
    TArray<FBlueLineGraphAnalyzer::FNodeCluster> Clusters = FBlueLineGraphAnalyzer::DetectNodeClusters(Graph);

    for (const auto& Cluster : Clusters)
    {
        // "IQ" Check: Don't tag clusters that are too small or likely trivial
        if (Cluster.Nodes.Num() < 3) continue; 

        // 2. Analyze Cluster Semantics
        FSemanticContext Context;
        for (UEdGraphNode* Node : Cluster.Nodes)
        {
            AnalyzeNodeSemantics(Node, Context);
        }

        // Find the strongest semantic mapping
        ENodeSemantic BestSemantic = ENodeSemantic::Unknown;
        float BestWeight = 0.0f;
        for (auto& It : Context.Weights)
        {
            if (It.Value > BestWeight)
            {
                BestWeight = It.Value;
                BestSemantic = It.Key;
            }
        }

        // 3. Threshold check: Only tag if we are reasonably sure
        if (BestSemantic != ENodeSemantic::Unknown && BestWeight > 3.0f)
        {
            FGameplayTag Tag = MapSemanticToTag(BestSemantic);
            FLinearColor Color = GetColorForSemantic(BestSemantic);

            // Calculate bounds with padding
            FBox2D Bounds = Cluster.Bounds;
            const float Padding = 60.0f;
            const float TitleBarHeight = 40.0f;
            
            // Create the comment node
            UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph);
            CommentNode->NodePosX = Bounds.Min.X - Padding;
            CommentNode->NodePosY = Bounds.Min.Y - Padding - TitleBarHeight; 
            CommentNode->NodeWidth = Bounds.GetSize().X + (Padding * 2.0f);
            CommentNode->NodeHeight = Bounds.GetSize().Y + (Padding * 2.0f) + TitleBarHeight;
            
            // Format title with semantic intelligence
            FString TagName = Tag.ToString();
            int32 LastDot;
            if (TagName.FindLastChar('.', LastDot)) TagName = TagName.Mid(LastDot + 1);

            CommentNode->NodeComment = FString::Printf(TEXT("âœ¨ %s Logic"), *TagName);
            CommentNode->CommentColor = Color;

            Graph->AddNode(CommentNode, true, false);
        }
    }
}

void FBlueLineSmartTagAnalyzer::AnalyzeNodeSemantics(UEdGraphNode* Node, FSemanticContext& Context)
{
    if (!Node || Node->IsA<UK2Node_Knot>()) return;

    FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
    FString NodeClass = Node->GetClass()->GetName();

    // 1. Title/Name Heuristics (Expanded)
    if (NodeTitle.Contains(TEXT("Location")) || NodeTitle.Contains(TEXT("Rotation")) || 
        NodeTitle.Contains(TEXT("Velocity")) || NodeTitle.Contains(TEXT("Movement")) ||
        NodeTitle.Contains(TEXT("Transform")) || NodeTitle.Contains(TEXT("Physics")))
    {
        Context.AddWeight(ENodeSemantic::Movement, 1.5f);
    }

    if (NodeTitle.Contains(TEXT("Widget")) || NodeTitle.Contains(TEXT("UI")) || 
        NodeTitle.Contains(TEXT("Viewport")) || NodeTitle.Contains(TEXT("HUD")) ||
        NodeTitle.Contains(TEXT("Canvas")) || NodeTitle.Contains(TEXT("Slate")))
    {
        Context.AddWeight(ENodeSemantic::UI, 2.5f);
    }

    if (NodeTitle.Contains(TEXT("Damage")) || NodeTitle.Contains(TEXT("Health")) || 
        NodeTitle.Contains(TEXT("Attack")) || NodeTitle.Contains(TEXT("Weapon")) ||
        NodeTitle.Contains(TEXT("Projectile")) || NodeTitle.Contains(TEXT("Combat")))
    {
        Context.AddWeight(ENodeSemantic::Combat, 2.0f);
    }

    // 2. Variable Access Analysis
    if (UK2Node_Variable* VarNode = Cast<UK2Node_Variable>(Node))
    {
        FString VarName = VarNode->VariableReference.GetMemberName().ToString();
        if (VarName.Contains(TEXT("Health")) || VarName.Contains(TEXT("HP")) || VarName.Contains(TEXT("Stamina")))
        {
            Context.AddWeight(ENodeSemantic::Combat, 1.2f);
        }
        else if (VarName.Contains(TEXT("Score")) || VarName.Contains(TEXT("Inventory")) || VarName.Contains(TEXT("Data")))
        {
            Context.AddWeight(ENodeSemantic::Data, 1.5f);
        }
    }

    // 3. Pin Type / Math Density Analysis
    int32 FloatPins = 0;
    int32 VectorPins = 0;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float) FloatPins++;
        if (Pin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()) VectorPins++;
    }

    if (NodeClass.Contains(TEXT("K2Node_Math")) || NodeClass.Contains(TEXT("K2Node_CallFunction_Math")))
    {
        Context.AddWeight(ENodeSemantic::Logic, 0.8f);
        if (VectorPins > 0) Context.AddWeight(ENodeSemantic::Movement, 0.5f);
    }

    // --- HEURISTIC: Specific Demo Nodes ---
    if (NodeClass.Contains(TEXT("K2Node_KingSafety")))
    {
        Context.AddWeight(ENodeSemantic::Combat, 5.0f);
        Context.AddWeight(ENodeSemantic::AI, 3.0f);
    }
    if (NodeClass.Contains(TEXT("K2Node_AWSTag")))
    {
        Context.AddWeight(ENodeSemantic::Data, 5.0f);
        Context.AddWeight(ENodeSemantic::Networking, 3.0f);
    }

    // 4. Node Class Specific Logic
    if (Node->IsA<UK2Node_Timeline>())
    {
        Context.AddWeight(ENodeSemantic::Visuals, 2.0f);
        Context.AddWeight(ENodeSemantic::Movement, 1.0f); // Timelines are often for movement
    }

    if (UK2Node_CallFunction* CallFunc = Cast<UK2Node_CallFunction>(Node))
    {
        if (UFunction* Func = CallFunc->GetTargetFunction())
        {
            FString FuncName = Func->GetName();
            
            // Audio detection
            if (FuncName.Contains(TEXT("Sound")) || FuncName.Contains(TEXT("Audio")) || FuncName.Contains(TEXT("Acoustic")))
            {
                Context.AddWeight(ENodeSemantic::Audio, 4.0f);
            }
            
            // Networking detection
            if (FuncName.Contains(TEXT("Server")) || FuncName.Contains(TEXT("Multicast")) || 
                FuncName.Contains(TEXT("Reliable")) || FuncName.Contains(TEXT("Rep_")))
            {
                Context.AddWeight(ENodeSemantic::Networking, 5.0f);
            }

            // AI / Behavior detection
            if (FuncName.Contains(TEXT("AI")) || FuncName.Contains(TEXT("Behavior")) || 
                FuncName.Contains(TEXT("Blackboard")) || FuncName.Contains(TEXT("PawnSensing")))
            {
                Context.AddWeight(ENodeSemantic::AI, 3.5f);
            }
        }
    }

    // 5. Context-Aware (Neighbor Analysis)
    // If we are connected to a node that is strongly Combat, we might be combat too
    for (UEdGraphPin* Pin : Node->Pins)
    {
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            if (UEdGraphNode* Neighbor = LinkedPin->GetOwningNode())
            {
                FString NeighborTitle = Neighbor->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
                if (NeighborTitle.Contains(TEXT("Damage")) || NeighborTitle.Contains(TEXT("Kill")))
                {
                    Context.AddWeight(ENodeSemantic::Combat, 0.5f);
                }
            }
        }
    }
}

FGameplayTag FBlueLineSmartTagAnalyzer::MapSemanticToTag(ENodeSemantic Semantic)
{
    // These should ideally match your project's tag hierarchy.
    // If they don't exist, we might want to suggest creating them.
    switch (Semantic)
    {
        case ENodeSemantic::Movement:   return FGameplayTag::RequestGameplayTag("BlueLine.Type.Movement");
        case ENodeSemantic::Combat:     return FGameplayTag::RequestGameplayTag("BlueLine.Type.Combat");
        case ENodeSemantic::UI:         return FGameplayTag::RequestGameplayTag("BlueLine.Type.UI");
        case ENodeSemantic::Input:      return FGameplayTag::RequestGameplayTag("BlueLine.Type.Input");
        case ENodeSemantic::Networking: return FGameplayTag::RequestGameplayTag("BlueLine.Type.Networking");
        case ENodeSemantic::Audio:      return FGameplayTag::RequestGameplayTag("BlueLine.Type.Audio");
        case ENodeSemantic::Visuals:    return FGameplayTag::RequestGameplayTag("BlueLine.Type.Visuals");
        case ENodeSemantic::AI:         return FGameplayTag::RequestGameplayTag("BlueLine.Type.AI");
        case ENodeSemantic::Logic:      return FGameplayTag::RequestGameplayTag("BlueLine.Type.Logic");
        case ENodeSemantic::Data:       return FGameplayTag::RequestGameplayTag("BlueLine.Type.Data");
        default:                        return FGameplayTag::RequestGameplayTag("BlueLine.Type.Unknown");
    }
}

FLinearColor FBlueLineSmartTagAnalyzer::GetColorForSemantic(ENodeSemantic Semantic)
{
    switch (Semantic)
    {
        case ENodeSemantic::Movement:   return FLinearColor(0.0f, 0.8f, 1.0f); // Cyan/Sky
        case ENodeSemantic::Combat:     return FLinearColor(0.9f, 0.1f, 0.1f); // Red
        case ENodeSemantic::UI:         return FLinearColor(0.7f, 0.0f, 0.8f); // Purple/Magenta
        case ENodeSemantic::Input:      return FLinearColor(0.1f, 0.8f, 0.2f); // Green/Emerald
        case ENodeSemantic::Networking: return FLinearColor(1.0f, 0.5f, 0.0f); // Orange
        case ENodeSemantic::Audio:      return FLinearColor(1.0f, 0.9f, 0.0f); // Yellow
        case ENodeSemantic::Visuals:    return FLinearColor(1.0f, 0.2f, 0.6f); // Hot Pink
        case ENodeSemantic::AI:         return FLinearColor(0.1f, 0.2f, 0.8f); // Deep Blue
        case ENodeSemantic::Logic:      return FLinearColor(0.4f, 0.4f, 0.4f); // Gray
        case ENodeSemantic::Data:       return FLinearColor(0.8f, 0.8f, 0.8f); // White/Silver
        default:                        return FLinearColor::Gray;
    }
}

FText FBlueLineSmartTagAnalyzer::GetReasonForSemantic(ENodeSemantic Semantic, float Confidence)
{
    FString ConfidenceStr = (Confidence > 0.8f) ? TEXT("high") : (Confidence > 0.5f ? TEXT("moderate") : TEXT("low"));
    
    switch (Semantic)
    {
        case ENodeSemantic::Movement:   return FText::Format(LOCTEXT("ReasonMovement", "Detected spatial or velocity manipulation ({0} confidence)."), FText::FromString(ConfidenceStr));
        case ENodeSemantic::Combat:     return FText::Format(LOCTEXT("ReasonCombat", "Identified health or damage-related patterns ({0} confidence)."), FText::FromString(ConfidenceStr));
        case ENodeSemantic::UI:         return FText::Format(LOCTEXT("ReasonUI", "Nodes interact with the viewport or widgets ({0} confidence)."), FText::FromString(ConfidenceStr));
        case ENodeSemantic::Input:      return FText::Format(LOCTEXT("ReasonInput", "Cluster starts with user interaction events ({0} confidence)."), FText::FromString(ConfidenceStr));
        case ENodeSemantic::Networking: return FText::Format(LOCTEXT("ReasonNetworking", "Found network-replicated function calls ({0} confidence)."), FText::FromString(ConfidenceStr));
        case ENodeSemantic::AI:         return FText::Format(LOCTEXT("ReasonAI", "Logic involves behavior trees or pathfinding ({0} confidence)."), FText::FromString(ConfidenceStr));
        default:                        return LOCTEXT("ReasonGeneric", "Pattern matches established category markers.");
    }
}

#undef LOCTEXT_NAMESPACE
