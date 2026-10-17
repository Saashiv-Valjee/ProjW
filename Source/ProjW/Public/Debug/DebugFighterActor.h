#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/FightingSimulationTypes.h"
#include "DebugFighterActor.generated.h"

class UStaticMeshComponent;

// Presentation only. Feet are at the Actor origin; local +X is forward.
UCLASS()
class PROJW_API ADebugFighterActor : public AActor
{
    GENERATED_BODY()
public:
    ADebugFighterActor();
    void SetColor(const FLinearColor& Color);
    void ApplyState(const FFighterState& State);
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Nose;
};
