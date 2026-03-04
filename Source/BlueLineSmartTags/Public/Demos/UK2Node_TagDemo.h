// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Data/UBlueLineThemeData.h"
#include "UK2Node_TagDemo.generated.h"

/**
 * A demo node that holds a Smart Tag.
 * Demonstrates how properties within a node use the same Smart Customization.
 */
UCLASS()
class BLUELINESMARTTAGS_API UK2Node_TagDemo : public UK2Node
{
	GENERATED_BODY()

public:
	/** 
	 * A tag stored directly inside the node. 
	 * Select this node in the graph to see the ✨ button in the Details panel.
	 */
	UPROPERTY(EditAnywhere, Category = "Tagging")
	FBlueLineTagStyle NodeSemanticTag;

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	// End of UK2Node interface
};
