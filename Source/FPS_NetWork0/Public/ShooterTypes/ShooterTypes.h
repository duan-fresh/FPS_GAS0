#pragma once

#include "ShooterTypes.generated.h"

UENUM(BlueprintType)
enum class ETurning:uint8
{
	Left UMETA(DisplayName="TurningLeft"),
	Right UMETA(DisplayName="TurningRight"),
	NotTurning UMETA(DisplayName="Is NotTurning")
};

USTRUCT(BlueprintType)
struct FReticleParams
{
	GENERATED_BODY()
//ShapeCut
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ShapeCutFactor_RoundFired = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ShapeCutFactor_Aiming = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ShapeCutFactor_NotAiming = 0.f;

//Sacle
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScaleFactor_RoundFired = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScaleFactor_Aiming = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScaleFactor_NotAiming = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScaleFactor_Targeting = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ScaleFactor_NotTargeting = 0.f;
	
//Speed
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RoundFiredInterpSpeed = 20.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float AimingInterpSpeed = 15.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TargetingPlayerInterpSpeed = 10.f;
};

UENUM(meta=(BitFlags))
enum class ESpecialElimType: uint16
{
	None = 0,
	Headshot		= 1 << 0,		
	Sequential		= 1 << 1,		
	Streak			= 1 << 2,		
	Revenge			= 1 << 3,		
	Dethrone		= 1 << 4,
	Showstopper		= 1 << 5,		
	FirstBlood		= 1 << 6,		
	GainedTheLead	= 1 << 7,		
	TiedTheLeader	= 1 << 8,		
	LostTheLead		= 1 << 9		
};

ENUM_CLASS_FLAGS(ESpecialElimType)//Defines all bitwise operators for enum classes so it can be (mostly) used as a regular flags enum允许二进制，字节运算
