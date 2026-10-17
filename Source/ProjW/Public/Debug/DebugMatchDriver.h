#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/FixedSimulationClock.h"
#include "DebugMatchDriver.generated.h"

class ADebugFighterActor;
class ACameraActor;

UCLASS()
class PROJW_API ADebugMatchDriver : public AActor
{
    GENERATED_BODY()
public:
    ADebugMatchDriver();
    virtual void Tick(float DeltaSeconds) override;
    const FMatchState& GetMatchState() const { return MatchState; }
    FFighterInput GetLastInput() const { return LastInput; }
    uint64 GetHitchCount() const { return Clock.HitchCount; }
    float GetRenderFPS() const { return RenderFPS; }
protected:
    virtual void BeginPlay() override;
private:
    FMatchState MatchState;
    FFighterInput LastInput;
    FFixedSimulationClock Clock;
    float RenderFPS = 0;
    UPROPERTY() TObjectPtr<ADebugFighterActor> Fighter1Visual;
    UPROPERTY() TObjectPtr<ADebugFighterActor> Fighter2Visual;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
};
