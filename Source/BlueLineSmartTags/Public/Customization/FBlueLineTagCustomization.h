// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Types/SlateEnums.h"
#include "Input/Reply.h"
#include "GameplayTagContainer.h"

class IPropertyHandle;
class SWidget;

/**
 * FBlueLineTagCustomization
 * 
 * Customizes the display of FGameplayTag properties in the Details Panel.
 * Instead of just showing the text string "Status.Fire", it renders a 
 * "BlueLineTagChip" widget that uses the color rules from the shared Theme Data.
 */
class FBlueLineTagCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	// New helpers that take Handles (since we are looking at child properties now)
	FText GetTagNameFromHandle(TSharedPtr<IPropertyHandle> TagHandle) const;
	FSlateColor GetColorFromProperty(TSharedPtr<IPropertyHandle> ColorHandle) const;

	FReply OnSuggestTagsClicked();
	TSharedRef<SWidget> GenerateSuggestionMenu() const;
	void ApplySuggestion(FGameplayTag Tag, FLinearColor Color) const;

	TSharedPtr<IPropertyHandle> StructPropertyHandle;
	TSharedPtr<IPropertyHandle> TagHandle;
	TSharedPtr<IPropertyHandle> ColorHandle;
};
