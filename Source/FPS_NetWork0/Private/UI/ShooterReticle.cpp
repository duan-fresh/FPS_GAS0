#include "UI/ShooterReticle.h"

#include "Character/FPSCharacter.h"
#include "Combat/CombatComponent.h"
#include "Components/Image.h"
#include "Weapon/Weapon.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace Ammo
{
	const FName Rounds_Current = FName("Rounds_Current");
	const FName Rounds_Max = FName("Rounds_Max");
}

namespace Reticle
{
	const FName RoundedCornerScale = FName("RoundedCornerScale");
	const FName ShapeCutThickness = FName("ShapeCutThickness");
	const FName Inner_RGBA = FName("Inner_RGBA");
}

void UShooterReticle::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Reticle->SetRenderOpacity(0.0f);
	AmmoCounter->SetRenderOpacity(0.0f);
	
	_BaseCornerScaleFactor_RoundFired=0.0f;
	_BaseShapeCutFactor_RoundFired=0.0f;
	
	_BaseCornerScaleFactor_Aiming = 0.f;
	_BaseShapeCutFactor_Aiming = 0.f;
	bAiming = false;
	
	GetOwningPlayer()->OnPossessedPawnChanged.AddDynamic(this,&ThisClass::OnPossessedPawnChanged);
	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetOwningPlayer()->GetPawn());
	if (!IsValid(FPSCharacter)) return;
	OnPossessedPawnChanged(nullptr,FPSCharacter);
	
	if (FPSCharacter->HasCurrentWeapon())
	{
		AWeapon* Weapon=FPSCharacter->CombatComponent->CurrentWeapon;
		OnReticleChanged(Weapon->GetReticleInstance(),Weapon->ReticleParams,false);
		OnAmmoCounterChanged(Weapon->GetAmmoCounterInstance(),Weapon->Ammo,Weapon->MaxCapacity);
	}
	else
	{
		FPSCharacter->OnWeaponFirstReplicated.AddDynamic(this,&ThisClass::OnWeaponFirstReplicated);
	}
	if (FPSCharacter->HasAuthority())
	{
		AWeapon* Weapon=FPSCharacter->CombatComponent->CurrentWeapon;
		OnReticleChanged(Weapon->GetReticleInstance(),Weapon->ReticleParams,false);
		OnAmmoCounterChanged(Weapon->GetAmmoCounterInstance(),Weapon->Ammo,Weapon->MaxCapacity);
	}
}

void UShooterReticle::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	_BaseCornerScaleFactor_RoundFired=FMath::FInterpTo(_BaseCornerScaleFactor_RoundFired,0.0f,InDeltaTime,CurrentReticleParams.RoundFiredInterpSpeed);
	_BaseShapeCutFactor_RoundFired=FMath::FInterpTo(_BaseShapeCutFactor_RoundFired,0.0f,InDeltaTime,CurrentReticleParams.RoundFiredInterpSpeed);
	
	_BaseCornerScaleFactor_Aiming = FMath::FInterpTo(_BaseCornerScaleFactor_Aiming, bAiming ? CurrentReticleParams.ScaleFactor_Aiming : CurrentReticleParams.ScaleFactor_NotAiming, InDeltaTime, CurrentReticleParams.AimingInterpSpeed);
	_BaseShapeCutFactor_Aiming = FMath::FInterpTo(_BaseShapeCutFactor_Aiming, bAiming ? CurrentReticleParams.ShapeCutFactor_Aiming : CurrentReticleParams.ShapeCutFactor_NotAiming, InDeltaTime, CurrentReticleParams.AimingInterpSpeed);
	
	_BaseCornerScaleFactor_TargetingPlayer = FMath::FInterpTo(_BaseCornerScaleFactor_TargetingPlayer, bHitPlayer ? CurrentReticleParams.ScaleFactor_Targeting : CurrentReticleParams.ScaleFactor_NotTargeting, InDeltaTime, CurrentReticleParams.TargetingPlayerInterpSpeed);
	
	BaseCornerScaleFactor = _BaseCornerScaleFactor_RoundFired + _BaseCornerScaleFactor_Aiming + _BaseCornerScaleFactor_TargetingPlayer;
	BaseShapeCutFactor = _BaseShapeCutFactor_RoundFired + _BaseShapeCutFactor_Aiming;
	
	if (ReticleMaterial.IsValid())
	{
		ReticleMaterial->SetScalarParameterValue(Reticle::RoundedCornerScale, BaseCornerScaleFactor);
		ReticleMaterial->SetScalarParameterValue(Reticle::ShapeCutThickness, BaseShapeCutFactor);
	}
}

void UShooterReticle::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	UCombatComponent*OldCombat=UCombatComponent::FindCombatComponent(OldPawn);
	if (IsValid(OldCombat))
	{
		OldCombat->OnAmmoCounterChanged.RemoveDynamic(this,&ThisClass::OnAmmoCounterChanged);
		OldCombat->OnReticleChanged.RemoveDynamic(this,&ThisClass::OnReticleChanged);
		OldCombat->OnRoundsChanged.RemoveDynamic(this,&ThisClass::OnRoundsChanged);
		OldCombat->OnAimingStatusChanged.RemoveDynamic(this, &ThisClass::OnAimingStatusChanged);
		OldCombat->OnHitPlayerStatusChanged.RemoveDynamic(this,&ThisClass::OnHitPlayerStatusChanged);
	}
	UCombatComponent*NewCombat=UCombatComponent::FindCombatComponent(NewPawn);
	if (IsValid(NewCombat))
	{
		Reticle->SetRenderOpacity(1.0f);
		AmmoCounter->SetRenderOpacity(1.0f);
		NewCombat->OnAmmoCounterChanged.AddDynamic(this,&ThisClass::OnAmmoCounterChanged);
		NewCombat->OnReticleChanged.AddDynamic(this,&ThisClass::OnReticleChanged);
		NewCombat->OnRoundsChanged.AddDynamic(this,&ThisClass::OnRoundsChanged);
		NewCombat->OnAimingStatusChanged.AddDynamic(this, &ThisClass::OnAimingStatusChanged);
		NewCombat->OnHitPlayerStatusChanged.AddDynamic(this,&ThisClass::OnHitPlayerStatusChanged);
	}
}

void UShooterReticle::OnWeaponFirstReplicated(AWeapon* Weapon)
{
	OnReticleChanged(Weapon->GetReticleInstance(),Weapon->ReticleParams,false);
	OnAmmoCounterChanged(Weapon->GetAmmoCounterInstance(),Weapon->Ammo,Weapon->MaxCapacity);
}

void UShooterReticle::OnReticleChanged(UMaterialInstanceDynamic* ReticleInstanceDynamic,const FReticleParams& ReticleParams, bool bCurrentlyTargetingPlayer)
{
	CurrentReticleParams=ReticleParams;
	ReticleMaterial=ReticleInstanceDynamic;
	FSlateBrush ReticleBrush;
	ReticleBrush.SetResourceObject(ReticleInstanceDynamic);
	if (IsValid(Reticle))
	{
		Reticle->SetBrush(ReticleBrush);
	}
	OnHitPlayerStatusChanged(bCurrentlyTargetingPlayer);
}

void UShooterReticle::OnAmmoCounterChanged(UMaterialInstanceDynamic* AmmoCounterInstanceDynamic, int32 CurRounds,
	int32 MaxRounds)
{
	AmmoCounterMaterial=AmmoCounterInstanceDynamic;
	AmmoCounterMaterial->SetScalarParameterValue(Ammo::Rounds_Current,CurRounds);
	AmmoCounterMaterial->SetScalarParameterValue(Ammo::Rounds_Max,MaxRounds);
	FSlateBrush AmmoCounterBrush;
	AmmoCounterBrush.SetResourceObject(AmmoCounterInstanceDynamic);
	
	if (IsValid(AmmoCounter))
	{
		AmmoCounter->SetBrush(AmmoCounterBrush);
	}
}

void UShooterReticle::OnRoundsChanged(int32 CurRounds, int32 MaxRounds)
{
	_BaseCornerScaleFactor_RoundFired+=CurrentReticleParams.ScaleFactor_RoundFired;
	_BaseShapeCutFactor_RoundFired+=CurrentReticleParams.RoundFiredInterpSpeed;
	if (AmmoCounterMaterial.IsValid())
	{
		AmmoCounterMaterial->SetScalarParameterValue(Ammo::Rounds_Current,CurRounds);
		AmmoCounterMaterial->SetScalarParameterValue(Ammo::Rounds_Max,MaxRounds);
	}
}

void UShooterReticle::OnAimingStatusChanged(bool bIsAiming)
{
	bAiming=bIsAiming;
}

void UShooterReticle::OnHitPlayerStatusChanged(bool OnHitPlayerStatusChanged)
{
	bHitPlayer=OnHitPlayerStatusChanged;
	if (ReticleMaterial.IsValid())
	{
		ReticleMaterial->SetVectorParameterValue(Reticle::Inner_RGBA,bHitPlayer?FLinearColor::Red:FLinearColor::White);
	}
}





