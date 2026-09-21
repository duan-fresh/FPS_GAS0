
#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "GameFramework/PlayerState.h"
//Modify//
#include "AbilitySystemInterface.h"
//Modify//
#include "FPSPlayerState.generated.h"

struct FSpecialElimInfo;
class USpecialElim;
class USpecialElimData;
enum class ESpecialElimType : uint16;
//Modify//
class AFPSCharacter;
class UAbilitySystemComponent;
class UFPSAbilitySystemComponent;
class UFPSAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
//Modify//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScoreChanged, int32, NewScore);

/*
//original code
UCLASS()
class FPS_NETWORK0_API AFPSPlayerState : public APlayerState
{
*/
//Modify//
// 实现 IAbilitySystemInterface：ASC 与 AttributeSet 挂在 PlayerState 上，而不是 Character 上。
//
// 原因：AFPSGameMode::RequestRespawn 会先 Character->Destroy() 再重新生成 Pawn
// （见 Private/Game/FPSGameMode.cpp），挂在 Pawn 上的话每次重生都会连能力带属性一起丢掉。
// 挂在 PlayerState 上则跨死亡、跨重生保留 —— 代价是重生时必须显式把属性和能力重置，
// 见本文件末尾的 EnsureStartupState() 与 ApplyResetAttributesEffect()。
UCLASS()
class FPS_NETWORK0_API AFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
//Modify//
	GENERATED_BODY()
public:
	AFPSPlayerState();

	//Modify//
	// ---------------- GAS ----------------
	// AFPSCharacter 也会实现同一个接口并转发到这里，方便蓝图里直接对角色调用。
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "FPS|GAS")
	UFPSAttributeSet* GetFPSAttributeSet() const { return AttributeSet; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|GAS")
	TObjectPtr<UFPSAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|GAS")
	TObjectPtr<UFPSAttributeSet> AttributeSet;

	/** 首次拥有角色时施加：把 MaxHealth / MaxShield / Health / Shield 设成初始值（见 GE_InitAttributes）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|GAS")
	TSubclassOf<UGameplayEffect> InitAttributesEffect;

	/** 重生时施加：Health += MaxHealth、Shield += MaxShield，把属性拉回满值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|GAS")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;

	/**
	 * 服务器专用、幂等：只在第一次调用时授予角色能力并初始化属性。
	 * 之所以放在 PlayerState 上，是因为角色会重生重建，而 PlayerState 不会 ——
	 * 把"是否已经初始化"的状态放在角色上会导致重生后重复授予能力。
	 */
	void EnsureStartupState(AFPSCharacter* Character);

	/** 服务器专用：施加 ResetAttributesEffect，把属性和回满值。重生流程调用。 */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	void ApplyResetAttributesEffect();
	//Modify//

	UPROPERTY(BlueprintAssignable)
	FScoreChanged OnScoreChanged;
	
	void AddScoredElim();
	void AddDefeat();
	void AddHit();
	void AddMiss();
	void AddHeadShotElim();
	void AddSequentialElim(int32 SequenceCount);
	void UpdateHighestStreak(int32 StreakCount);
	void AddRevengeElim();
	void AddDethroneElim();
	void AddShowStopperElim();
	void GotFirstBlood();
	void IsNowWinner();
	
	//Streak
	void SetOnStreak(bool bIsOnStreak);
	bool IsOnStreak() const;
	//Attacker and Revenge
	void SetLastAttacker(APlayerState* Attacker);
	APlayerState* GetLastAttacker() const;
	//Leader
	int32 GetScoredElims() const;
	
	//FromServer to Client
	UFUNCTION(Client, Reliable)
	void Client_LostTheLead();
	
	UFUNCTION(Client, Reliable)
	void Client_ScoredElim(int32 ElimScore);
	
	UFUNCTION(Client, Reliable)
	void Client_SpecialElim(const ESpecialElimType& SpecialElim, int32 SequentialElimCount, int32 StreakCount, int32 ElimScore);
	
	//UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|SpecialElims")
	TObjectPtr<USpecialElimData> SpecialElimData;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|SpecialElims")
	TSubclassOf<USpecialElim> SpecialElimWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|SpecialElims")
	float ElimDisplayTime;
private:
	int32 ScoredElims;
	int32 Defeats;
	int32 Hits;
	int32 Misses;
	bool bOnStreak; // How many elims do we have since we've spawned?
	int32 HeadShotElims;
	TMap<int32, int32> SequentialElims; // Sequential elims - multiple elims within a short period of time
	int32 HighestStreak;
	int32 RevengeElims;
	int32 DethroneElims;
	int32 ShowStopperElims;
	bool bFirstBlood;
	bool bWinner;
	
	TWeakObjectPtr<APlayerState> LastAttacker;
	
	TArray<ESpecialElimType> DecodeElimBitmask(ESpecialElimType ElimTypeBitmask);
	
	void ProcessNextSpecialElim();
	void ShowSpecialElim(const FSpecialElimInfo& ElimMessageInfo);
	TQueue<FSpecialElimInfo> SpecialElimQueue;
	bool bIsProcessingQueue;

	//Modify//
	// ---------------- GAS 初始化标记（仅服务器有意义，不复制）----------------
	// 放在 PlayerState 而不是 Character 上：Character 每次重生都会重建，
	// 标记放它身上会导致重生后重复 GiveAbility（能力会叠成两份）。
	bool bStartupAbilitiesGranted;
	bool bAttributesInitialized;
	//Modify//
};
