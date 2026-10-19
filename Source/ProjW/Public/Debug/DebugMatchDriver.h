#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Debug/FixedSimulationClock.h"
#include "DebugMatchDriver.generated.h"

class ADebugFighterActor;
class ACameraActor;
class AStaticMeshActor;

UCLASS()
class PROJW_API ADebugMatchDriver : public AActor
{
    GENERATED_BODY()
public:
    ADebugMatchDriver();
    virtual void Tick(float DeltaSeconds) override;
    const FMatchState& GetMatchState() const { return MatchState; }
    FFighterInput GetLastInput() const { return LastInput; }
    const FString& GetInputPathDiagnostic() const { return InputPathDiagnostic; }
    const FString& GetContactDiagnostic() const { return ContactDiagnostic; }
    uint64 GetHitchCount() const { return Clock.HitchCount; }
    float GetRenderFPS() const { return RenderFPS; }
    bool IsDebugGeometryVisible() const { return bDebugGeometryVisible; }
    bool IsDetailedHudVisible() const { return bDetailedHudVisible; }
    bool IsPresentationDiagnosticsVisible() const { return bPresentationDiagnosticsVisible; }
    FString GetPresentationDiagnosticText() const;
protected:
    virtual void BeginPlay() override;
private:
    FMatchState MatchState;
    FFighterInput LastInput;
    FFighterInput LastOpponentInput;
    FString InputPathDiagnostic;
    FString ContactDiagnostic;
    FString LastLoggedPhysicalInput;
    FFixedSimulationClock Clock;
    float RenderFPS = 0;
    // Debug-camera state is presentation-only and never enters match snapshots.
    double DisplayedCombatYawDegrees = -90.0;
    double RecenterStartYawDegrees = -90.0;
    double RecenterTargetYawDegrees = -90.0;
    uint64 LastDiagnosticLogFrame = 0;
    uint64 LastBoundaryTraceFrame = 0;
    float RecenterElapsedSeconds = 0.0f;
    bool bCombatYawHeldForPivot = false;
    bool bRecenteringCombatYaw = false;
    bool bDebugGeometryVisible = false;
    bool bDetailedHudVisible = false;
    bool bPresentationDiagnosticsVisible = false;
    bool bF2WasDown = false;
    bool bF3WasDown = false;
    bool bF4WasDown = false;
    bool bReportedInvalidPresentation = false;
    UPROPERTY() TObjectPtr<ADebugFighterActor> Fighter1Visual;
    UPROPERTY() TObjectPtr<ADebugFighterActor> Fighter2Visual;
    UPROPERTY() TObjectPtr<ACameraActor> Camera;
    UPROPERTY() TObjectPtr<AStaticMeshActor> BackdropVisual;
    double GetScreenRightAlignment(const FFighterState& Fighter, const FFighterState& Opponent) const;
};
