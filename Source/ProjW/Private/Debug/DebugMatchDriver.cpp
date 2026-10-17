#include "Debug/DebugMatchDriver.h"
#include "Debug/DebugFighterActor.h"
#include "Debug/DebugFightingPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ADebugMatchDriver::ADebugMatchDriver()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork; // After player input processing.
}

void ADebugMatchDriver::BeginPlay()
{
    Super::BeginPlay();
    MatchState.Fighter1.PositionX = -160 * FFightingSimulation::SubunitsPerWorldUnit;
    MatchState.Fighter2.PositionX = 160 * FFightingSimulation::SubunitsPerWorldUnit;
    MatchState.Fighter2.Facing = EFighterFacing::Left;
    Fighter1Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    Fighter2Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    if (Fighter1Visual && Fighter2Visual)
    {
        Fighter1Visual->SetColor(FLinearColor(0.05f, 0.3f, 1.0f));
        Fighter2Visual->SetColor(FLinearColor(1.0f, 0.15f, 0.05f));
        Fighter1Visual->ApplyState(MatchState.Fighter1);
        Fighter2Visual->ApplyState(MatchState.Fighter2);
    }

    // Stage is visual only; top is Z=0. Simulation constants own gameplay bounds.
    AStaticMeshActor* Floor = GetWorld()->SpawnActor<AStaticMeshActor>();
    if (Floor)
    {
        Floor->SetMobility(EComponentMobility::Movable);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorLocation(FVector(0, 0, -10));
        Floor->SetActorScale3D(FVector(1000, 4, 0.2));
        Floor->SetActorEnableCollision(false);
    }
    ADirectionalLight* Light = GetWorld()->SpawnActor<ADirectionalLight>();
    if (Light)
    {
        Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Light->SetActorRotation(FRotator(-45, -60, 0));
        Light->GetLightComponent()->SetIntensity(5.0f);
    }
    Camera = GetWorld()->SpawnActor<ACameraActor>();
    if (Camera)
    {
        // Looking along -Y makes world +X screen-right (D moves right).
        Camera->SetActorLocation(FVector(0, 1300, 160));
        Camera->SetActorRotation(FRotator(0, -90, 0));
        UCameraComponent* Lens = Camera->GetCameraComponent();
        Lens->ProjectionMode = ECameraProjectionMode::Orthographic;
        Lens->OrthoWidth = 1200;
        Lens->AspectRatio = 16.0f / 9.0f;
        Lens->bConstrainAspectRatio = true;
        Lens->PostProcessSettings.bOverride_AutoExposureMethod = true;
        Lens->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
        Lens->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
        Lens->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
        Lens->PostProcessSettings.bOverride_AutoExposureBias = true;
        Lens->PostProcessSettings.AutoExposureBias = 0;
        // Select before the first camera-manager update/render. Tick also handles
        // a local controller becoming available later.
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        {
            Controller->SetViewTarget(Camera);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("Debug match ready: 60 Hz; P1 WASD / JKL UIO; X->X, Y->Z / 1024; no collision authority."));
}

void ADebugMatchDriver::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RenderFPS = DeltaSeconds > 0 ? 1.0f / DeltaSeconds : 0;
    ADebugFightingPlayerController* Controller = Cast<ADebugFightingPlayerController>(
        UGameplayStatics::GetPlayerController(this, 0));
    if (Controller && Camera && Controller->GetViewTarget() != Camera)
    {
        Controller->SetViewTarget(Camera);
    }
    // DeltaSeconds schedules steps ONLY. Authoritative movement never uses it.
    const int32 Steps = Clock.Consume(DeltaSeconds);
    for (int32 Step = 0; Step < Steps; ++Step)
    {
        LastInput = Controller ? Controller->BuildFrameInput() : FFighterInput{};
        FFightingSimulation::SimulateFrame(MatchState, LastInput, FFighterInput{});
    }
    if (Fighter1Visual) { Fighter1Visual->ApplyState(MatchState.Fighter1); }
    if (Fighter2Visual) { Fighter2Visual->ApplyState(MatchState.Fighter2); }
}
