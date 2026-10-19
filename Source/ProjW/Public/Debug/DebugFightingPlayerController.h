#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Debug/DebugInputMapping.h"
#include "Simulation/FightingSimulationTypes.h"
#include "DebugFightingPlayerController.generated.h"

UCLASS()
class PROJW_API ADebugFightingPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    ADebugFightingPlayerController();
    FFighterInput BuildFrameInput(double ScreenRightAlignment);
    FFighterInput BuildOpponentFrameInput(double ScreenRightAlignment);
    FString GetInputDiagnosticText() const;
protected:
    virtual void BeginPlay() override;
private:
    FDebugPhysicalInput SamplePlayer1Keys() const;
    FDebugPhysicalInput SamplePlayer2Keys() const;
    int8 Player1MappingSign = 0;
    int8 Player2MappingSign = 0;
    FFighterInput LastPlayer1Input;
    FFighterInput LastPlayer2Input;
};
