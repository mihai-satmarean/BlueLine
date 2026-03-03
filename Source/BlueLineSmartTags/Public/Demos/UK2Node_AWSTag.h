// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "Data/UBlueLineThemeData.h"
#include "UK2Node_AWSTag.generated.h"

/**
 * A node based on the AWS Tagging snippet.
 * Designed to trigger the 'Data' and 'Networking' semantic categories.
 */
UCLASS()
class BLUELINESMARTTAGS_API UK2Node_AWSTag : public UK2Node
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Tagging")
	FBlueLineTagStyle ResourceTag;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
};
