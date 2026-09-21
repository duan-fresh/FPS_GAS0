// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/FPSGameMode.h"

//Modify//
#include "Player/FPSPlayerState.h"
//Modify//
#include "GameFramework/Character.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void AFPSGameMode::RequestRespawn(ACharacter* Character, AController* Controller)
{
	if (!IsValid(Character) || !IsValid(Controller)) return;

	//Modify//
	// ---------------- 重生前把属性拉回满值 ----------------
	// ASC 与 AttributeSet 挂在 AFPSPlayerState 上，会【跨死亡与重生保留】。
	// 也就是说角色虽然换了新的，属于 PlayerState 的血量还停在死亡那一刻的 0。
	// 不重置的话，新角色一出生就满足"血量<=0"，会立刻再死一次（死循环）。
	//
	// 放在 Destroy 之前调：此刻 GetAvatarActor() 还指向旧角色，
	// GE 结算后仍能把值推进旧角色的 UHealthComponent（此时它还活着，广播是安全的）；
	// 新角色随后在 AFPSCharacter::InitializeAbilitySystem 里重新同步一次。
	if (AFPSPlayerState* FPSPlayerState = Controller->GetPlayerState<AFPSPlayerState>())
	{
		FPSPlayerState->ApplyResetAttributesEffect();
	}
	//Modify//

	Character->Reset();
	Character->Destroy();
	
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
	ensure(PlayerStarts.Num() > 0);
	int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
	
	RestartPlayerAtPlayerStart(Controller, PlayerStarts[Selection]);
}
