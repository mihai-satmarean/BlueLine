// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "BlueLineGraphModule.h"

// Visualization Includes
#include "Drawing/FBlueLineGraphPanelFactory.h"
#include "Drawing/FBlueLineGraphPinFactory.h"
#include "Formatting/BlueLineFormatter.h"
#include "Formatting/FBlueLineMultiModeFormatter.h"
#include "Styles/FBlueLineStyle.h"  // From BlueLineCore (shared)
#include "BlueLineCore/Public/Settings/UBlueLineEditorSettings.h"

// Routing & UI Includes
#include "Routing/FBlueLineManhattanRouter.h"
#include "Routing/FBlueLineConnectionInterceptor.h"
#include "Routing/FBlueLineWireSnapper.h"
#include "Formatting/FBlueLineGraphCleaner.h"
#include "UI/FBlueLineGraphMenuExtender.h"

// Framework Includes
#include "Commands/FBlueLineCommands.h"
#include "BlueLineLog.h" 
#include "Modules/ModuleManager.h"
#include "Interfaces/IMainFrameModule.h"
#include "Framework/Commands/UICommandList.h"
#include "EdGraphUtilities.h"

// For dialogs
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "BlueLineGraph"

void FBlueLineGraphModule::StartupModule()
{
	// Note: FBlueLineStyle is initialized by BlueLineCore (shared style system)
	FBlueLineCommands::Register();
	RegisterCommands();

	// Factories
	InstallGraphDrawingPolicy();
	InstallGraphPinFactory();

	// Menu Extension
	FBlueLineGraphMenuExtender::Register(); // <--- Init Context Menu

	FBlueLineConnectionInterceptor::Enable();
	FBlueLineWireSnapper::Enable();

	// Unregister Slate-dependent components BEFORE FSlateApplication is destroyed.
	// ShutdownModule() is called too late (after Slate teardown begins), so we hook
	// OnPreShutdown to guarantee a safe unregister window.
	if (FSlateApplication::IsInitialized())
	{
		SlatePreShutdownHandle = FSlateApplication::Get().OnPreShutdown().AddRaw(
			this, &FBlueLineGraphModule::DisableSlateComponents);
	}

	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineGraph: Module Started."));
}

void FBlueLineGraphModule::DisableSlateComponents()
{
	// Remove the pre-shutdown binding (if still registered)
	if (SlatePreShutdownHandle.IsValid())
	{
		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().OnPreShutdown().Remove(SlatePreShutdownHandle);
		}
		SlatePreShutdownHandle.Reset();
	}

	// These are idempotent — safe to call even if already disabled
	FBlueLineConnectionInterceptor::Disable();
	FBlueLineWireSnapper::Disable();
}

void FBlueLineGraphModule::ShutdownModule()
{
	// DisableSlateComponents was already called via OnPreShutdown during normal shutdown.
	// Call again as a safety net for unusual teardown paths — both are idempotent.
	DisableSlateComponents();

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
	// Note: FBlueLineStyle is shut down by BlueLineCore (shared style system)
}

void FBlueLineGraphModule::RegisterCommands()
{
	PluginCommands = MakeShareable(new FUICommandList);
	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineGraph: Registering commands..."));

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

	// Multi-mode formatter (Shift+X / Shift+V / Shift+B)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().FormatStraight,
		FExecuteAction::CreateStatic(&FBlueLineMultiModeFormatter::FormatStraight)
	);
	PluginCommands->MapAction(
		FBlueLineCommands::Get().FormatCenter,
		FExecuteAction::CreateStatic(&FBlueLineMultiModeFormatter::FormatCenter)
	);
	PluginCommands->MapAction(
		FBlueLineCommands::Get().FormatCompact,
		FExecuteAction::CreateStatic(&FBlueLineMultiModeFormatter::FormatCompact)
	);

	// Select Connected Graph (Shift+F)
	PluginCommands->MapAction(
		FBlueLineCommands::Get().SelectConnectedGraph,
		FExecuteAction::CreateStatic(&FBlueLineMultiModeFormatter::SelectConnectedGraph)
	);

	// Shift+W Toggle (Changed to avoid conflict with Engine's "Possess or Eject Player")
	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineGraph: Mapping Shift+W to ToggleWireStyle"));
	PluginCommands->MapAction(
		FBlueLineCommands::Get().ToggleWireStyle,
		FExecuteAction::CreateLambda([]() {
			UBlueLineEditorSettings* S = GetMutableDefault<UBlueLineEditorSettings>();
			// Cycle through routing methods: Curved -> Manhattan -> Circuit -> Hybrid -> Curved
			int32 CurrentMethod = static_cast<int32>(S->RoutingMethod);
			int32 NextMethod = (CurrentMethod + 1) % 4;
			S->RoutingMethod = static_cast<EBlueLineRoutingMethod>(NextMethod);
			S->PostEditChange();
			S->SaveConfig();
			
			// FIX: Refresh all visible graph editors to apply the wire style change immediately
			FSlateApplication& SlateApp = FSlateApplication::Get();
			TArray<TSharedRef<SWindow>> AllWindows;
			SlateApp.GetAllVisibleWindowsOrdered(AllWindows);
			for (const TSharedRef<SWindow>& Window : AllWindows)
			{
				// Recursively search for SGraphEditor widgets
				TArray<TSharedRef<SWidget>> WidgetStack;
				WidgetStack.Add(Window);
				while (WidgetStack.Num() > 0)
				{
					TSharedRef<SWidget> Widget = WidgetStack.Pop();
					if (Widget->GetType() == TEXT("SGraphEditor"))
					{
						// Invalidate the graph editor to trigger a redraw with the new wire style
						Widget->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
					}
					// Add children to stack
					FChildren* Children = Widget->GetChildren();
					for (int32 i = 0; i < Children->Num(); ++i)
					{
						WidgetStack.Add(Children->GetChildAt(i));
					}
				}
			}
			
			// Log the new routing method name for user feedback
			static const TCHAR* RoutingNames[] = { TEXT("Curved"), TEXT("Manhattan"), TEXT("Circuit"), TEXT("Hybrid") };
			UE_LOG(LogBlueLineCore, Log, TEXT("ToggleWireStyle executed: routing=%d (%s)"), NextMethod, RoutingNames[NextMethod]);
			})
	);

	// Bind to MainFrame (global commands)
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		MainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());
		UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineGraph: Commands bound to MainFrame"));
	}
	else
	{
		UE_LOG(LogBlueLineCore, Warning, TEXT("BlueLineGraph: MainFrame not loaded!"));
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
