// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphPin;

/**
 * FBlueLineConnectionInterceptor
 * 
 * Automatically applies Manhattan routing to new connections as they're created.
 * This provides real-time Manhattan routing without manual intervention.
 * 
 * Can be toggled on/off via settings.
 */
class FBlueLineConnectionInterceptor
{
public:
    /**
     * Enable automatic Manhattan routing for new connections.
     * Call this in module StartupModule() if auto-routing is enabled in settings.
     */
    static void Enable();

    /**
     * Disable automatic routing.
     * Call this in module ShutdownModule().
     */
    static void Disable();

    /**
     * Check if auto-routing is currently active.
     */
    static bool IsEnabled() { return bIsEnabled; }

private:
    // Graph notification callbacks
    static void OnObjectModified(UObject* Object);
    
    // Handle new pin connections
    static void OnPinConnectionCreated(UEdGraphPin* PinA, UEdGraphPin* PinB);

    // State
    static bool bIsEnabled;
    static TArray<FDelegateHandle> GraphChangedDelegateHandles;
};
