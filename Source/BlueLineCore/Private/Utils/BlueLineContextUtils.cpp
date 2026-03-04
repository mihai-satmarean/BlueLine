// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Utils/BlueLineContextUtils.h"
#include "BlueLineLog.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SGraphPanel.h"
#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"

TSharedPtr<SGraphPanel> FBlueLineContextUtils::GetFocusedGraphPanel()
{
    TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (!FocusedWidget.IsValid())
    {
        return nullptr;
    }

    // Try to find SGraphPanel parent
    TSharedPtr<SWidget> CurrentWidget = FocusedWidget;
    int32 Depth = 0;

    while (CurrentWidget.IsValid() && Depth < 50)
    {
        // SAFETY: Verify type before casting to prevent undefined behavior
        const FName WidgetType = CurrentWidget->GetType();
        if (WidgetType == FName(TEXT("SGraphPanel")) || WidgetType.ToString().Contains(TEXT("GraphPanel")))
        {
            // Additional safety: Try to cast, but verify the pointer is valid
            TSharedPtr<SGraphPanel> GraphPanel = StaticCastSharedPtr<SGraphPanel>(CurrentWidget);
            if (GraphPanel.IsValid())
            {
                return GraphPanel;
            }
        }

        CurrentWidget = CurrentWidget->GetParentWidget();
        Depth++;
    }

    return nullptr;
}

TSharedPtr<SGraphEditor> FBlueLineContextUtils::GetFocusedGraphEditor()
{
    TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    if (!FocusedWidget.IsValid())
    {
        return nullptr;
    }

    TSharedPtr<SWidget> CurrentWidget = FocusedWidget;
    int32 Depth = 0;

    while (CurrentWidget.IsValid() && Depth < 50)
    {
        // SAFETY: Verify type before casting
        const FName WidgetType = CurrentWidget->GetType();
        if (WidgetType == FName(TEXT("SGraphEditor")) || WidgetType.ToString().Contains(TEXT("GraphEditor")))
        {
            TSharedPtr<SGraphEditor> GraphEditor = StaticCastSharedPtr<SGraphEditor>(CurrentWidget);
            if (GraphEditor.IsValid())
            {
                return GraphEditor;
            }
        }

        CurrentWidget = CurrentWidget->GetParentWidget();
        Depth++;
    }

    return nullptr;
}

UEdGraph* FBlueLineContextUtils::GetCurrentGraphFromFocus()
{
    UE_LOG(LogBlueLineCore, Verbose, TEXT("GetCurrentGraphFromFocus: Starting..."));
    
    // Try keyboard focus first
    TSharedPtr<SGraphPanel> GraphPanel = GetFocusedGraphPanel();
    if (GraphPanel.IsValid())
    {
        UE_LOG(LogBlueLineCore, Verbose, TEXT("GetCurrentGraphFromFocus: Found graph from SGraphPanel"));
        return GraphPanel->GetGraphObj();
    }

    // Fallback to GraphEditor
    TSharedPtr<SGraphEditor> GraphEditor = GetFocusedGraphEditor();
    if (GraphEditor.IsValid())
    {
        UE_LOG(LogBlueLineCore, Verbose, TEXT("GetCurrentGraphFromFocus: Found graph from SGraphEditor"));
        return GraphEditor->GetCurrentGraph();
    }

    // FINAL FALLBACK: Try to get from active tab/window
    // This handles cases where hotkey is pressed but focus detection fails
    UE_LOG(LogBlueLineCore, Verbose, TEXT("GetCurrentGraphFromFocus: Trying active tab fallback..."));
    UEdGraph* Graph = GetCurrentGraphFromActiveTab();
    if (Graph)
    {
        UE_LOG(LogBlueLineCore, Verbose, TEXT("GetCurrentGraphFromFocus: Found graph from active tab: %s"), *Graph->GetName());
    }
    else
    {
        UE_LOG(LogBlueLineCore, Warning, TEXT("GetCurrentGraphFromFocus: Could not find graph from any source!"));
    }
    return Graph;
}

UEdGraph* FBlueLineContextUtils::GetCurrentGraphFromActiveTab()
{
    // Try to find graph from the active editor window
    // This is useful when keyboard focus detection fails
    
    FSlateApplication& SlateApp = FSlateApplication::Get();
    
    // Get the active top-level window
    TSharedPtr<SWindow> ActiveWindow = SlateApp.GetActiveTopLevelWindow();
    if (!ActiveWindow.IsValid())
    {
        return nullptr;
    }
    
    // Search for graph widgets in the window
    TFunction<void(TSharedRef<SWidget>)> SearchForGraph;
    UEdGraph* FoundGraph = nullptr;
    
    SearchForGraph = [&](TSharedRef<SWidget> Widget)
    {
        if (FoundGraph)
        {
            return;
        }
        
        // Check if this is a graph panel
        const FName WidgetType = Widget->GetType();
        if (WidgetType.ToString().Contains(TEXT("GraphPanel")))
        {
            TSharedPtr<SWidget> WidgetPtr = Widget;
            TSharedPtr<SGraphPanel> GraphPanel = StaticCastSharedPtr<SGraphPanel>(WidgetPtr);
            if (GraphPanel.IsValid())
            {
                FoundGraph = GraphPanel->GetGraphObj();
                return;
            }
        }
        
        // Check if this is a graph editor
        if (WidgetType.ToString().Contains(TEXT("GraphEditor")))
        {
            TSharedPtr<SWidget> WidgetPtr = Widget;
            TSharedPtr<SGraphEditor> GraphEditor = StaticCastSharedPtr<SGraphEditor>(WidgetPtr);
            if (GraphEditor.IsValid())
            {
                FoundGraph = GraphEditor->GetCurrentGraph();
                return;
            }
        }
        
        // Recurse into children
        FChildren* Children = Widget->GetChildren();
        if (Children)
        {
            for (int32 i = 0; i < Children->Num(); ++i)
            {
                if (FoundGraph)
                {
                    return;
                }
                SearchForGraph(Children->GetChildAt(i));
            }
        }
    };
    
    SearchForGraph(ActiveWindow.ToSharedRef());
    return FoundGraph;
}

int32 FBlueLineContextUtils::GetSelectedNodesFromFocus(TArray<UEdGraphNode*>& OutSelectedNodes)
{
    OutSelectedNodes.Empty();

    // Try keyboard focus first
    TSharedPtr<SGraphPanel> GraphPanel = GetFocusedGraphPanel();
    
    // FALLBACK: If no focused panel, try to find from active tab
    if (!GraphPanel.IsValid())
    {
        if (UEdGraph* Graph = GetCurrentGraphFromActiveTab())
        {
            // We found a graph, but we need the SGraphPanel to get selection
            // Search for the graph panel that owns this graph
            FSlateApplication& SlateApp = FSlateApplication::Get();
            TSharedPtr<SWindow> ActiveWindow = SlateApp.GetActiveTopLevelWindow();
            if (ActiveWindow.IsValid())
            {
                TFunction<void(TSharedRef<SWidget>)> SearchForGraphPanel;
                SearchForGraphPanel = [&](TSharedRef<SWidget> Widget)
                {
                    if (GraphPanel.IsValid()) return;
                    
                    const FName WidgetType = Widget->GetType();
                    if (WidgetType.ToString().Contains(TEXT("GraphPanel")))
                    {
                        TSharedPtr<SWidget> WidgetPtr = Widget;
                        TSharedPtr<SGraphPanel> Panel = StaticCastSharedPtr<SGraphPanel>(WidgetPtr);
                        if (Panel.IsValid() && Panel->GetGraphObj() == Graph)
                        {
                            GraphPanel = Panel;
                            return;
                        }
                    }
                    
                    FChildren* Children = Widget->GetChildren();
                    if (Children)
                    {
                        for (int32 i = 0; i < Children->Num(); ++i)
                        {
                            SearchForGraphPanel(Children->GetChildAt(i));
                        }
                    }
                };
                SearchForGraphPanel(ActiveWindow.ToSharedRef());
            }
        }
    }
    
    if (!GraphPanel.IsValid())
    {
        return 0;
    }

    const FGraphPanelSelectionSet& Selection = GraphPanel->SelectionManager.GetSelectedNodes();
    for (UObject* Obj : Selection)
    {
        if (UEdGraphNode* Node = Cast<UEdGraphNode>(Obj))
        {
            OutSelectedNodes.Add(Node);
        }
    }

    return OutSelectedNodes.Num();
}

bool FBlueLineContextUtils::IsInBlueprintGraphContext()
{
    return GetCurrentGraphFromFocus() != nullptr;
}
