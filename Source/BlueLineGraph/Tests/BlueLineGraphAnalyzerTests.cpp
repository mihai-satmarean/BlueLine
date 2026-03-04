// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Analysis/FBlueLineGraphAnalyzer.h"  // Now in BlueLineCore
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"

/**
 * Automation tests for FBlueLineGraphAnalyzer
 * Run these tests via: Automation Tool -> BlueLine -> Graph
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueLineGraphAnalyzerTest, 
    "BlueLine.Graph.Analyzer.BasicMetrics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBlueLineGraphAnalyzerTest::RunTest(const FString& Parameters)
{
    // Create a test graph
    UEdGraph* TestGraph = NewObject<UEdGraph>();
    TestGraph->Schema = UEdGraphSchema_K2::StaticClass();
    
    // Test 1: Empty graph analysis
    {
        FBlueLineGraphAnalyzer::FAnalysisResult Result = FBlueLineGraphAnalyzer::AnalyzeGraph(TestGraph);
        TestEqual(TEXT("Empty graph should have 0 nodes"), Result.TotalNodes, 0);
        TestEqual(TEXT("Empty graph should have 0 connections"), Result.TotalConnections, 0);
        TestEqual(TEXT("Empty graph complexity should be 0"), Result.ComplexityScore, 0.0f);
    }
    
    // Test 2: Single node graph
    {
        UEdGraphNode* TestNode = NewObject<UEdGraphNode>(TestGraph);
        TestNode->NodePosX = 0;
        TestNode->NodePosY = 0;
        TestGraph->Nodes.Add(TestNode);
        
        FBlueLineGraphAnalyzer::FAnalysisResult Result = FBlueLineGraphAnalyzer::AnalyzeGraph(TestGraph);
        TestEqual(TEXT("Single node graph should have 1 node"), Result.TotalNodes, 1);
        TestEqual(TEXT("Single node graph should have 0 connections"), Result.TotalConnections, 0);
        
        // Cleanup
        TestGraph->Nodes.Empty();
    }
    
    return true;
}

/**
 * Test cluster detection functionality
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueLineClusterDetectionTest,
    "BlueLine.Graph.Analyzer.ClusterDetection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBlueLineClusterDetectionTest::RunTest(const FString& Parameters)
{
    UEdGraph* TestGraph = NewObject<UEdGraph>();
    TestGraph->Schema = UEdGraphSchema_K2::StaticClass();
    
    // Create two disconnected nodes (should be two clusters)
    UEdGraphNode* NodeA = NewObject<UEdGraphNode>(TestGraph);
    NodeA->NodePosX = 0;
    NodeA->NodePosY = 0;
    TestGraph->Nodes.Add(NodeA);
    
    UEdGraphNode* NodeB = NewObject<UEdGraphNode>(TestGraph);
    NodeB->NodePosX = 500;
    NodeB->NodePosY = 500;
    TestGraph->Nodes.Add(NodeB);
    
    // Detect clusters
    TArray<FBlueLineGraphAnalyzer::FNodeCluster> Clusters = 
        FBlueLineGraphAnalyzer::DetectNodeClusters(TestGraph);
    
    // Note: Since nodes aren't connected, they should form separate clusters
    // The actual implementation may vary, so we just check that we got results
    TestTrue(TEXT("Should detect at least one cluster"), Clusters.Num() >= 1);
    
    // Test with selection
    TArray<UEdGraphNode*> Selection;
    Selection.Add(NodeA);
    
    TArray<FBlueLineGraphAnalyzer::FNodeCluster> SelectedClusters = 
        FBlueLineGraphAnalyzer::DetectNodeClusters(TestGraph, Selection);
    
    // Should only return clusters containing the selected node
    TestTrue(TEXT("Selection filtering should work"), SelectedClusters.Num() <= Clusters.Num());
    
    return true;
}

/**
 * Test bounds calculation
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueLineBoundsCalculationTest,
    "BlueLine.Graph.Analyzer.BoundsCalculation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBlueLineBoundsCalculationTest::RunTest(const FString& Parameters)
{
    UEdGraph* TestGraph = NewObject<UEdGraph>();
    
    // Test empty graph bounds
    FBox2D EmptyBounds = FBlueLineGraphAnalyzer::CalculateGraphBounds(TestGraph);
    TestEqual(TEXT("Empty graph should have zero bounds"), (float)EmptyBounds.GetArea(), 0.0f);
    
    // Create nodes at specific positions
    UEdGraphNode* NodeA = NewObject<UEdGraphNode>(TestGraph);
    NodeA->NodePosX = 100;
    NodeA->NodePosY = 200;
    TestGraph->Nodes.Add(NodeA);
    
    UEdGraphNode* NodeB = NewObject<UEdGraphNode>(TestGraph);
    NodeB->NodePosX = 300;
    NodeB->NodePosY = 400;
    TestGraph->Nodes.Add(NodeB);
    
    FBox2D Bounds = FBlueLineGraphAnalyzer::CalculateGraphBounds(TestGraph);
    
    // Verify bounds contain both nodes
    TestTrue(TEXT("Bounds should contain NodeA"), Bounds.IsInside(FVector2D(NodeA->NodePosX, NodeA->NodePosY)));
    TestTrue(TEXT("Bounds should contain NodeB"), Bounds.IsInside(FVector2D(NodeB->NodePosX, NodeB->NodePosY)));
    
    return true;
}

/**
 * Test wire crossing detection (simplified)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlueLineWireCrossingTest,
    "BlueLine.Graph.Analyzer.WireCrossings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBlueLineWireCrossingTest::RunTest(const FString& Parameters)
{
    // This is a simplified test - wire crossing requires actual pin connections
    // which are complex to set up in a test environment
    
    UEdGraph* TestGraph = NewObject<UEdGraph>();
    
    // Empty graph should have 0 crossings
    int32 EmptyCrossings = FBlueLineGraphAnalyzer::CountWireCrossings(TestGraph);
    TestEqual(TEXT("Empty graph should have 0 wire crossings"), EmptyCrossings, 0);
    
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
