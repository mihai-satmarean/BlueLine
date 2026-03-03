// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/UBlueLineThemeData.h"
#include "BlueLineDemoActor.generated.h"

/**
 * A simple actor to demonstrate FBlueLineTagStyle usage in variables.
 */
UCLASS(Blueprintable)
class BLUELINECORE_API ABlueLineDemoActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ABlueLineDemoActor();

	/** 
	 * This variable will show the BlueLine Smart UI in the Details panel.
	 * Click the âœ¨ icon to see the Smart Tag Analyzer in action!
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo")
	FBlueLineTagStyle DemoTag;

	// --- COMBAT CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Combat")
	FBlueLineTagStyle HealthStatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Combat")
	FBlueLineTagStyle DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Combat")
	FBlueLineTagStyle WeaponAffinityTag;

	// --- MOVEMENT CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Movement")
	FBlueLineTagStyle VelocityModeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Movement")
	FBlueLineTagStyle RotationLogicTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Movement")
	FBlueLineTagStyle PhysicsSurfaceTag;

	// --- AI CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|AI")
	FBlueLineTagStyle BehaviorStateTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|AI")
	FBlueLineTagStyle BlackboardKeyTag;

	// --- NETWORKING CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Networking")
	FBlueLineTagStyle ServerReplicationTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Networking")
	FBlueLineTagStyle MulticastEventTag;

	// --- VISUALS & AUDIO CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Presentation")
	FBlueLineTagStyle NiagaraEffectTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Presentation")
	FBlueLineTagStyle AudioAmbientTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Presentation")
	FBlueLineTagStyle UIVisibilityTag;

	// --- DATA & LOGIC CATEGORY ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Internal")
	FBlueLineTagStyle ConfigDataTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlueLine Demo|Internal")
	FBlueLineTagStyle PriorityLogicTag;

protected:
	virtual void BeginPlay() override;
};
