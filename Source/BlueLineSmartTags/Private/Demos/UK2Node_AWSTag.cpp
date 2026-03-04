// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Demos/UK2Node_AWSTag.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "K2Node_AWSTag"

void UK2Node_AWSTag::AllocateDefaultPins()
{
	// FIX: Clear existing pins before creating new ones to prevent duplication
	// when node is reconstructed (e.g., during compilation or undo/redo)
	Pins.Reset();
	
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("ResourceARN"));
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("ResourceName"));
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, TEXT("BillingGroup"));
	
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Boolean, TEXT("bIsTagged"));
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_String, TEXT("ResultMetadata"));
}

FText UK2Node_AWSTag::GetNodeTitle(ENodeTitleType::Type TitleType) const { return LOCTEXT("Title", "AWS Resource Tag"); }
FText UK2Node_AWSTag::GetTooltipText() const { return LOCTEXT("Tooltip", "Handles cloud resource tagging metadata."); }
FLinearColor UK2Node_AWSTag::GetNodeTitleColor() const { return FLinearColor(0.1f, 0.4f, 0.8f); }
FText UK2Node_AWSTag::GetMenuCategory() const { return LOCTEXT("Category", "BlueLine|Demos"); }

void UK2Node_AWSTag::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

void UK2Node_AWSTag::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
	BreakAllNodeLinks();
}
#undef LOCTEXT_NAMESPACE
