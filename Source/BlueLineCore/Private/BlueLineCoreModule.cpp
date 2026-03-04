// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#include "BlueLineCoreModule.h"
#include "BlueLineLog.h"
#include "Styles/FBlueLineStyle.h"

void FBlueLineCoreModule::StartupModule()
{
	// Initialize shared style system (used by all BlueLine modules)
	FBlueLineStyle::Initialize();
	
	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineCore module has started."));
}

void FBlueLineCoreModule::ShutdownModule()
{
	UE_LOG(LogBlueLineCore, Log, TEXT("BlueLineCore module is shutting down."));
	
	// Shutdown shared style system
	FBlueLineStyle::Shutdown();
}

IMPLEMENT_MODULE(FBlueLineCoreModule, BlueLineCore)
