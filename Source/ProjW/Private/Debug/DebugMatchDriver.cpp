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
#include "DrawDebugHelpers.h"

namespace
{
    double PairCombatCameraYaw(const FMatchState& Match)
    {
        const double DX = static_cast<double>(Match.Fighter2.PositionX) - Match.Fighter1.PositionX;
        const double DZ = static_cast<double>(Match.Fighter2.PositionZ) - Match.Fighter1.PositionZ;
        const double AxisYaw = DX == 0.0 && DZ == 0.0
            ? (Match.Fighter1.Facing == EFighterFacing::Right ? 0.0 : 180.0)
            : FMath::RadiansToDegrees(FMath::Atan2(DZ, DX));
        return -90.0 + AxisYaw;
    }

    bool IsAnyFighterPivoting(const FMatchState& Match)
    {
        return Match.Fighter1.CurrentState == EFighterState::Pivoting
            || Match.Fighter2.CurrentState == EFighterState::Pivoting;
    }
}

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
    DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState);
    Fighter1Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    Fighter2Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    if (Fighter1Visual && Fighter2Visual)
    {
        Fighter1Visual->SetColor(FLinearColor(0.05f, 0.3f, 1.0f));
        Fighter2Visual->SetColor(FLinearColor(1.0f, 0.15f, 0.05f));
        Fighter1Visual->ApplyState(MatchState.Fighter1, MatchState.Fighter2);
        Fighter2Visual->ApplyState(MatchState.Fighter2, MatchState.Fighter1);
    }

    // Visual disc only; its top is Unreal Z=0. The integer simulation owns the circle.
    AStaticMeshActor* Floor = GetWorld()->SpawnActor<AStaticMeshActor>();
    if (Floor)
    {
        Floor->SetMobility(EComponentMobility::Movable);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
        Floor->SetActorLocation(FVector(0, 0, -10));
        const float DiscDiameterScale = 2.0f * FFightingSimulation::ArenaRadius
            / (100.0f * FFightingSimulation::SubunitsPerWorldUnit);
        Floor->SetActorScale3D(FVector(DiscDiameterScale, DiscDiameterScale, 0.2f));
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
    UE_LOG(LogTemp, Display, TEXT("Debug match ready: 60 Hz; P1 WASD, Q/E Pivot, Numpad789 UIO; deterministic 3D combat boxes."));
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
    if (Steps == 0 && !bCombatYawHeldForPivot && !bRecenteringCombatYaw)
    {
        DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState);
    }
    for (int32 Step = 0; Step < Steps; ++Step)
    {
        if (!bCombatYawHeldForPivot && !bRecenteringCombatYaw)
        {
            // Keep the ordinary presentation plane aligned before a Pivot can
            // begin, then freeze this exact displayed orientation on entry.
            DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState);
        }
        const bool bWasPivoting = IsAnyFighterPivoting(MatchState);
        LastInput = Controller ? Controller->BuildFrameInput() : FFighterInput{};
        FFightingSimulation::SimulateFrame(MatchState, LastInput, FFighterInput{});
        const bool bIsPivoting = IsAnyFighterPivoting(MatchState);
        if (!bWasPivoting && bIsPivoting)
        {
            // A new Pivot interrupts any unfinished recenter and holds the
            // yaw the player currently sees, even when P2 initiated it.
            bRecenteringCombatYaw = false;
            bCombatYawHeldForPivot = true;
        }
        else if (bWasPivoting && !bIsPivoting)
        {
            bCombatYawHeldForPivot = false;
            RecenterStartYawDegrees = DisplayedCombatYawDegrees;
            RecenterTargetYawDegrees = PairCombatCameraYaw(MatchState);
            RecenterElapsedSeconds = 0.0f;
            bRecenteringCombatYaw = true;
        }
    }
    if (bRecenteringCombatYaw && !bCombatYawHeldForPivot)
    {
        constexpr float RecenterDurationSeconds = 10.0f / 60.0f;
        RecenterElapsedSeconds = FMath::Min(RecenterDurationSeconds,
            RecenterElapsedSeconds + FMath::Max(0.0f, DeltaSeconds));
        const double Alpha = RecenterElapsedSeconds / RecenterDurationSeconds;
        const double SmoothAlpha = Alpha * Alpha * (3.0 - 2.0 * Alpha);
        const double YawDelta = FMath::FindDeltaAngleDegrees(
            RecenterStartYawDegrees, RecenterTargetYawDegrees);
        DisplayedCombatYawDegrees = RecenterStartYawDegrees + YawDelta * SmoothAlpha;
        if (Alpha >= 1.0)
        {
            DisplayedCombatYawDegrees = RecenterTargetYawDegrees;
            bRecenteringCombatYaw = false;
        }
    }
    if (Fighter1Visual) { Fighter1Visual->ApplyState(MatchState.Fighter1, MatchState.Fighter2); }
    if (Fighter2Visual) { Fighter2Visual->ApplyState(MatchState.Fighter2, MatchState.Fighter1); }
    if (Camera)
    {
        const double U = FFightingSimulation::SubunitsPerWorldUnit;
        const FVector Center((MatchState.Fighter1.PositionX + static_cast<double>(MatchState.Fighter2.PositionX)) / (2 * U),
            (MatchState.Fighter1.PositionZ + static_cast<double>(MatchState.Fighter2.PositionZ)) / (2 * U), 160);
        const FRotator TargetRotation(0, DisplayedCombatYawDegrees, 0);
        const FVector TargetLocation = Center - TargetRotation.Vector() * 1300.0;
        Camera->SetActorRotation(TargetRotation);
        Camera->SetActorLocation(FMath::VInterpTo(Camera->GetActorLocation(), TargetLocation, DeltaSeconds, 2.0f));
    }
    DrawDebugLine(GetWorld(), Fighter1Visual ? Fighter1Visual->GetActorLocation() : FVector::ZeroVector,
        Fighter2Visual ? Fighter2Visual->GetActorLocation() : FVector::ZeroVector,
        FColor::Cyan, false, 0.05f, 0, 1.0f);
    if (Fighter1Visual && Fighter2Visual)
    {
        const double PresentedAxisYaw = FMath::DegreesToRadians(DisplayedCombatYawDegrees + 90.0);
        const FVector PresentedAxis(FMath::Cos(PresentedAxisYaw), FMath::Sin(PresentedAxisYaw), 0.0);
        const FVector PairCenter(
            (Fighter1Visual->GetActorLocation().X + Fighter2Visual->GetActorLocation().X) * 0.5,
            (Fighter1Visual->GetActorLocation().Y + Fighter2Visual->GetActorLocation().Y) * 0.5, 2.0);
        DrawDebugLine(GetWorld(), PairCenter - PresentedAxis * 100.0, PairCenter + PresentedAxis * 100.0,
            FColor::Yellow, false, 0.05f, 0, 1.5f);
    }
    for (const FFighterState* Fighter : {&MatchState.Fighter1, &MatchState.Fighter2})
    {
        const FSimBox Hurt = FFightingSimulation::GetHurtbox(*Fighter);
        const FVector HurtCenter((Hurt.MinX + Hurt.MaxX) * 0.5 / 1024.0,
            (Hurt.MinZ + Hurt.MaxZ) * 0.5 / 1024.0,
            (Hurt.MinY + Hurt.MaxY) * 0.5 / 1024.0);
        DrawDebugBox(GetWorld(), HurtCenter, FVector((Hurt.MaxX-Hurt.MinX)*0.5/1024.0,
            (Hurt.MaxZ-Hurt.MinZ)*0.5/1024.0, (Hurt.MaxY-Hurt.MinY)*0.5/1024.0),
            FColor::Green, false, 0.05f, 0, 1.5f);
        FSimOrientedBox Attack;
        if (FFightingSimulation::GetMoveHitVolume(*Fighter, Attack))
        {
            const FVector Center(Attack.CenterX / 1024.0, Attack.CenterZ / 1024.0,
                (Attack.MinY + Attack.MaxY) * 0.5 / 1024.0);
            const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(
                static_cast<double>(Attack.AxisZ), static_cast<double>(Attack.AxisX)));
            DrawDebugBox(GetWorld(), Center, FVector(Attack.HalfForward / 1024.0,
                Attack.HalfLateral / 1024.0, (Attack.MaxY-Attack.MinY)*0.5/1024.0),
                FRotator(0, Yaw, 0).Quaternion(), FColor::Red, false, 0.05f, 0, 2.0f);
        }
    }
}
