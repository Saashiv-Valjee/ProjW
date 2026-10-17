#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DebugFightingGameMode.generated.h"

UCLASS()
class PROJW_API ADebugFightingGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ADebugFightingGameMode();
    virtual void StartPlay() override;
};
