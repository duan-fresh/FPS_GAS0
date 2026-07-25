#pragma once

#include "ShooterTypes.generated.h"

UENUM(BlueprintType)
enum class ETurning:uint8
{
	Left UMETA(DisplayName="TurningLeft"),
	Right UMETA(DisplayName="TurningRight"),
	NotTurning UMETA(DisplayName="Is NotTurning")
};


