#include "Debug/DebugFightingGameMode.h"
#include "Debug/DebugFightingPlayerController.h"
#include "Debug/DebugFightingHUD.h"
#include "Debug/DebugMatchDriver.h"
#include "Engine/World.h"

ADebugFightingGameMode::ADebugFightingGameMode()
{
    PlayerControllerClass = ADebugFightingPlayerController::StaticClass();
    HUDClass = ADebugFightingHUD::StaticClass();
    DefaultPawnClass = nullptr; // No default pawn movement or camera.
}

void ADebugFightingGameMode::StartPlay()
{
    GetWorld()->SpawnActor<ADebugMatchDriver>();
    Super::StartPlay();
}
