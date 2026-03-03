// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "Demos/UK2Node_TagDemo.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"

#define LOCTEXT_NAMESPACE "K2Node_TagDemo"

void UK2Node_TagDemo::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
}

FText UK2Node_TagDemo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "BlueLine Tag Demo Node");
}

FText UK2Node_TagDemo::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "A demo node containing a Smart Tag property.");
}

FLinearColor UK2Node_TagDemo::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.5f, 0.2f);
}

FText UK2Node_TagDemo::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "BlueLine|Demos");
}

void UK2Node_TagDemo::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

#undef LOCTEXT_NAMESPACE
