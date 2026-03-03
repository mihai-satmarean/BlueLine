// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "BlueLineSmartTagsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Data/UBlueLineThemeData.h"
#include "Customization/FBlueLineTagCustomization.h"
#include "UI/FBlueLineSmartTagMenuExtender.h"
#include "FBlueLineSmartTagCommands.h"
#include "FBlueLineSmartTagAnalyzer.h"
#include "Framework/Application/SlateApplication.h"
#include "GraphEditor.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Commands/UICommandList.h"
#include "Interfaces/IMainFrameModule.h"

#define LOCTEXT_NAMESPACE "BlueLineSmartTags"

void FBlueLineSmartTagsModule::StartupModule()
{
	FModuleManager::Get().LoadModuleChecked<IModuleInterface>("GraphEditor");
	FBlueLineSmartTagCommands::Register();
	FBlueLineSmartTagMenuExtender::Register();
	RegisterCommands();
	RegisterPropertyTypeCustomizations();
}

void FBlueLineSmartTagsModule::ShutdownModule()
{
	UnregisterPropertyTypeCustomizations();
	FBlueLineSmartTagMenuExtender::Unregister();
	FBlueLineSmartTagCommands::Unregister();
}

void FBlueLineSmartTagsModule::RegisterCommands()
{
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FBlueLineSmartTagCommands::Get().AutoTagGraph,
		FExecuteAction::CreateRaw(this, &FBlueLineSmartTagsModule::ExecuteAutoTagGraph)
	);

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		MainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());
	}
}

void FBlueLineSmartTagsModule::ExecuteAutoTagGraph()
{
	// We use the "Maximal IQ" analyzer to find the graph from focus
	TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
	if (!FocusedWidget.IsValid()) return;

	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<SWidget> CurrentWidget = FocusedWidget;

	int32 Depth = 0;
	while (CurrentWidget.IsValid() && Depth < 50)
	{
		if (CurrentWidget->GetType().ToString().Contains(TEXT("GraphEditor")))
		{
			GraphEditor = StaticCastSharedPtr<SGraphEditor>(CurrentWidget);
			break;
		}
		CurrentWidget = CurrentWidget->GetParentWidget();
		Depth++;
	}

	if (GraphEditor.IsValid())
	{
		UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		FBlueLineSmartTagAnalyzer::AutoTagGraph(Graph);
	}
}

void FBlueLineSmartTagsModule::RegisterPropertyTypeCustomizations()
{
	if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor")) return;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Customize our Theme Style struct only.
	// We leave FGameplayTag alone so the engine defaults handle the dropdown logic.
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FBlueLineTagStyle::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBlueLineTagCustomization::MakeInstance)
	);
}

void FBlueLineSmartTagsModule::UnregisterPropertyTypeCustomizations()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FBlueLineTagStyle::StaticStruct()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FBlueLineSmartTagsModule, BlueLineSmartTags)
