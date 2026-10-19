#include "Debug/DebugMatchDriver.h"
#include "Debug/DebugFighterActor.h"
#include "Debug/DebugFightingPlayerController.h"
#include "Debug/CameraSideSelection.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "InputCoreTypes.h"

namespace
{
    constexpr double DebugCameraPitchDegrees = -8.0;

    double RawPairCombatCameraYaw(const FMatchState& Match)
    {
        const double DX = static_cast<double>(Match.Fighter2.PositionX) - Match.Fighter1.PositionX;
        const double DZ = static_cast<double>(Match.Fighter2.PositionZ) - Match.Fighter1.PositionZ;
        const double AxisYaw = DX == 0.0 && DZ == 0.0
            ? (Match.Fighter1.Facing == EFighterFacing::Right ? 0.0 : 180.0)
            : FMath::RadiansToDegrees(FMath::Atan2(DZ, DX));
        return -90.0 + AxisYaw;
    }

    double PairCombatCameraYaw(const FMatchState& Match, double PresentedYawDegrees)
    {
        return ProjW::Presentation::SelectContinuousSideOnYaw(
            RawPairCombatCameraYaw(Match), PresentedYawDegrees);
    }

    bool IsAnyFighterPivoting(const FMatchState& Match)
    {
        return Match.Fighter1.CurrentState == EFighterState::Pivoting
            || Match.Fighter2.CurrentState == EFighterState::Pivoting;
    }

    AStaticMeshActor* SpawnStagePiece(UWorld* World, UStaticMesh* Mesh, const FVector& Location,
        const FVector& Scale, const FLinearColor& Color)
    {
        if (!World || !Mesh) return nullptr;
        AStaticMeshActor* Piece = World->SpawnActor<AStaticMeshActor>();
        if (!Piece) return nullptr;
        Piece->SetMobility(EComponentMobility::Movable);
        UStaticMeshComponent* Component = Piece->GetStaticMeshComponent();
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Piece->SetActorLocation(Location);
        Piece->SetActorScale3D(Scale);
        Piece->SetActorEnableCollision(false);
        if (UMaterialInstanceDynamic* Tint = Component->CreateAndSetMaterialInstanceDynamic(0))
        {
            Tint->SetVectorParameterValue(TEXT("Color"), Color);
        }
        return Piece;
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
    DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState, DisplayedCombatYawDegrees);
    Fighter1Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    Fighter2Visual = GetWorld()->SpawnActor<ADebugFighterActor>();
    if (Fighter1Visual && Fighter2Visual)
    {
        Fighter1Visual->SetColor(FLinearColor(0.05f, 0.3f, 1.0f));
        Fighter2Visual->SetColor(FLinearColor(1.0f, 0.15f, 0.05f));
        Fighter1Visual->SetCharacter(false);
        Fighter2Visual->SetCharacter(true);
        Fighter1Visual->ApplyState(MatchState.Fighter1, MatchState.Fighter2, 0.0f);
        Fighter2Visual->ApplyState(MatchState.Fighter2, MatchState.Fighter1, 0.0f);
    }

    // A presentation-only training room. The gameplay circle is still owned
    // solely by FFightingSimulation; every environment mesh has collision off.
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    SpawnStagePiece(GetWorld(), Cylinder, FVector(0, 0, -35), FVector(30.0, 30.0, 0.3),
        FLinearColor(0.025f, 0.035f, 0.055f));
    SpawnStagePiece(GetWorld(), Cylinder, FVector(0, 0, -10), FVector(9.6, 9.6, 0.08),
        FLinearColor(0.12f, 0.32f, 0.38f)); // Outer silhouette is exactly the 480-unit arena radius.
    SpawnStagePiece(GetWorld(), Cylinder, FVector(0, 0, -4), FVector(9.3, 9.3, 0.08),
        FLinearColor(0.055f, 0.075f, 0.095f)); // 465-unit inset leaves the center-limit cue legible.
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
        Camera->SetActorRotation(FRotator(DebugCameraPitchDegrees, -90, 0));
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
    // A camera-facing muted color card sits well beyond the entire arena. It is
    // collisionless and follows the view at every yaw, filling the black void
    // without adding a wall or any gameplay obstruction.
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    BackdropVisual = SpawnStagePiece(GetWorld(), Cube, FVector::ZeroVector,
        FVector(0.05, 40.0, 40.0), FLinearColor(0.16f, 0.20f, 0.24f));
    if (BackdropVisual && BackdropVisual->GetStaticMeshComponent())
    {
        UStaticMeshComponent* BackdropMesh = BackdropVisual->GetStaticMeshComponent();
        BackdropMesh->SetCastShadow(false);
        BackdropMesh->SetReceivesDecals(false);
        if (UMaterialInterface* FlatBackdrop = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/ArtTools/RenderToTexture/Materials/Debug/M_Emissive_Color.M_Emissive_Color")))
        {
            UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(FlatBackdrop, BackdropVisual);
            if (Tint)
            {
                Tint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.045f, 0.08f, 0.12f));
                Tint->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.045f, 0.08f, 0.12f));
                BackdropMesh->SetMaterial(0, Tint);
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("Debug match ready: 60 Hz; F2 diagnostics, F3 combat geometry, F4 camera/movement diagnostics."));
}

double ADebugMatchDriver::GetScreenRightAlignment(const FFighterState& Fighter,
    const FFighterState& Opponent) const
{
    double DX = static_cast<double>(Opponent.PositionX) - Fighter.PositionX;
    double DZ = static_cast<double>(Opponent.PositionZ) - Fighter.PositionZ;
    const double Length = FMath::Sqrt(DX * DX + DZ * DZ);
    double AxisX = Length > 0.0 ? DX / Length : static_cast<double>(Fighter.Facing);
    double AxisZ = Length > 0.0 ? DZ / Length : 0.0;
    if (AxisX < 0.0) { AxisX = -AxisX; AxisZ = -AxisZ; }
    const double ScreenYaw = FMath::DegreesToRadians(DisplayedCombatYawDegrees + 90.0);
    return AxisX * FMath::Cos(ScreenYaw) + AxisZ * FMath::Sin(ScreenYaw);
}

FString ADebugMatchDriver::GetPresentationDiagnosticText() const
{
    const double U = FFightingSimulation::SubunitsPerWorldUnit;
    const FVector P1(MatchState.Fighter1.PositionX / U, MatchState.Fighter1.PositionZ / U,
        MatchState.Fighter1.PositionY / U);
    const FVector P2(MatchState.Fighter2.PositionX / U, MatchState.Fighter2.PositionZ / U,
        MatchState.Fighter2.PositionY / U);
    const FVector Center = (P1 + P2) * 0.5;
    const FVector CameraTarget(Center.X, Center.Y, 160.0);
    const double DX = P2.X - P1.X, DY = P2.Y - P1.Y;
    const double AxisLength = FMath::Sqrt(DX * DX + DY * DY);
    const FVector CameraLocation = Camera ? Camera->GetActorLocation() : FVector::ZeroVector;
    const FRotator CameraRotation = Camera ? Camera->GetActorRotation() : FRotator::ZeroRotator;
    const double CameraDistance = FVector::Dist(CameraLocation, CameraTarget);
    const UCameraComponent* Lens = Camera ? Camera->GetCameraComponent() : nullptr;
    const double OrthoWidth = Lens ? Lens->OrthoWidth : 0.0;
    const double FOV = Lens ? Lens->FieldOfView : 0.0;
    const double DesiredYaw = PairCombatCameraYaw(MatchState, DisplayedCombatYawDegrees);
    const FVector CameraUp = Camera ? Camera->GetActorUpVector() : FVector::UpVector;
    const FVector FloorCenter(0.0, 0.0, -35.0);
    const double FloorVerticalCenter = FVector::DotProduct(FloorCenter - CameraLocation, CameraUp);
    const double FloorVerticalExtent = 480.0 * FMath::Sqrt(CameraUp.X * CameraUp.X + CameraUp.Y * CameraUp.Y)
        + 15.0 * FMath::Abs(CameraUp.Z);
    const double OrthoHalfHeight = Lens && Lens->AspectRatio > 0.0
        ? OrthoWidth / (2.0 * Lens->AspectRatio) : 0.0;
    const bool bFloorWithinVerticalView = FMath::Abs(FloorVerticalCenter) - FloorVerticalExtent < OrthoHalfHeight
        && FMath::Abs(FloorVerticalCenter) + FloorVerticalExtent > -OrthoHalfHeight;
    const double MaxFighterHeight = FMath::Max(MatchState.Fighter1.PositionY, MatchState.Fighter2.PositionY) / U;
    const auto IsFiniteVector = [](const FVector& V)
    { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z); };
    const bool bFinite = IsFiniteVector(P1) && IsFiniteVector(P2) && IsFiniteVector(Center)
        && IsFiniteVector(CameraLocation)
        && FMath::IsFinite(DisplayedCombatYawDegrees) && FMath::IsFinite(DesiredYaw)
        && FMath::IsFinite(CameraRotation.Pitch) && FMath::IsFinite(CameraRotation.Yaw)
        && FMath::IsFinite(CameraRotation.Roll)
        && FMath::IsFinite(CameraDistance) && FMath::IsFinite(OrthoWidth) && FMath::IsFinite(FOV);
    const auto WithinCircle = [](const FFighterState& F)
    {
        return static_cast<int64>(F.PositionX) * F.PositionX
            + static_cast<int64>(F.PositionZ) * F.PositionZ
            <= FFightingSimulation::FighterCenterRadiusSquared;
    };
    const FVector Actor1 = Fighter1Visual ? Fighter1Visual->GetActorLocation() : FVector::ZeroVector;
    const FVector Actor2 = Fighter2Visual ? Fighter2Visual->GetActorLocation() : FVector::ZeroVector;
    const double ActorYaw1 = Fighter1Visual ? Fighter1Visual->GetActorRotation().Yaw : 0.0;
    const double ActorYaw2 = Fighter2Visual ? Fighter2Visual->GetActorRotation().Yaw : 0.0;
    const bool bCameraInvariant = Camera
        && FMath::IsNearlyEqual(CameraDistance, 1300.0, 0.1)
        && FMath::IsNearlyEqual(CameraRotation.Pitch, DebugCameraPitchDegrees, 0.01)
        && FMath::IsNearlyZero(CameraRotation.Roll, 0.01)
        && FMath::Abs(FMath::FindDeltaAngleDegrees(CameraRotation.Yaw, DisplayedCombatYawDegrees)) < 0.01
        && FMath::IsNearlyEqual(OrthoWidth, 1200.0, 0.1)
        && FVector::Dist(P1, Actor1) < 0.01 && FVector::Dist(P2, Actor2) < 0.01;
    return FString::Printf(
        TEXT("5V presentation diagnostics | frame=%llu | finite=%s cameraInvariant=%s | arenaCenters=%s/%s\n")
        TEXT("P1=(%.2f, %.2f, %.2f) actor=(%.2f, %.2f, %.2f) yaw=%.2f | P2=(%.2f, %.2f, %.2f) actor=(%.2f, %.2f, %.2f) yaw=%.2f\n")
        TEXT("center=(%.2f, %.2f, %.2f) axisDelta=(%.2f, %.2f) length=%.2f | displayYaw=%.3f desiredYaw=%.3f held=%s recenter=%s\n")
        TEXT("camera=(%.2f, %.2f, %.2f) rot=(%.3f, %.3f, %.3f) distance=%.2f ortho=%.2f fov=%.2f | " )
        TEXT("fighterMaxY=%.2f cameraTargetZ=160 floorScreenY=%.2f extent=%.2f orthoHalfHeight=%.2f floorInView=%s backdrop=%s"),
        MatchState.SimulationFrame, bFinite ? TEXT("yes") : TEXT("NO"), bCameraInvariant ? TEXT("ok") : TEXT("FAIL"),
        WithinCircle(MatchState.Fighter1) ? TEXT("inside") : TEXT("OUTSIDE"),
        WithinCircle(MatchState.Fighter2) ? TEXT("inside") : TEXT("OUTSIDE"),
        P1.X, P1.Y, P1.Z, Fighter1Visual ? Fighter1Visual->GetActorLocation().X : 0.0,
        Fighter1Visual ? Fighter1Visual->GetActorLocation().Y : 0.0, Fighter1Visual ? Fighter1Visual->GetActorLocation().Z : 0.0, ActorYaw1,
        P2.X, P2.Y, P2.Z, Fighter2Visual ? Fighter2Visual->GetActorLocation().X : 0.0,
        Fighter2Visual ? Fighter2Visual->GetActorLocation().Y : 0.0, Fighter2Visual ? Fighter2Visual->GetActorLocation().Z : 0.0, ActorYaw2,
        Center.X, Center.Y, Center.Z, DX, DY, AxisLength, DisplayedCombatYawDegrees, DesiredYaw,
        bCombatYawHeldForPivot ? TEXT("yes") : TEXT("no"), bRecenteringCombatYaw ? TEXT("yes") : TEXT("no"),
        CameraLocation.X, CameraLocation.Y, CameraLocation.Z, CameraRotation.Pitch, CameraRotation.Yaw,
        CameraRotation.Roll, CameraDistance, OrthoWidth, FOV, MaxFighterHeight,
        FloorVerticalCenter, FloorVerticalExtent, OrthoHalfHeight,
        bFloorWithinVerticalView ? TEXT("yes") : TEXT("no"), BackdropVisual ? TEXT("on") : TEXT("missing"));
}

void ADebugMatchDriver::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    constexpr double WorldUnits = FFightingSimulation::SubunitsPerWorldUnit;
    RenderFPS = DeltaSeconds > 0 ? 1.0f / DeltaSeconds : 0;
    ADebugFightingPlayerController* Controller = Cast<ADebugFightingPlayerController>(
        UGameplayStatics::GetPlayerController(this, 0));
    const bool bF2Down = Controller && Controller->IsInputKeyDown(EKeys::F2);
    const bool bF3Down = Controller && Controller->IsInputKeyDown(EKeys::F3);
    const bool bF4Down = Controller && Controller->IsInputKeyDown(EKeys::F4);
    if (bF2Down && !bF2WasDown) bDetailedHudVisible = !bDetailedHudVisible;
    if (bF3Down && !bF3WasDown) bDebugGeometryVisible = !bDebugGeometryVisible;
    if (bF4Down && !bF4WasDown) bPresentationDiagnosticsVisible = !bPresentationDiagnosticsVisible;
    bF2WasDown = bF2Down;
    bF3WasDown = bF3Down;
    bF4WasDown = bF4Down;
    if (Controller && Camera && Controller->GetViewTarget() != Camera)
    {
        Controller->SetViewTarget(Camera);
    }
    // DeltaSeconds schedules steps ONLY. Authoritative movement never uses it.
    const int32 Steps = Clock.Consume(DeltaSeconds);
    if (Steps == 0 && !bCombatYawHeldForPivot && !bRecenteringCombatYaw)
    {
        DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState, DisplayedCombatYawDegrees);
    }
    for (int32 Step = 0; Step < Steps; ++Step)
    {
        if (!bCombatYawHeldForPivot && !bRecenteringCombatYaw)
        {
            // Keep the ordinary presentation plane aligned before a Pivot can
            // begin, then freeze this exact displayed orientation on entry.
            DisplayedCombatYawDegrees = PairCombatCameraYaw(MatchState, DisplayedCombatYawDegrees);
        }
        const bool bWasPivoting = IsAnyFighterPivoting(MatchState);
        const double P1Alignment = GetScreenRightAlignment(MatchState.Fighter1, MatchState.Fighter2);
        const double P2Alignment = GetScreenRightAlignment(MatchState.Fighter2, MatchState.Fighter1);
        LastInput = Controller ? Controller->BuildFrameInput(P1Alignment) : FFighterInput{};
        LastOpponentInput = Controller
            ? Controller->BuildOpponentFrameInput(P2Alignment) : FFighterInput{};
        if (Controller)
        {
            const FString PhysicalInput = Controller->GetInputDiagnosticText();
            if (PhysicalInput != LastLoggedPhysicalInput)
            {
                LastLoggedPhysicalInput = PhysicalInput;
                UE_LOG(LogTemp, Display, TEXT("Debug human keyboard path | %s"), *PhysicalInput);
            }
            InputPathDiagnostic = PhysicalInput;
        }
        const FMatchState BeforeStep = MatchState;
        FFightingSimulation::SimulateFrame(MatchState, LastInput, LastOpponentInput);
        const auto DescribeOutcome = [](const FFighterState& Defender, const FFighterState& BeforeDefender)
        {
            if (Defender.CurrentState == EFighterState::ParrySuccess) return TEXT("Parry");
            if (Defender.PendingReaction == EPendingReaction::Block
                && BeforeDefender.PendingReaction != EPendingReaction::Block) return TEXT("Block");
            if (Defender.PendingReaction == EPendingReaction::Hit
                && BeforeDefender.PendingReaction != EPendingReaction::Hit) return TEXT("Hit");
            return TEXT("");
        };
        const TCHAR* Outcome12 = DescribeOutcome(MatchState.Fighter2, BeforeStep.Fighter2);
        const TCHAR* Outcome21 = DescribeOutcome(MatchState.Fighter1, BeforeStep.Fighter1);
        if (Outcome12[0] || Outcome21[0])
        {
            const FMoveDefinition* Move1 = FFightingSimulation::FindMoveDefinition(BeforeStep.Fighter1.CurrentMoveId);
            const FMoveDefinition* Move2 = FFightingSimulation::FindMoveDefinition(BeforeStep.Fighter2.CurrentMoveId);
            ContactDiagnostic = FString::Printf(
                TEXT("Contact P1->P2: %s (move=%u height=%s) | P2->P1: %s (move=%u height=%s)"),
                Outcome12, static_cast<uint8>(BeforeStep.Fighter1.CurrentMoveId),
                Move1 ? (Move1->ParryHeight == EParryHeight::High ? TEXT("High") : TEXT("Low")) : TEXT("-"),
                Outcome21, static_cast<uint8>(BeforeStep.Fighter2.CurrentMoveId),
                Move2 ? (Move2->ParryHeight == EParryHeight::High ? TEXT("High") : TEXT("Low")) : TEXT("-"));
            UE_LOG(LogTemp, Display, TEXT("Debug human contact | %s"), *ContactDiagnostic);
        }
        const auto MakeControlState = [](const TCHAR* Name, const FFighterState& Fighter)
        {
            const EFighterInputButton Away = Fighter.Facing == EFighterFacing::Right
                ? EFighterInputButton::Left : EFighterInputButton::Right;
            const bool bAway = Fighter.PreviousInput.IsHeld(Away);
            const TCHAR* Guard = !Fighter.bGrounded || !bAway ? TEXT("None")
                : Fighter.PreviousInput.IsHeld(EFighterInputButton::Down) ? TEXT("Crouch") : TEXT("Stand");
            const TCHAR* ParryPhase = Fighter.CurrentState != EFighterState::Parrying ? TEXT("-")
                : Fighter.StateFrame < FFightingSimulation::ParryActiveFrames ? TEXT("Active") : TEXT("Recovery");
            return FString::Printf(TEXT("%s state=%u/%u face=%s away=%s guard=%s parry=%u press(H/L)=%d/%d"),
                Name, static_cast<uint32>(Fighter.CurrentState), Fighter.StateFrame,
                Fighter.Facing == EFighterFacing::Right ? TEXT("R") : TEXT("L"),
                bAway ? TEXT("yes") : TEXT("no"), Guard,
                static_cast<uint32>(Fighter.ParryHeight),
                Fighter.PressedInput.IsHeld(EFighterInputButton::HighParry),
                Fighter.PressedInput.IsHeld(EFighterInputButton::LowParry)) + TEXT(" ") + ParryPhase;
        };
        InputPathDiagnostic += TEXT(" | ") + MakeControlState(TEXT("P1"), MatchState.Fighter1)
            + TEXT(" | ") + MakeControlState(TEXT("P2"), MatchState.Fighter2);
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
            RecenterTargetYawDegrees = PairCombatCameraYaw(MatchState, DisplayedCombatYawDegrees);
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
    if (Fighter1Visual) Fighter1Visual->ApplyState(MatchState.Fighter1, MatchState.Fighter2, DeltaSeconds);
    if (Fighter2Visual) Fighter2Visual->ApplyState(MatchState.Fighter2, MatchState.Fighter1, DeltaSeconds);
    if (Camera)
    {
        const double U = FFightingSimulation::SubunitsPerWorldUnit;
        const FVector Center((MatchState.Fighter1.PositionX + static_cast<double>(MatchState.Fighter2.PositionX)) / (2 * U),
            (MatchState.Fighter1.PositionZ + static_cast<double>(MatchState.Fighter2.PositionZ)) / (2 * U), 160);
        const FRotator TargetRotation(DebugCameraPitchDegrees, DisplayedCombatYawDegrees, 0);
        const FVector TargetLocation = Center - TargetRotation.Vector() * 1300.0;
        // Derive both camera components from the same center/yaw sample so the
        // lens cannot lag behind the orbit plane during rapid Pivot sequences.
        Camera->SetActorRotation(TargetRotation);
        Camera->SetActorLocation(TargetLocation);
        if (BackdropVisual)
        {
            BackdropVisual->SetActorLocation(Camera->GetActorLocation() + Camera->GetActorForwardVector() * 5000.0);
            BackdropVisual->SetActorRotation(Camera->GetActorRotation());
        }
    }
    if (bPresentationDiagnosticsVisible && MatchState.SimulationFrame >= LastDiagnosticLogFrame + 60)
    {
        LastDiagnosticLogFrame = MatchState.SimulationFrame;
        const FString Diagnostic = GetPresentationDiagnosticText();
        UE_LOG(LogTemp, Display, TEXT("%s"), *Diagnostic.Replace(TEXT("\n"), TEXT(" | ")));
        if (!Diagnostic.Contains(TEXT("finite=yes")) || Diagnostic.Contains(TEXT("cameraInvariant=ok")) == false
            || Diagnostic.Contains(TEXT("OUTSIDE")))
        {
            if (!bReportedInvalidPresentation)
            {
                UE_LOG(LogTemp, Error, TEXT("5V presentation invariant violation: %s"), *Diagnostic);
                bReportedInvalidPresentation = true;
            }
        }
    }
    if (bPresentationDiagnosticsVisible && MatchState.SimulationFrame != LastBoundaryTraceFrame
        && (!MatchState.Fighter1.bGrounded || !MatchState.Fighter2.bGrounded)
        && (static_cast<int64>(MatchState.Fighter1.PositionX) * MatchState.Fighter1.PositionX
                + static_cast<int64>(MatchState.Fighter1.PositionZ) * MatchState.Fighter1.PositionZ
                > static_cast<int64>(420 * FFightingSimulation::SubunitsPerWorldUnit)
                    * (420 * FFightingSimulation::SubunitsPerWorldUnit)
            || static_cast<int64>(MatchState.Fighter2.PositionX) * MatchState.Fighter2.PositionX
                + static_cast<int64>(MatchState.Fighter2.PositionZ) * MatchState.Fighter2.PositionZ
                > static_cast<int64>(420 * FFightingSimulation::SubunitsPerWorldUnit)
                    * (420 * FFightingSimulation::SubunitsPerWorldUnit)))
    {
        // Trace every rendered simulation state during a boundary jump. This
        // brackets the last sane frame if the visual disappears between jumps.
        LastBoundaryTraceFrame = MatchState.SimulationFrame;
        UE_LOG(LogTemp, Display, TEXT("5V boundary-air trace | %s"),
            *GetPresentationDiagnosticText().Replace(TEXT("\n"), TEXT(" | ")));
    }
    // Permanent visual stage markings: exact 480 arena edge, 455 fighter-center
    // limit, depth rings and compass references. None have collision.
    DrawDebugCircle(GetWorld(), FVector(0, 0, 1), 480.0, 128, FColor(115, 220, 225), false, 0.05f, 0, 2.0f,
        FVector(1, 0, 0), FVector(0, 1, 0));
    for (const double Radius : {120.0, 240.0, 360.0})
    {
        DrawDebugCircle(GetWorld(), FVector(0, 0, 0), Radius, 96, FColor(55, 86, 100), false, 0.05f, 0, 0.8f,
            FVector(1, 0, 0), FVector(0, 1, 0));
    }
    for (int32 Index = 0; Index < 36; ++Index)
    {
        const double A0 = Index * 10.0 * PI / 180.0;
        const double A1 = A0 + 5.0 * PI / 180.0;
        DrawDebugLine(GetWorld(), FVector(455.0 * FMath::Cos(A0), 455.0 * FMath::Sin(A0), 1),
            FVector(455.0 * FMath::Cos(A1), 455.0 * FMath::Sin(A1), 1), FColor(245, 185, 95), false, 0.05f, 0, 2.0f);
    }
    for (int32 Index = 0; Index < 8; ++Index)
    {
        const double Angle = Index * PI / 4.0;
        DrawDebugLine(GetWorld(), FVector(65.0 * FMath::Cos(Angle), 65.0 * FMath::Sin(Angle), 0),
            FVector(440.0 * FMath::Cos(Angle), 440.0 * FMath::Sin(Angle), 0), FColor(48, 72, 82), false, 0.05f, 0, 1.0f);
    }

    if (bDebugGeometryVisible && Fighter1Visual && Fighter2Visual)
    {
        DrawDebugLine(GetWorld(), Fighter1Visual->GetActorLocation(), Fighter2Visual->GetActorLocation(),
            FColor::Cyan, false, 0.05f, 0, 1.0f);
        const double PresentedAxisYaw = FMath::DegreesToRadians(DisplayedCombatYawDegrees + 90.0);
        const FVector PresentedAxis(FMath::Cos(PresentedAxisYaw), FMath::Sin(PresentedAxisYaw), 0.0);
        const FVector PairCenter((Fighter1Visual->GetActorLocation().X + Fighter2Visual->GetActorLocation().X) * 0.5,
            (Fighter1Visual->GetActorLocation().Y + Fighter2Visual->GetActorLocation().Y) * 0.5, 2.0);
        DrawDebugLine(GetWorld(), PairCenter - PresentedAxis * 100.0, PairCenter + PresentedAxis * 100.0,
            FColor::Yellow, false, 0.05f, 0, 1.5f);
        for (const FFighterState* Fighter : {&MatchState.Fighter1, &MatchState.Fighter2})
        {
            const FSimBox Hurt = FFightingSimulation::GetHurtbox(*Fighter);
            const FVector HurtCenter((Hurt.MinX + Hurt.MaxX) * 0.5 / WorldUnits,
                (Hurt.MinZ + Hurt.MaxZ) * 0.5 / WorldUnits, (Hurt.MinY + Hurt.MaxY) * 0.5 / WorldUnits);
            DrawDebugBox(GetWorld(), HurtCenter, FVector((Hurt.MaxX-Hurt.MinX)*0.5/WorldUnits,
                (Hurt.MaxZ-Hurt.MinZ)*0.5/WorldUnits, (Hurt.MaxY-Hurt.MinY)*0.5/WorldUnits),
                FColor::Green, false, 0.05f, 0, 1.5f);
            FSimOrientedBox Attack;
            if (FFightingSimulation::GetMoveHitVolume(*Fighter, Attack))
            {
                const FVector Center(Attack.CenterX / WorldUnits, Attack.CenterZ / WorldUnits,
                    (Attack.MinY + Attack.MaxY) * 0.5 / WorldUnits);
                const double AttackYaw = FMath::RadiansToDegrees(FMath::Atan2(
                    static_cast<double>(Attack.AxisZ), static_cast<double>(Attack.AxisX)));
                DrawDebugBox(GetWorld(), Center, FVector(Attack.HalfForward / WorldUnits,
                    Attack.HalfLateral / WorldUnits, (Attack.MaxY-Attack.MinY)*0.5/WorldUnits),
                    FRotator(0, AttackYaw, 0).Quaternion(), FColor::Red, false, 0.05f, 0, 2.0f);
            }
            FSimRadialVolume Radial;
            if (FFightingSimulation::GetMoveRadialVolume(*Fighter, Radial))
            {
                const FVector Center(Radial.CenterX / WorldUnits, Radial.CenterZ / WorldUnits,
                    (Radial.MinY + Radial.MaxY) * 0.5 / WorldUnits);
                DrawDebugCircle(GetWorld(), Center, Radial.Radius / WorldUnits, 64,
                    FColor::Orange, false, 0.05f, 0, 3.0f, FVector(1, 0, 0), FVector(0, 1, 0));
            }
        }
    }
}
