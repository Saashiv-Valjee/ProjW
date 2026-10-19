#include "Debug/DebugFighterActor.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Simulation/FightingSimulation.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    enum class EVisualAction : uint8
    {
        Idle, WalkForward, WalkBackward, Crouch, JumpStartup, Jump, DoubleJump,
        ForwardDash, BackDash, Light, Medium, Heavy, GroundRupture, NarrowThrust, BroadSweep, SlowCommittedStrike, BlockHigh,
        BlockLow, BlockstunHigh, BlockstunLow, PivotClockwise, PivotCounterClockwise,
        HighParry, LowParry, ParrySuccess, ParryStun, Hitstun
    };

    struct FVisualSelection
    {
        EVisualAction Action = EVisualAction::Idle;
        UAnimationAsset* Animation = nullptr;
        float PositionFraction = 0.0f;
        bool bLoop = false;
        bool bSampleBySimulationFrame = false;
    };

    FVisualSelection SelectVisual(const FFighterState& Fighter, const FFighterState& Opponent)
    {
        using Sim = FFightingSimulation;
        const bool bAwayHeld = Fighter.Facing == EFighterFacing::Right
            ? Fighter.PreviousInput.IsHeld(EFighterInputButton::Left)
            : Fighter.PreviousInput.IsHeld(EFighterInputButton::Right);
        const bool bCrouchGuard = bAwayHeld && Fighter.PreviousInput.IsHeld(EFighterInputButton::Down);
        FVisualSelection Result;
        switch (Fighter.CurrentState)
        {
            case EFighterState::Idle:
                Result.Action = bAwayHeld ? EVisualAction::BlockHigh : EVisualAction::Idle;
                Result.bLoop = true;
                break;
            case EFighterState::Walking:
            {
                const int64 DX = static_cast<int64>(Opponent.PositionX) - Fighter.PositionX;
                const int64 DZ = static_cast<int64>(Opponent.PositionZ) - Fighter.PositionZ;
                const int64 TowardDot = DX * Fighter.VelocityX + DZ * Fighter.VelocityZ;
                const double AxisLength = FMath::Sqrt(static_cast<double>(DX) * DX + static_cast<double>(DZ) * DZ);
                const double VelocityLength = FMath::Sqrt(static_cast<double>(Fighter.VelocityX) * Fighter.VelocityX
                    + static_cast<double>(Fighter.VelocityZ) * Fighter.VelocityZ);
                const double TowardSpeed = AxisLength > 0.0 ? TowardDot / AxisLength : 0.0;
                constexpr double VisualMovementDeadZone = 64.0; // 1/16 world unit per simulation frame.
                if (VelocityLength <= VisualMovementDeadZone || FMath::Abs(TowardSpeed) <= VisualMovementDeadZone)
                {
                    Result.Action = EVisualAction::Idle;
                    Result.bLoop = true;
                }
                else
                {
                    Result.Action = TowardSpeed > 0.0
                        ? EVisualAction::WalkForward : EVisualAction::WalkBackward;
                    Result.bLoop = true;
                }
                break;
            }
            case EFighterState::Crouching:
                Result.Action = bCrouchGuard ? EVisualAction::BlockLow : EVisualAction::Crouch;
                Result.bLoop = true;
                break;
            case EFighterState::ForwardDash: Result.Action = EVisualAction::ForwardDash; Result.bLoop = true; break;
            case EFighterState::BackDash: Result.Action = EVisualAction::BackDash; Result.bLoop = true; break;
            case EFighterState::JumpStartup:
                Result.Action = EVisualAction::JumpStartup;
                Result.PositionFraction = static_cast<float>(Fighter.StateFrame)
                    / static_cast<float>(Sim::JumpStartupFrames);
                Result.bSampleBySimulationFrame = true;
                break;
            case EFighterState::Jumping: Result.Action = EVisualAction::Jump; Result.bLoop = true; break;
            case EFighterState::DoubleJumping: Result.Action = EVisualAction::DoubleJump; Result.bLoop = true; break;
            case EFighterState::Attack:
            {
                Result.Action = Fighter.CurrentMoveId == EMoveId::Light ? EVisualAction::Light
                    : Fighter.CurrentMoveId == EMoveId::Medium ? EVisualAction::Medium
                    : Fighter.CurrentMoveId == EMoveId::GroundRupture ? EVisualAction::GroundRupture
                    : Fighter.CurrentMoveId == EMoveId::NarrowThrust ? EVisualAction::NarrowThrust
                    : Fighter.CurrentMoveId == EMoveId::BroadSweep ? EVisualAction::BroadSweep
                    : Fighter.CurrentMoveId == EMoveId::SlowCommittedStrike ? EVisualAction::SlowCommittedStrike
                    : EVisualAction::Heavy;
                const FMoveDefinition* Move = Sim::FindMoveDefinition(Fighter.CurrentMoveId);
                if (Move)
                {
                    const uint32 Total = Move->StartupFrames + Move->ActiveFrames + Move->RecoveryFrames;
                    const uint32 PhaseFrame = Fighter.MovePhase == EMovePhase::Startup
                        ? Fighter.StateFrame
                        : Fighter.MovePhase == EMovePhase::Active
                            ? Move->StartupFrames + Fighter.StateFrame
                            : Move->StartupFrames + Move->ActiveFrames + Fighter.StateFrame;
                    Result.PositionFraction = Total > 1
                        ? FMath::Clamp(static_cast<float>(PhaseFrame) / static_cast<float>(Total - 1), 0.0f, 1.0f)
                        : 0.0f;
                    Result.bSampleBySimulationFrame = true;
                }
                break;
            }
            case EFighterState::Blockstun:
                Result.Action = bCrouchGuard ? EVisualAction::BlockstunLow : EVisualAction::BlockstunHigh;
                Result.PositionFraction = FMath::Clamp(static_cast<float>(Fighter.StateFrame) / 18.0f, 0.0f, 1.0f);
                Result.bSampleBySimulationFrame = true;
                break;
            case EFighterState::Pivoting:
                Result.Action = Fighter.PivotDirection > 0
                    ? EVisualAction::PivotCounterClockwise : EVisualAction::PivotClockwise;
                Result.PositionFraction = static_cast<float>(Fighter.StateFrame)
                    / static_cast<float>(Sim::PivotDurationFrames);
                Result.bLoop = false;
                Result.bSampleBySimulationFrame = true;
                break;
            case EFighterState::Parrying:
                Result.Action = Fighter.ParryHeight == EParryHeight::High
                    ? EVisualAction::HighParry : EVisualAction::LowParry;
                Result.PositionFraction = static_cast<float>(Fighter.StateFrame)
                    / static_cast<float>(Sim::ParryTotalFrames);
                Result.bSampleBySimulationFrame = true;
                break;
            case EFighterState::ParrySuccess: Result.Action = EVisualAction::ParrySuccess; break;
            case EFighterState::ParryStun:
                Result.Action = EVisualAction::ParryStun;
                Result.PositionFraction = FMath::Clamp(static_cast<float>(Fighter.StateFrame) / Sim::ParryStunFrames, 0.0f, 1.0f);
                Result.bSampleBySimulationFrame = true;
                break;
            case EFighterState::Hitstun:
                Result.Action = EVisualAction::Hitstun;
                Result.PositionFraction = FMath::Clamp(static_cast<float>(Fighter.StateFrame) / 20.0f, 0.0f, 1.0f);
                Result.bSampleBySimulationFrame = true;
                break;
        }

        return Result;
    }
}

ADebugFighterActor::ADebugFighterActor()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SimulationPosition"));
    FighterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FighterPresentation"));
    FighterMesh->SetupAttachment(RootComponent);
    FighterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FighterMesh->SetGenerateOverlapEvents(false);
    FighterMesh->SetSimulatePhysics(false);
    FighterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    FighterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    FighterMesh->SetRelativeScale3D(FVector(0.78));
    CrouchPoseMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("CrouchPosePresentation"));
    CrouchPoseMesh->SetupAttachment(RootComponent);
    CrouchPoseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CrouchPoseMesh->SetGenerateOverlapEvents(false);
    CrouchPoseMesh->SetVisibility(false);
    CrouchPoseMesh->SetRelativeScale3D(FVector(0.78));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    CueMesh = Sphere.Object;
    RingMesh = Cylinder.Object;
    FlatMaterial = Material.Object;

    ActionCue = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ActionCue"));
    ActionCue->SetupAttachment(RootComponent);
    ActionCue->SetStaticMesh(Cylinder.Object);
    ActionCue->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ActionCue->SetGenerateOverlapEvents(false);
    ActionCue->SetVisibility(false);

    GuardCue = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GuardCue"));
    GuardCue->SetupAttachment(RootComponent);
    GuardCue->SetStaticMesh(Cube.Object);
    GuardCue->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GuardCue->SetGenerateOverlapEvents(false);
    GuardCue->SetVisibility(false);

    TeamMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeamMarker"));
    TeamMarker->SetupAttachment(RootComponent);
    TeamMarker->SetStaticMesh(Sphere.Object);
    TeamMarker->SetRelativeLocation(FVector(8, 18, 124));
    TeamMarker->SetRelativeScale3D(FVector(0.15));
    TeamMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TeamMarker->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> WalkF(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> WalkB(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Bwd.MF_Unarmed_Walk_Bwd"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> JogF(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> JogB(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Bwd.MF_Unarmed_Jog_Bwd"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Jump(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Jump.MM_Jump"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> DoubleJump(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_WallJump.MM_WallJump"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Light(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Medium(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Heavy(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Rupture(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> Hit(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01"));
    static ConstructorHelpers::FObjectFinder<UAnimationAsset> HardHit(TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Hvy_01.MM_HitReact_Front_Hvy_01"));
    IdleAnimation = Idle.Object;
    WalkForwardAnimation = WalkF.Object;
    WalkBackwardAnimation = WalkB.Object;
    JogForwardAnimation = JogF.Object;
    JogBackwardAnimation = JogB.Object;
    JumpAnimation = Jump.Object;
    DoubleJumpAnimation = DoubleJump.Object;
    LightAnimation = Light.Object;
    MediumAnimation = Medium.Object;
    HeavyAnimation = Heavy.Object;
    GroundRuptureAnimation = Rupture.Object;
    HitReactAnimation = Hit.Object;
    ChargedHitReactAnimation = HardHit.Object;
}

void ADebugFighterActor::SetCharacter(bool bUseQuinn)
{
    // P1 and P2 use the same positive-scale presentation; the fixed mesh-space
    // yaw in ApplyState aligns each mannequin's anatomical forward with actor +X.
    const TCHAR* Path = bUseQuinn
        ? TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")
        : TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
    if (USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Path))
    {
        FighterMesh->SetSkeletalMesh(Mesh);
        CrouchPoseMesh->SetSkinnedAssetAndUpdate(Mesh);
    }
}

void ADebugFighterActor::SetColor(const FLinearColor& Color)
{
    FighterColor = Color;
    if (FlatMaterial && TeamMarker)
    {
        TeamMaterial = UMaterialInstanceDynamic::Create(FlatMaterial, this);
        TeamMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        TeamMarker->SetMaterial(0, TeamMaterial);
    }
    if (TeamMaterial)
    {
        TeamMaterial->SetVectorParameterValue(TEXT("Color"), Color);
    }
}

void ADebugFighterActor::ApplyState(const FFighterState& State, const FFighterState& Opponent,
    float PresentationDeltaSeconds)
{
    const double Units = FFightingSimulation::SubunitsPerWorldUnit;
    const double DX = static_cast<double>(Opponent.PositionX) - State.PositionX;
    const double DZ = static_cast<double>(Opponent.PositionZ) - State.PositionZ;
    const double OpponentYaw = DX == 0 && DZ == 0 ? (State.Facing == EFighterFacing::Right ? 0.0 : 180.0)
        : FMath::RadiansToDegrees(FMath::Atan2(DZ, DX));
    // The mannequin's visible anatomical front is local +Y. A fixed -90
    // component yaw maps it to actor +X, the visual combat-forward axis.
    const double TargetYaw = OpponentYaw;
    const bool bPivotState = State.CurrentState == EFighterState::Pivoting;
    if (!bHasPresentationYaw)
    {
        PresentationYawDegrees = TargetYaw;
        bHasPresentationYaw = true;
    }
    else if (!bPivotState)
    {
        // Retain a stable body heading throughout Pivot; settle smoothly after
        // camera recenter instead of twisting with the changing opponent vector.
        PresentationYawDegrees = FMath::RInterpTo(FRotator(0, PresentationYawDegrees, 0),
            FRotator(0, TargetYaw, 0), FMath::Max(0.0f, PresentationDeltaSeconds), 12.0f).Yaw;
    }
    const double UnwrappedYaw = FMath::UnwindDegrees(PresentationYawDegrees);
    SetActorLocationAndRotation(FVector(State.PositionX / Units, State.PositionZ / Units, State.PositionY / Units),
        FRotator(0, UnwrappedYaw, 0), false, nullptr, ETeleportType::TeleportPhysics);

    FVisualSelection Visual = SelectVisual(State, Opponent);
    if (State.CurrentState == EFighterState::Blockstun)
    {
        const bool bPreviousPoseWasLowGuard = CurrentVisualAction == static_cast<uint8>(EVisualAction::BlockLow)
            || CurrentVisualAction == static_cast<uint8>(EVisualAction::BlockstunLow);
        if (State.PreviousInput.IsHeld(EFighterInputButton::Down) || bPreviousPoseWasLowGuard)
        {
            bPresentationBlockstunLow = true;
        }
        Visual.Action = bPresentationBlockstunLow ? EVisualAction::BlockstunLow : EVisualAction::BlockstunHigh;
    }
    else
    {
        bPresentationBlockstunLow = false;
    }
    UAnimationAsset* SelectedAnimation = nullptr;
    switch (Visual.Action)
    {
        case EVisualAction::Idle: SelectedAnimation = IdleAnimation; break;
        case EVisualAction::Crouch: case EVisualAction::BlockLow:
        case EVisualAction::LowParry: SelectedAnimation = IdleAnimation; break;
        case EVisualAction::WalkForward: SelectedAnimation = WalkForwardAnimation; break;
        case EVisualAction::WalkBackward: SelectedAnimation = WalkBackwardAnimation; break;
        case EVisualAction::JumpStartup: case EVisualAction::Jump: SelectedAnimation = JumpAnimation; break;
        case EVisualAction::DoubleJump: SelectedAnimation = DoubleJumpAnimation; break;
        case EVisualAction::ForwardDash: SelectedAnimation = JogForwardAnimation; break;
        case EVisualAction::BackDash: SelectedAnimation = JogBackwardAnimation; break;
        case EVisualAction::PivotCounterClockwise: case EVisualAction::PivotClockwise:
            // The copied mannequin pack contains no dodge/evade/side-step
            // one-shot, so use idle with a short authored-in-code pose curve.
            SelectedAnimation = IdleAnimation; break;
        case EVisualAction::Light: SelectedAnimation = LightAnimation; break;
        case EVisualAction::Medium: SelectedAnimation = MediumAnimation; break;
        case EVisualAction::Heavy: SelectedAnimation = HeavyAnimation; break;
        case EVisualAction::GroundRupture: SelectedAnimation = GroundRuptureAnimation; break;
        case EVisualAction::NarrowThrust: SelectedAnimation = LightAnimation; break;
        case EVisualAction::BroadSweep: SelectedAnimation = HeavyAnimation; break;
        case EVisualAction::SlowCommittedStrike: SelectedAnimation = GroundRuptureAnimation; break;
        case EVisualAction::BlockHigh: SelectedAnimation = IdleAnimation; break;
        case EVisualAction::HighParry: SelectedAnimation = IdleAnimation; break;
        case EVisualAction::BlockstunHigh: case EVisualAction::Hitstun: SelectedAnimation = HitReactAnimation; break;
        case EVisualAction::ParrySuccess: SelectedAnimation = IdleAnimation; break;
        case EVisualAction::ParryStun: SelectedAnimation = ChargedHitReactAnimation; break;
    }

    const uint8 VisualActionValue = static_cast<uint8>(Visual.Action);
    const bool bVisualChanged = CurrentVisualAction != VisualActionValue;
    if (FighterMesh && SelectedAnimation)
    {
        if (CurrentAnimation != SelectedAnimation || bVisualChanged)
        {
            FighterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            FighterMesh->SetAnimation(SelectedAnimation);
            CurrentAnimation = SelectedAnimation;
            if (UAnimInstance* AnimInstance = FighterMesh->GetAnimInstance())
            {
                AnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
            }
        }
        if (UAnimSingleNodeInstance* SingleNode = FighterMesh->GetSingleNodeInstance())
        {
            SingleNode->SetLooping(Visual.bLoop);
            if (Visual.bSampleBySimulationFrame || !Visual.bLoop)
            {
                const float Length = FMath::Max(0.0f, CurrentAnimation->GetPlayLength());
                const float SafeEnd = FMath::Max(0.0f, Length - FMath::Max(KINDA_SMALL_NUMBER, Length * 0.0001f));
                const float SampleTime = FMath::Clamp(Visual.PositionFraction, 0.0f, 1.0f) * SafeEnd;
                SingleNode->SetPosition(SampleTime, false);
                SingleNode->SetPlaying(Visual.bLoop);
            }
            else
            {
                SingleNode->SetPlaying(true);
            }
        }
        CurrentVisualAction = VisualActionValue;
    }

    const bool bCrouch = Visual.Action == EVisualAction::Crouch
        || Visual.Action == EVisualAction::BlockLow || Visual.Action == EVisualAction::BlockstunLow
        || Visual.Action == EVisualAction::LowParry;
    // No crouch clip exists in the bundled mannequin set. This compressed,
    // slightly wider low stance is presentation-only and keeps the root/feet
    // close to their standing floor anchor; it does not alter simulation Y.
    CrouchPoseBlend = FMath::FInterpConstantTo(CrouchPoseBlend, bCrouch ? 1.0f : 0.0f,
        PresentationDeltaSeconds, 5.0f);
    FVector MeshOffset(0, 0, 0);
    FVector MeshScale(0.78);
    FRotator MeshPose = FRotator::ZeroRotator;
    if (bPivotState)
    {
        const float T = FMath::Clamp(static_cast<float>(State.StateFrame) / 11.0f, 0.0f, 1.0f);
        const float Weave = FMath::Sin(T * PI);
        const float Direction = State.PivotDirection > 0 ? 1.0f : -1.0f;
        // Weight shift -> committed weave -> plant. Ignore-root-motion is already
        // set on the animation instance; simulation alone supplies displacement.
        MeshOffset.X = 7.0f * (1.0f - T) + 5.0f * T;
        MeshOffset.Z = -8.0f * (1.0f - T) - 3.0f * Weave;
        MeshScale = FVector(0.78f - 0.025f * Weave, 0.78f, 0.78f - 0.08f * Weave);
        MeshPose.Roll = Direction * 7.0f * Weave;
        MeshPose.Pitch = -5.0f * Weave;
    }
    FighterMesh->SetRelativeLocation(MeshOffset);
    MeshPose.Yaw = -90.0;
    FighterMesh->SetRelativeRotation(MeshPose);
    FighterMesh->SetRelativeScale3D(MeshScale);
    const bool bShowCrouchPose = CrouchPoseBlend > 0.001f;
    if (CrouchPoseMesh && bShowCrouchPose)
    {
        CrouchPoseMesh->CopyPoseFromSkeletalComponent(FighterMesh);
        CrouchPoseMesh->SetRelativeLocation(MeshOffset);
        CrouchPoseMesh->SetRelativeRotation(MeshPose);
        CrouchPoseMesh->SetRelativeScale3D(MeshScale);
        const float Weight = CrouchPoseBlend;
        const auto AddRotation = [this, Weight](FName Bone, const FRotator& Delta)
        {
            if (FighterMesh->GetBoneIndex(Bone) == INDEX_NONE) return;
            const FRotator Base = CrouchPoseMesh->GetBoneRotationByName(Bone, EBoneSpaces::ComponentSpace);
            CrouchPoseMesh->SetBoneRotationByName(Bone, Base + Delta * Weight, EBoneSpaces::ComponentSpace);
        };
        if (FighterMesh->GetBoneIndex(TEXT("pelvis")) != INDEX_NONE)
        {
            FTransform Pelvis = CrouchPoseMesh->GetBoneTransformByName(TEXT("pelvis"), EBoneSpaces::ComponentSpace);
            Pelvis.AddToTranslation(FVector(0, 0, -24.0f * Weight));
            CrouchPoseMesh->SetBoneTransformByName(TEXT("pelvis"), Pelvis, EBoneSpaces::ComponentSpace);
        }
        AddRotation(TEXT("thigh_l"), FRotator(28, 0, 0));
        AddRotation(TEXT("thigh_r"), FRotator(28, 0, 0));
        AddRotation(TEXT("calf_l"), FRotator(-62, 0, 0));
        AddRotation(TEXT("calf_r"), FRotator(-62, 0, 0));
        AddRotation(TEXT("spine_01"), FRotator(-10, 0, 0));
        AddRotation(TEXT("spine_02"), FRotator(-8, 0, 0));
    }
    if (CrouchPoseMesh) CrouchPoseMesh->SetVisibility(bShowCrouchPose);
    FighterMesh->SetVisibility(!bShowCrouchPose);
    TeamMarker->SetVisibility(true);
    TeamMarker->SetRelativeLocation(bCrouch ? FVector(8, 18, 88) : FVector(8, 18, 124));

    const bool bGuard = Visual.Action == EVisualAction::BlockHigh || Visual.Action == EVisualAction::BlockLow
        || Visual.Action == EVisualAction::BlockstunHigh || Visual.Action == EVisualAction::BlockstunLow;
    GuardCue->SetVisibility(bGuard);
    if (bGuard)
    {
        GuardCue->SetRelativeLocation(FVector(36, 0, bCrouch ? 52 : 104));
        GuardCue->SetRelativeScale3D(FVector(0.28, 0.13, bCrouch ? 0.48 : 0.58));
        if (!CueMaterial)
        {
            CueMaterial = UMaterialInstanceDynamic::Create(FlatMaterial, this);
            GuardCue->SetMaterial(0, CueMaterial);
            ActionCue->SetMaterial(0, CueMaterial);
        }
        CueMaterial->SetVectorParameterValue(TEXT("Color"), bCrouch
            ? FLinearColor(0.5f, 0.68f, 0.8f, 1.0f) : FLinearColor(0.7f, 0.78f, 0.82f, 1.0f));
    }

    const bool bParry = Visual.Action == EVisualAction::HighParry || Visual.Action == EVisualAction::LowParry;
    const bool bSuccess = Visual.Action == EVisualAction::ParrySuccess;
    const bool bParryStun = Visual.Action == EVisualAction::ParryStun;
    const bool bHit = Visual.Action == EVisualAction::Hitstun;
    const bool bPivot = Visual.Action == EVisualAction::PivotClockwise || Visual.Action == EVisualAction::PivotCounterClockwise;
    const bool bRuptureActive = Visual.Action == EVisualAction::GroundRupture && State.MovePhase == EMovePhase::Active;
    const bool bAttackActive = (Visual.Action == EVisualAction::Light || Visual.Action == EVisualAction::Medium
        || Visual.Action == EVisualAction::Heavy) && State.MovePhase == EMovePhase::Active;
    const bool bCue = bParry || bSuccess || bParryStun || bHit || bPivot || bRuptureActive || bAttackActive;
    ActionCue->SetVisibility(bCue);
    if (bCue)
    {
        if (!CueMaterial)
        {
            CueMaterial = UMaterialInstanceDynamic::Create(FlatMaterial, this);
            GuardCue->SetMaterial(0, CueMaterial);
            ActionCue->SetMaterial(0, CueMaterial);
        }
        const FLinearColor CueColor = bSuccess ? FLinearColor(0.25f, 1.0f, 0.55f)
            : bParry ? (Visual.Action == EVisualAction::HighParry
                ? FLinearColor(0.1f, 0.85f, 1.0f) : FLinearColor(0.65f, 0.4f, 1.0f))
            : bParryStun ? FLinearColor(1.0f, 0.62f, 0.12f)
            : bHit ? FLinearColor(1.0f, 0.12f, 0.08f)
            : bPivot ? FLinearColor(0.2f, 0.9f, 0.85f)
            : bRuptureActive ? FLinearColor(1.0f, 0.4f, 0.06f)
            : FLinearColor(1.0f, 0.85f, 0.12f);
        CueMaterial->SetVectorParameterValue(TEXT("Color"), CueColor);
        if (bRuptureActive)
        {
            ActionCue->SetStaticMesh(RingMesh);
            ActionCue->SetRelativeLocation(FVector(0, 0, 3));
            ActionCue->SetRelativeScale3D(FVector(2.24, 2.24, 0.025));
        }
        else if (bParry || bSuccess || bParryStun)
        {
            ActionCue->SetStaticMesh(CueMesh);
            const float Height = Visual.Action == EVisualAction::LowParry ? 25.0f : 128.0f;
            ActionCue->SetRelativeLocation(FVector(0, 0, Height));
            ActionCue->SetRelativeScale3D(FVector(bSuccess ? 1.15 : 0.68, bSuccess ? 1.15 : 0.68, 0.035));
        }
        else if (bHit)
        {
            ActionCue->SetRelativeLocation(FVector(-18, 0, 90));
            ActionCue->SetRelativeScale3D(FVector(0.42, 0.42, 0.42));
        }
        else if (bPivot)
        {
            ActionCue->SetStaticMesh(RingMesh);
            ActionCue->SetRelativeLocation(FVector(0, 0, 5));
            ActionCue->SetRelativeScale3D(FVector(0.7, 0.7, 0.025));
        }
        else
        {
            ActionCue->SetStaticMesh(CueMesh);
            ActionCue->SetRelativeLocation(FVector(52, 0, 100));
            ActionCue->SetRelativeScale3D(FVector(0.24, 0.24, 0.24));
        }
    }
}
