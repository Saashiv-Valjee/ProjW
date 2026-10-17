#include "Debug/DebugFighterActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Simulation/FightingSimulation.h"
#include "UObject/ConstructorHelpers.h"

ADebugFighterActor::ADebugFighterActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Feet"));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Nose = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    for (UStaticMeshComponent* Mesh : {Body.Get(), Nose.Get()})
    {
        Mesh->SetupAttachment(RootComponent);
        Mesh->SetStaticMesh(Cube.Object);
        Mesh->SetMaterial(0, Material.Object);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetSimulatePhysics(false);
    }
    Body->SetRelativeLocation(FVector(0, 0, 50));
    Body->SetRelativeScale3D(FVector(0.4, 0.45, 1.0));
    Nose->SetRelativeLocation(FVector(32, 0, 75));
    Nose->SetRelativeScale3D(FVector(0.3, 0.3, 0.15));
    SetActorEnableCollision(false);
}

void ADebugFighterActor::SetColor(const FLinearColor& Color)
{
    if (UMaterialInstanceDynamic* Material = Body->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), Color);
    }
    if (UMaterialInstanceDynamic* Material = Nose->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
    }
}

void ADebugFighterActor::ApplyState(const FFighterState& State)
{
    // Presentation-only crouch. Feet and authoritative dimensions do not change.
    const bool Crouched = State.CurrentState == EFighterState::Crouching;
    const bool StartingJump = State.CurrentState == EFighterState::JumpStartup;
    const double HeightScale = Crouched ? 0.55 : StartingJump ? 0.82 : 1.0;
    Body->SetRelativeScale3D(FVector(StartingJump ? 0.46 : 0.4, 0.45, HeightScale));
    Body->SetRelativeLocation(FVector(0, 0, 50 * HeightScale));
    Nose->SetRelativeLocation(FVector(32, 0, StartingJump ? 62 : Crouched ? 40 : 75));
    const double Units = FFightingSimulation::SubunitsPerWorldUnit;
    SetActorLocationAndRotation(FVector(State.PositionX / Units, 0, State.PositionY / Units),
        FRotator(0, State.Facing == EFighterFacing::Right ? 0 : 180, 0), false, nullptr,
        ETeleportType::TeleportPhysics);
}
