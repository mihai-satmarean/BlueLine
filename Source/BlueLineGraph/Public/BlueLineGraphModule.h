// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "EdGraph/EdGraph.h"

class FUICommandList;
struct FGraphPanelNodeFactory;
class FBlueLineGraphPinFactory;
class IConsoleObject;

class FBlueLineGraphModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void InstallGraphDrawingPolicy();
	void UninstallGraphDrawingPolicy();
	void InstallGraphPinFactory();
	void UninstallGraphPinFactory();
	void RegisterCommands();
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();

	/** Unregisters Slate-dependent components (WireSnapper, ConnectionInterceptor).
	 *  Called both from OnPreShutdown (while Slate is still valid) and ShutdownModule
	 *  as a safety net. Idempotent — safe to call multiple times. */
	void DisableSlateComponents();

	// --- Auto-format on new node ---
	void RegisterGraphChangeHooks();
	void UnregisterGraphChangeHooks();
	void OnAssetOpenedInEditor(UObject* Asset, class IAssetEditorInstance* Editor);
	void SubscribeToGraph(UEdGraph* Graph);
	void OnGraphChanged(const FEdGraphEditAction& Action);

	TSharedPtr<FGraphPanelNodeFactory> BlueLineGraphPanelFactory;
	TSharedPtr<FBlueLineGraphPinFactory> BlueLinePinFactory;
	TSharedPtr<FUICommandList> PluginCommands;

	/** Handle for FSlateApplication::OnPreShutdown binding. */
	FDelegateHandle SlatePreShutdownHandle;

	/** Handle for UAssetEditorSubsystem::OnAssetOpenedInEditor binding. */
	FDelegateHandle AssetOpenedHandle;

	/** Per-graph delegate handles so we can unsubscribe cleanly. */
	TMap<TWeakObjectPtr<UEdGraph>, FDelegateHandle> GraphChangeHandles;

	/** Console command objects — held to keep them alive. */
	TArray<IConsoleObject*> ConsoleCommands;
};
