// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Routing/FBlueLineConnectionInterceptor.h"
#include "Routing/FBlueLineManhattanRouter.h"
#include "Settings/UBlueLineEditorSettings.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"
#include "K2Node_Knot.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintUtilities.h" // For FEdGraphEditAction

bool FBlueLineConnectionInterceptor::bIsEnabled = false;
TArray<FDelegateHandle> FBlueLineConnectionInterceptor::GraphChangedDelegateHandles;
static FDelegateHandle GlobalObjectModifiedDelegateHandle;

void FBlueLineConnectionInterceptor::Enable()
{
    if (bIsEnabled)
    {
        return;
    }

    // FBlueprintEditorUtils::OnGraphChanged does not exist as a static delegate.
    // We use OnObjectModified to detect graph changes. This is high-traffic, so efficient filtering is key.
    GlobalObjectModifiedDelegateHandle = FCoreUObjectDelegates::OnObjectModified.AddStatic(&FBlueLineConnectionInterceptor::OnObjectModified);
    
    bIsEnabled = true;
    UE_LOG(LogTemp, Log, TEXT("BlueLine: Auto-routing enabled"));
}

void FBlueLineConnectionInterceptor::Disable()
{
    if (!bIsEnabled)
    {
        return;
    }

    FCoreUObjectDelegates::OnObjectModified.Remove(GlobalObjectModifiedDelegateHandle);
    GlobalObjectModifiedDelegateHandle.Reset();

    bIsEnabled = false;
    
    UE_LOG(LogTemp, Log, TEXT("BlueLine: Auto-routing disabled"));
}

void FBlueLineConnectionInterceptor::OnObjectModified(UObject* Object)
{
    if (!bIsEnabled || !Object)
    {
        return;
    }

    // We only care about Graphs being modified
    UEdGraph* Graph = Cast<UEdGraph>(Object);
    if (!Graph)
    {
        return;
    }

    // Scan the graph for new connections
    // This looks expensive but we can optimize by only checking if necessary
    // For now, let's do a safe scan
    
    // Iterate over all nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (!Node || Node->IsA(UK2Node_Knot::StaticClass()))
        {
            continue;
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction != EGPD_Output)
            {
                continue;
            }

            TArray<UEdGraphPin*> LinkedToCopy = Pin->LinkedTo;
            for (UEdGraphPin* LinkedPin : LinkedToCopy)
            {
                // Safety check
                if (!LinkedPin) continue;

                UEdGraphNode* OtherNode = LinkedPin->GetOwningNode();
                if (!OtherNode || OtherNode->IsA(UK2Node_Knot::StaticClass()))
                {
                    continue;
                }

                // Found a direct connection between two non-knot nodes
                OnPinConnectionCreated(Pin, LinkedPin);
            }
        }
    }
}

void FBlueLineConnectionInterceptor::OnPinConnectionCreated(UEdGraphPin* PinA, UEdGraphPin* PinB)
{
    if (!bIsEnabled || !PinA || !PinB)
    {
        return;
    }

    const UBlueLineEditorSettings* Settings = GetDefault<UBlueLineEditorSettings>();
    if (!Settings || !Settings->bAutoRouteNewConnections)
    {
        return;
    }

    // Determine which is output/input
    UEdGraphPin* OutputPin = (PinA->Direction == EGPD_Output) ? PinA : PinB;
    UEdGraphPin* InputPin = (PinA->Direction == EGPD_Input) ? PinA : PinB;

    if (OutputPin->Direction != EGPD_Output || InputPin->Direction != EGPD_Input)
    {
        return; // Invalid connection
    }

    UEdGraph* Graph = OutputPin->GetOwningNode()->GetGraph();
    if (!Graph)
    {
        return;
    }

    // Break the direct connection
    OutputPin->BreakLinkTo(InputPin);

    // Apply Manhattan routing
    FBlueLineManhattanRouter::RouteConnection(
        OutputPin,
        InputPin,
        Graph
    );

    UE_LOG(LogTemp, Log, TEXT("BlueLine: Auto-routed new connection"));
}
