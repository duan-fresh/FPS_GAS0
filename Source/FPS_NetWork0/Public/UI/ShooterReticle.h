#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterTypes/ShooterTypes.h"
#include "ShooterReticle.generated.h"

class AWeapon;
class UImage;

UCLASS()
class FPS_NETWORK0_API UShooterReticle : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> Reticle;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> AmmoCounter;
	
	FReticleParams CurrentReticleParams;
	float BaseCornerScaleFactor;
	float BaseShapeCutFactor;
	float _BaseCornerScaleFactor_RoundFired;
	float _BaseShapeCutFactor_RoundFired;
	float _BaseCornerScaleFactor_Aiming;
	float _BaseShapeCutFactor_Aiming;
	float _BaseCornerScaleFactor_TargetingPlayer;
	bool bAiming;
	bool bHitPlayer;
private:
	UPROPERTY()
	TWeakObjectPtr<UMaterialInstanceDynamic> ReticleMaterial;
	
	UPROPERTY()
	TWeakObjectPtr<UMaterialInstanceDynamic> AmmoCounterMaterial;
	
	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	
	UFUNCTION()
	void OnWeaponFirstReplicated(AWeapon* Weapon);
	
	UFUNCTION()
	void OnAmmoCounterChanged(UMaterialInstanceDynamic* AmmoCounterInstanceDynamic, int32 CurRounds,
	int32 MaxRounds);
	
	UFUNCTION()
	void OnReticleChanged(UMaterialInstanceDynamic* ReticleInstanceDynamic,const FReticleParams& ReticleParams,bool bCurrentlyTargetingPlayer);
	
	UFUNCTION()
	void OnRoundsChanged(int32 CurRounds,int32 MaxRounds);
	
	UFUNCTION()
	void OnAimingStatusChanged(bool bIsAiming);
	
	UFUNCTION()
	void OnHitPlayerStatusChanged(bool OnHitPlayerStatusChanged);
};
