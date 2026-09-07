

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReserveAmmo.generated.h"

class AWeapon;
class UTextBlock;
class UImage;

UCLASS()
class FPS_NETWORK0_API UReserveAmmo : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeOnInitialized() override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_WeaponIcon;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Ammo;
private:
	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn,APawn* NewPawn);
	
	//响应三个回调
	UFUNCTION()
	void OnCurrentReserveAmmoChanged(int32 RoundsInReserve, int32 RoundsInWeapon, UMaterialInterface* WeaponIconMaterial);
	
	UFUNCTION()
	void OnRoundsChanged(int32 CurRounds,int32 MaxRounds,int32 RoundsInReserve);
	
	UFUNCTION()
	void OnWeaponFirstReplicated(AWeapon* Weapon, bool bTargetingPlayer);
};
