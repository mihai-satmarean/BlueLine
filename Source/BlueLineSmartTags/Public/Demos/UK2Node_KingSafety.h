// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Data/UBlueLineThemeData.h"
#include "UK2Node_KingSafety.generated.h"

/**
 * A complex node based on the King Safety snippet.
 * Designed to trigger the 'Combat' and 'AI' semantic categories.
 */
UCLASS()
class BLUELINESMARTTAGS_API UK2Node_KingSafety : public UK2Node
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Tagging")
	FBlueLineTagStyle NodeTag;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

	/** Helpers to get pins */
	UEdGraphPin* GetInputPin() const;
	UEdGraphPin* GetThenPin() const;
};
