// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "BlueLineGraphModule.h"

// Visualization Includes
#include "Drawing/FBlueLineGraphPanelFactory.h"
#include "Drawing/FBlueLineGraphPinFactory.h"
#include "Formatting/BlueLineFormatter.h"
#include "Styles/FBlueLineStyle.h"
#include "Settings/UBlueLineEditorSettings.h"

// Routing & UI Includes
#include "Routing/FBlueLineManhattanRouter.h"
#include "Routing/FBlueLineConnectionInterceptor.h"
#include "Formatting/FBlueLineGraphCleaner.h"
#include "UI/FBlueLineGraphMenuExtender.h" // <--- Valid here

// Framework Includes
#include "Commands/FBlueLineCommands.h"
#include "BlueLineLog.h" 
#include "Modules/ModuleManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Framework/Commands/UICommandList.h"
#include "EdGraphUtilities.h"

#define LOCTEXT_NAMESPACE "BlueLineGraph"

void FBlueLineGraphModule::StartupModule()
{
	FBlueLineStyle::Initialize();
	FBlueLineCommands::Register();
	RegisterCommands();

	// Factories
	InstallGraphDrawingPolicy();
	InstallGraphPinFactory();

	// Menu Extension
	FBlueLineGraphMenuExtender::Register(); // <--- Init Context Menu

	FBlueLineConnectionInterceptor::Enable();

	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineGraph: Module Started."));
}

void FBlueLineGraphModule::ShutdownModule()
{
	FBlueLineConnectionInterceptor::Disable();

	// Cleanup Menus
	FBlueLineGraphMenuExtender::Unregister();

	// Cleanup Factories
	UninstallGraphPinFactory();
	UninstallGraphDrawingPolicy();

	if (PluginCommands.IsValid())
	{
		PluginCommands.Reset();
	}

	FBlueLineCommands::Unregister();
	FBlueLineStyle::Shutdown();
}

void FBlueLineGraphModule::RegisterCommands()
{
	PluginCommands = MakeShareable(new FUICommandList);

	// Shift+Q (Format)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().AutoFormatSelected,
		FExecuteAction::CreateStatic(&FBlueLineFormatter::FormatActiveGraphSelection)
	);

	// Shift+R (Manhattan Router)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().RigidifyConnections,
		FExecuteAction::CreateStatic(&FBlueLineManhattanRouter::RigidifySelectedConnections)
	);

	// Shift+C (Clean Graph)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().CleanGraph,
		FExecuteAction::CreateStatic(&FBlueLineGraphCleaner::CleanActiveGraph)
	);

	// F8 Toggle (Optional/Legacy)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().ToggleWireStyle,
		FExecuteAction::CreateLambda([]() {
			UBlueLineEditorSettings* S = GetMutableDefault<UBlueLineEditorSettings>();
			S->bEnableManhattanRouting = !S->bEnableManhattanRouting;
			S->PostEditChange();
			S->SaveConfig();
			})
	);

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		MainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());
	}
}

void FBlueLineGraphModule::InstallGraphDrawingPolicy()
{
	if (BlueLineGraphPanelFactory.IsValid()) return;
	// Use explicit template to satisfy MakeShareable with the struct FGraphPanelNodeFactory
	BlueLineGraphPanelFactory = MakeShareable<FGraphPanelNodeFactory>(new FBlueLineGraphPanelFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(BlueLineGraphPanelFactory);
}

void FBlueLineGraphModule::UninstallGraphDrawingPolicy()
{
	if (BlueLineGraphPanelFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(BlueLineGraphPanelFactory);
		BlueLineGraphPanelFactory.Reset();
	}
}

void FBlueLineGraphModule::InstallGraphPinFactory()
{
	if (BlueLinePinFactory.IsValid()) return;
	BlueLinePinFactory = MakeShareable(new FBlueLineGraphPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(BlueLinePinFactory);
}

void FBlueLineGraphModule::UninstallGraphPinFactory()
{
	if (BlueLinePinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(BlueLinePinFactory);
		BlueLinePinFactory.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FBlueLineGraphModule, BlueLineGraph)
