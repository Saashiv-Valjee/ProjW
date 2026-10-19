#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/FightingSimulationTypes.h"
#include "DebugFighterActor.generated.h"

class USkeletalMeshComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UAnimationAsset;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Presentation-only Combat Lab fighter. The Actor transform follows the integer
// simulation; its mesh and cues never feed back into gameplay.
UCLASS()
class PROJW_API ADebugFighterActor : public AActor
{
    GENERATED_BODY()
public:
    ADebugFighterActor();
    void SetCharacter(bool bUseQuinn);
    void SetColor(const FLinearColor& Color);
    void ApplyState(const FFighterState& State, const FFighterState& Opponent,
        float PresentationDeltaSeconds);

private:
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> FighterMesh;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> CrouchPoseMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> TeamMarker;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ActionCue;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GuardCue;
    UPROPERTY() TObjectPtr<UAnimationAsset> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> WalkForwardAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> WalkBackwardAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> JogForwardAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> JogBackwardAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> JumpAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> DoubleJumpAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> LightAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> MediumAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> HeavyAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> GroundRuptureAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> HitReactAnimation;
    UPROPERTY() TObjectPtr<UAnimationAsset> ChargedHitReactAnimation;
    UPROPERTY() TObjectPtr<UStaticMesh> CueMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> RingMesh;
    UPROPERTY() TObjectPtr<UMaterialInterface> FlatMaterial;

    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> TeamMaterial;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> CueMaterial;
    FLinearColor FighterColor = FLinearColor::White;
    double PresentationYawDegrees = 0.0;
    bool bHasPresentationYaw = false;
    UAnimationAsset* CurrentAnimation = nullptr;
    uint8 CurrentVisualAction = 255;
    bool bPresentationBlockstunLow = false;
    float CrouchPoseBlend = 0.0f;
};
