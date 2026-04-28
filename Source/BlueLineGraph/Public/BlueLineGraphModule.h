// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FUICommandList;
struct FGraphPanelNodeFactory;
class FBlueLineGraphPinFactory;

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

	/** Unregisters Slate-dependent components (WireSnapper, ConnectionInterceptor).
	 *  Called both from OnPreShutdown (while Slate is still valid) and ShutdownModule
	 *  as a safety net. Idempotent — safe to call multiple times. */
	void DisableSlateComponents();

	TSharedPtr<FGraphPanelNodeFactory> BlueLineGraphPanelFactory;
	TSharedPtr<FBlueLineGraphPinFactory> BlueLinePinFactory;
	TSharedPtr<FUICommandList> PluginCommands;

	/** Handle for FSlateApplication::OnPreShutdown binding. */
	FDelegateHandle SlatePreShutdownHandle;
};
