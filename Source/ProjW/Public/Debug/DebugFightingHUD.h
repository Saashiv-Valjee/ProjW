#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DebugFightingHUD.generated.h"

class ADebugMatchDriver;

UCLASS()
class PROJW_API ADebugFightingHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    TWeakObjectPtr<ADebugMatchDriver> Driver;
};
