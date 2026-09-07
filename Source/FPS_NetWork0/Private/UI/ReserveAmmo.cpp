


#include "UI/ReserveAmmo.h"

#include "Character/FPSCharacter.h"
#include "Combat/CombatComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Interfaces/PlayerInterface.h"
#include "Materials/MaterialInterface.h"
#include "Weapon/Weapon.h"

void UReserveAmmo::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	Image_WeaponIcon->SetRenderOpacity(0.f);
	Text_Ammo->SetRenderOpacity(0.f);
	
	GetOwningPlayer()->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChanged);
	
	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetOwningPlayer()->GetPawn());
	if (!IsValid(FPSCharacter)) return;
	OnPossessedPawnChanged(nullptr, FPSCharacter);
	
	if (FPSCharacter->HasWeaponFirstReplicated())
	{
		AWeapon* Weapon = IPlayerInterface::Execute_GetCurrentWeapon(FPSCharacter);
		if (IsValid(Weapon))
		{
			OnCurrentReserveAmmoChanged(IPlayerInterface::Execute_GetReserveAmmo(FPSCharacter), Weapon->Ammo,Weapon->WeaponIcon);
		}
	}
	else
	{
		FPSCharacter->OnWeaponFirstReplicated.AddDynamic(this, &ThisClass::OnWeaponFirstReplicated);
	}
	if (FPSCharacter->HasAuthority())
	{
		AWeapon* Weapon = IPlayerInterface::Execute_GetCurrentWeapon(FPSCharacter);
		if (!IsValid(Weapon)) return;
		OnCurrentReserveAmmoChanged(IPlayerInterface::Execute_GetReserveAmmo(FPSCharacter), Weapon->Ammo,Weapon->WeaponIcon);
	}
}

void UReserveAmmo::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	UCombatComponent* OldPawnCombat = UCombatComponent::FindCombatComponent(OldPawn);
	if (IsValid(OldPawnCombat))
	{
		OldPawnCombat->OnCurrentReserveAmmoChanged.RemoveDynamic(this, &ThisClass::OnCurrentReserveAmmoChanged);
		OldPawnCombat->OnRoundsChanged.RemoveDynamic(this, &ThisClass::OnRoundsChanged);
	}
	UCombatComponent* NewPawnCombat = UCombatComponent::FindCombatComponent(NewPawn);
	if (IsValid(NewPawnCombat))
	{
		Image_WeaponIcon->SetRenderOpacity(1.f);
		Text_Ammo->SetRenderOpacity(1.f);
		NewPawnCombat->OnCurrentReserveAmmoChanged.AddDynamic(this, &ThisClass::OnCurrentReserveAmmoChanged);
		NewPawnCombat->OnRoundsChanged.AddDynamic(this, &ThisClass::OnRoundsChanged);
		
		//Warning:修改，防止在死亡后重生图标未修改
		AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetOwningPlayer()->GetPawn());
		if (!IsValid(FPSCharacter)) return;
		AWeapon* Weapon = IPlayerInterface::Execute_GetCurrentWeapon(FPSCharacter);
		if (!IsValid(Weapon)) return;
		OnCurrentReserveAmmoChanged(IPlayerInterface::Execute_GetReserveAmmo(FPSCharacter), Weapon->Ammo,Weapon->WeaponIcon);
		
	}
}

void UReserveAmmo::OnCurrentReserveAmmoChanged(int32 RoundsInReserve, int32 RoundsInWeapon,UMaterialInterface* WeaponIconMaterial)
{
	if (IsValid(WeaponIconMaterial))
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(WeaponIconMaterial);
		if (IsValid(Image_WeaponIcon))
		{
			Image_WeaponIcon->SetBrush(Brush);
		}		
	}
	if (IsValid(Text_Ammo))
	{
		FText AmmoText = FText::Format(NSLOCTEXT("AmmoText", "AmmoKey", "{0}/{1}"), RoundsInWeapon, RoundsInReserve);
		Text_Ammo->SetText(AmmoText);
	}
}

void UReserveAmmo::OnRoundsChanged(int32 CurRounds, int32 MaxRounds,int32 RoundsInReserve)
{
	if (IsValid(Text_Ammo))
	{
		FText AmmoText = FText::Format(NSLOCTEXT("AmmoText", "AmmoKey", "{0}/{1}"), CurRounds, RoundsInReserve);
		Text_Ammo->SetText(AmmoText);
	}
}

void UReserveAmmo::OnWeaponFirstReplicated(AWeapon* Weapon, bool bTargetingPlayer)
{
	AFPSCharacter* FPSCharacter = Cast<AFPSCharacter>(GetOwningPlayer()->GetPawn());
	if (!IsValid(FPSCharacter)) return;
	
	OnCurrentReserveAmmoChanged(IPlayerInterface::Execute_GetReserveAmmo(FPSCharacter), Weapon->Ammo,Weapon->WeaponIcon);
}
