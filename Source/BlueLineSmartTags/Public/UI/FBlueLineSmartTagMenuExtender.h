// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FExtender;
class FUICommandList;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;

class FBlueLineSmartTagMenuExtender
{
public:
    static void Register();
    static void Unregister();

private:
    static TSharedRef<FExtender> OnExtendContextMenu(const TSharedRef<FUICommandList> CommandList, const UEdGraph* Graph, const UEdGraphNode* Node, const UEdGraphPin* Pin, bool bIsDebug);
    static void AddGraphMenuEntries(class FMenuBuilder& MenuBuilder, UEdGraph* Graph);
    static void ExecuteSpawnMessyDemo(UEdGraph* Graph);
    static void ExecuteSpawnMessyDemoFromMenu();
};
