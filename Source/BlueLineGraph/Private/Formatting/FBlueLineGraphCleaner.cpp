// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Formatting/FBlueLineGraphCleaner.h"
#include "Analysis/FBlueLineGraphAnalyzer.h"
#include "Settings/UBlueLineEditorSettings.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "ScopedTransaction.h"
#include "Framework/Application/SlateApplication.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "K2Node_Knot.h"

#define LOCTEXT_NAMESPACE "BlueLineGraphCleaner"

void FBlueLineGraphCleaner::CleanActiveGraph()
{
    UEdGraph* ActiveGraph = GetActiveGraph();
    if (ActiveGraph)
    {
        CleanGraph(ActiveGraph);
    }
}

void FBlueLineGraphCleaner::CleanGraph(UEdGraph* Graph)
{
    if (!Graph) return;

    FScopedTransaction Transaction(LOCTEXT("CleanGraphTrans", "BlueLine: Clean Graph"));
    
    // 1. Analyze
    FBlueLineGraphAnalyzer::FAnalysisResult Analysis = FBlueLineGraphAnalyzer::AnalyzeGraph(Graph);
    
    // 2. Identify Connected Components (Islands)
    // We treat execution wires as the primary backbone, data wires as secondary.
    TArray<TArray<UEdGraphNode*>> Islands;
    TSet<UEdGraphNode*> ProcessedNodes;

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || ProcessedNodes.Contains(Node)) continue;

        TArray<UEdGraphNode*> Island;
        TArray<UEdGraphNode*> Stack;
        Stack.Push(Node);

        while (Stack.Num() > 0)
        {
            UEdGraphNode* Current = Stack.Pop();
            if (ProcessedNodes.Contains(Current)) continue;

            Island.Add(Current);
            ProcessedNodes.Add(Current);

            for (UEdGraphPin* Pin : Current->Pins)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (UEdGraphNode* Neighbor = LinkedPin ? LinkedPin->GetOwningNode() : nullptr)
                    {
                        if (!ProcessedNodes.Contains(Neighbor))
                        {
                            Stack.Push(Neighbor);
                        }
                    }
                }
            }
        }
        Islands.Add(Island);
    }

    const UBlueLineEditorSettings* Settings = GetDefault<UBlueLineEditorSettings>();
    const float HorizontalSpacing = Settings ? Settings->FormatterPadding * 2.5f : 300.0f;
    const float VerticalSpacing = 120.0f;

    float CurrentIslandY = 0.0f;

    // 3. Process each Island
    for (const TArray<UEdGraphNode*>& Island : Islands)
    {
        if (Island.Num() == 0) continue;

        // Find "Root" nodes for this island (inputs or nodes with no incoming execution wires)
        TArray<UEdGraphNode*> Roots;
        for (UEdGraphNode* Node : Island)
        {
            bool bHasIncomingExec = false;
            for (UEdGraphPin* Pin : Node->Pins)
            {
                if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == TEXT("exec") && Pin->LinkedTo.Num() > 0)
                {
                    bHasIncomingExec = true;
                    break;
                }
            }
            if (!bHasIncomingExec) Roots.Add(Node);
        }
        
        // If no clear roots (e.g. data loop or pure isolated nodes), pick the leftmost one
        if (Roots.Num() == 0) Roots.Add(Island[0]);

        // Rank Assignment (BFS-based)
        TMap<UEdGraphNode*, int32> NodeRanks;
        TArray<UEdGraphNode*> Queue;
        for (UEdGraphNode* Root : Roots)
        {
            Queue.Add(Root);
            NodeRanks.Add(Root, 0);
        }

        while (Queue.Num() > 0)
        {
            UEdGraphNode* Current = Queue[0];
            Queue.RemoveAt(0);
            int32 Rank = NodeRanks[Current];

            for (UEdGraphPin* Pin : Current->Pins)
            {
                if (Pin->Direction == EGPD_Output)
                {
                    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                    {
                        if (UEdGraphNode* Neighbor = LinkedPin->GetOwningNode())
                        {
                            if (!NodeRanks.Contains(Neighbor) || NodeRanks[Neighbor] < Rank + 1)
                            {
                                NodeRanks.Add(Neighbor, Rank + 1);
                                Queue.AddUnique(Neighbor);
                            }
                        }
                    }
                }
            }
        }

        // Layout within Island
        TMap<int32, TArray<UEdGraphNode*>> RankGroups;
        for (UEdGraphNode* Node : Island)
        {
            int32 Rank = NodeRanks.Contains(Node) ? NodeRanks[Node] : 0;
            RankGroups.FindOrAdd(Rank).Add(Node);
        }

        // --- UPGRADE: Genetic Algorithm for Crossing Minimization ---
        EvolutionaryCrossingMinimizer(RankGroups, Graph);

        float IslandMaxHeight = 0.0f;
        for (auto& KVP : RankGroups)
        {
            int32 Rank = KVP.Key;
            TArray<UEdGraphNode*>& NodesInRank = KVP.Value;

            for (int32 i = 0; i < NodesInRank.Num(); ++i)
            {
                UEdGraphNode* Node = NodesInRank[i];
                Node->Modify();
                Node->NodePosX = Rank * HorizontalSpacing;
                Node->NodePosY = CurrentIslandY + (i * VerticalSpacing);
                
                IslandMaxHeight = FMath::Max(IslandMaxHeight, (float)(i + 1) * VerticalSpacing);
            }
        }

        CurrentIslandY += IslandMaxHeight + (VerticalSpacing * 2.0f);
    }

    Graph->NotifyGraphChanged();
    
    UE_LOG(LogTemp, Log, TEXT("BlueLine: Cleaned %d nodes in %d islands using Evolutionary Optimization."), Analysis.TotalNodes, Islands.Num());
}

void FBlueLineGraphCleaner::EvolutionaryCrossingMinimizer(TMap<int32, TArray<UEdGraphNode*>>& RankGroups, UEdGraph* Graph)
{
    // Genetic Algorithm Parameters
    const int32 PopulationSize = 30;
    const int32 MaxGenerations = 40;
    const float MutationRate = 0.15f;

    struct FIndividual
    {
        TMap<int32, TArray<UEdGraphNode*>> Chromosome;
        int32 Fitness = 0;

        void CalculateFitness()
        {
            Fitness = 0;
            TArray<int32> Ranks;
            Chromosome.GetKeys(Ranks);
            Ranks.Sort();

            // Check crossings between adjacent ranks
            for (int32 i = 0; i < Ranks.Num() - 1; ++i)
            {
                const TArray<UEdGraphNode*>& RankA = Chromosome[Ranks[i]];
                const TArray<UEdGraphNode*>& RankB = Chromosome[Ranks[i+1]];

                TMap<UEdGraphNode*, int32> PosA;
                for (int32 j = 0; j < RankA.Num(); ++j) PosA.Add(RankA[j], j);
                
                TMap<UEdGraphNode*, int32> PosB;
                for (int32 j = 0; j < RankB.Num(); ++j) PosB.Add(RankB[j], j);

                // Find all edges between RankA and RankB
                struct FEdge { int32 u; int32 v; };
                TArray<FEdge> Edges;

                for (UEdGraphNode* NodeA : RankA)
                {
                    for (UEdGraphPin* Pin : NodeA->Pins)
                    {
                        if (Pin->Direction == EGPD_Output)
                        {
                            for (UEdGraphPin* LP : Pin->LinkedTo)
                            {
                                if (UEdGraphNode* NodeB = LP->GetOwningNode())
                                {
                                    if (PosB.Contains(NodeB))
                                    {
                                        Edges.Add({PosA[NodeA], PosB[NodeB]});
                                    }
                                }
                            }
                        }
                    }
                }

                // Count crossings
                for (int32 j = 0; j < Edges.Num(); ++j)
                {
                    for (int32 k = j + 1; k < Edges.Num(); ++k)
                    {
                        const FEdge& E1 = Edges[j];
                        const FEdge& E2 = Edges[k];

                        if ((E1.u < E2.u && E1.v > E2.v) || (E1.u > E2.u && E1.v < E2.v))
                        {
                            Fitness++;
                        }
                    }
                }
            }
        }
    };

    TArray<FIndividual> Population;

    // 1. Initial Population
    // First individual is the Barycenter result (good starting point)
    FIndividual Initial;
    Initial.Chromosome = RankGroups;
    for (auto& KVP : Initial.Chromosome)
    {
        KVP.Value.Sort([&](const UEdGraphNode& A, const UEdGraphNode& B) {
            auto GetAvgParentY = [&](const UEdGraphNode& Node) {
                float SumY = 0.0f; int32 Count = 0;
                for (UEdGraphPin* Pin : Node.Pins) {
                    if (Pin->Direction == EGPD_Input) {
                        for (UEdGraphPin* LP : Pin->LinkedTo) { SumY += LP->GetOwningNode()->NodePosY; Count++; }
                    }
                }
                return Count > 0 ? SumY / Count : 0.0f;
            };
            return GetAvgParentY(A) < GetAvgParentY(B);
        });
    }
    Initial.CalculateFitness();
    Population.Add(Initial);

    // Fill rest with random permutations
    for (int32 i = 1; i < PopulationSize; ++i)
    {
        FIndividual Ind;
        Ind.Chromosome = RankGroups;
        for (auto& KVP : Ind.Chromosome)
        {
            // Fisher-Yates shuffle
            for (int32 j = KVP.Value.Num() - 1; j > 0; --j)
            {
                int32 k = FMath::RandRange(0, j);
                KVP.Value.Swap(j, k);
            }
        }
        Ind.CalculateFitness();
        Population.Add(Ind);
    }

    // 2. Evolution Loop
    for (int32 Gen = 0; Gen < MaxGenerations; ++Gen)
    {
        Population.Sort([](const FIndividual& A, const FIndividual& B) { return A.Fitness < B.Fitness; });
        
        // If we hit 0 crossings, we're done
        if (Population[0].Fitness == 0) break;

        TArray<FIndividual> NewPopulation;
        // Elitism: Keep top 2
        NewPopulation.Add(Population[0]);
        NewPopulation.Add(Population[1]);

        while (NewPopulation.Num() < PopulationSize)
        {
            // Selection (Tournament)
            auto Select = [&]() -> const FIndividual& {
                int32 i1 = FMath::RandRange(0, PopulationSize / 2);
                int32 i2 = FMath::RandRange(0, PopulationSize / 2);
                return (Population[i1].Fitness < Population[i2].Fitness) ? Population[i1] : Population[i2];
            };

            const FIndividual& Parent1 = Select();
            const FIndividual& Parent2 = Select();

            // Crossover (Uniform Rank Crossover)
            FIndividual Child;
            Child.Chromosome = RankGroups;
            for (auto& KVP : RankGroups)
            {
                int32 Rank = KVP.Key;
                // Randomly take rank ordering from Parent1 or Parent2
                Child.Chromosome[Rank] = (FMath::RandBool()) ? Parent1.Chromosome[Rank] : Parent2.Chromosome[Rank];
            }

            // Mutation (Swap Mutation)
            if (FMath::FRand() < MutationRate)
            {
                TArray<int32> RankKeys;
                Child.Chromosome.GetKeys(RankKeys);
                int32 RandRank = RankKeys[FMath::RandRange(0, RankKeys.Num() - 1)];
                TArray<UEdGraphNode*>& Nodes = Child.Chromosome[RandRank];
                if (Nodes.Num() > 1)
                {
                    Nodes.Swap(FMath::RandRange(0, Nodes.Num() - 1), FMath::RandRange(0, Nodes.Num() - 1));
                }
            }

            Child.CalculateFitness();
            NewPopulation.Add(Child);
        }
        Population = NewPopulation;
    }

    // Apply best result
    Population.Sort([](const FIndividual& A, const FIndividual& B) { return A.Fitness < B.Fitness; });
    RankGroups = Population[0].Chromosome;
}

UEdGraph* FBlueLineGraphCleaner::GetActiveGraph()
{
    TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (!FocusedWidget.IsValid()) return nullptr;

    TSharedPtr<SGraphEditor> GraphEditor;
    TSharedPtr<SWidget> CurrentWidget = FocusedWidget;

    int32 Depth = 0;
    while (CurrentWidget.IsValid() && Depth < 50)
    {
        if (CurrentWidget->GetType().ToString().Contains(TEXT("GraphEditor")))
        {
            GraphEditor = StaticCastSharedPtr<SGraphEditor>(CurrentWidget);
            break;
        }
        CurrentWidget = CurrentWidget->GetParentWidget();
        Depth++;
    }

    if (GraphEditor.IsValid())
    {
        return GraphEditor->GetCurrentGraph();
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE
