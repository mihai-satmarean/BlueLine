// Copyright (c) 2026 GregOrigin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FBlueLineSmartTagCommands : public TCommands<FBlueLineSmartTagCommands>
{
public:
    FBlueLineSmartTagCommands()
        : TCommands<FBlueLineSmartTagCommands>(
            TEXT("BlueLineSmartTags"),
            NSLOCTEXT("Contexts", "BlueLineSmartTags", "BlueLine Smart Tags"),
            NAME_None,
            FAppStyle::GetAppStyleSetName()
        )
    {}

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> AutoTagGraph;
};
