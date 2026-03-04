// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "BlueLineSmartTagsModule.h"
#include "BlueLineLog.h"
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
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "BlueLineSmartTags"

void FBlueLineSmartTagsModule::StartupModule()
{
	FModuleManager::Get().LoadModuleChecked<IModuleInterface>("GraphEditor");
	FBlueLineSmartTagCommands::Register();
	FBlueLineSmartTagMenuExtender::Register();
	RegisterCommands();
	RegisterPropertyTypeCustomizations();

	// Register Native Gameplay Tags
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Movement"), TEXT("Movement related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Combat"), TEXT("Combat related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.UI"), TEXT("UI related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Input"), TEXT("Input related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Networking"), TEXT("Networking related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Audio"), TEXT("Audio related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Visuals"), TEXT("Visuals related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.AI"), TEXT("AI related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Logic"), TEXT("Logic related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Data"), TEXT("Data related nodes"));
	TagManager.AddNativeGameplayTag(FName("BlueLine.Type.Unknown"), TEXT("Unknown tag category"));
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
		// SAFETY: Verify type string before casting
		const FName CurrentType = CurrentWidget->GetType();
		if (CurrentType.ToString().Contains(TEXT("GraphEditor")))
		{
			TSharedPtr<SGraphEditor> Editor = StaticCastSharedPtr<SGraphEditor>(CurrentWidget);
			if (Editor.IsValid())
			{
				GraphEditor = Editor;
				break;
			}
		}
		CurrentWidget = CurrentWidget->GetParentWidget();
		Depth++;
	}

	if (GraphEditor.IsValid())
	{
		UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		
		// Get selected nodes - only tag clusters containing selected nodes
		TArray<UEdGraphNode*> SelectedNodes;
		const FGraphPanelSelectionSet SelectedGraphNodes = GraphEditor->GetSelectedNodes();
		for (UObject* SelectedObject : SelectedGraphNodes)
		{
			if (UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
			{
				SelectedNodes.Add(Node);
				UE_LOG(LogBlueLineCore, Log, TEXT("BlueLine: Selected node '%s' at position (%d, %d)"), 
					*Node->GetNodeTitle(ENodeTitleType::ListView).ToString(), Node->NodePosX, Node->NodePosY);
			}
			else
			{
				UE_LOG(LogBlueLineCore, Warning, TEXT("BlueLine: Selected object is not a UEdGraphNode: %s"), 
					*SelectedObject->GetClass()->GetName());
			}
		}
		
		UE_LOG(LogBlueLineCore, Log, TEXT("BlueLine: Total selected UEdGraphNodes: %d"), SelectedNodes.Num());
		
		FBlueLineSmartTagAnalyzer::AutoTagGraph(Graph, SelectedNodes);
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
