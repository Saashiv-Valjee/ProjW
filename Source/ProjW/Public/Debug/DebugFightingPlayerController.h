#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/FightingSimulationTypes.h"
#include "DebugFightingPlayerController.generated.h"

UCLASS()
class PROJW_API ADebugFightingPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ADebugFightingPlayerController();
    FFighterInput BuildFrameInput() const;
protected:
    virtual void BeginPlay() override;
};
