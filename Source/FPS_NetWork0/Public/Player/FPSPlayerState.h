
#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "FPSPlayerState.generated.h"

struct FSpecialElimInfo;
class USpecialElim;
class USpecialElimData;
enum class ESpecialElimType : uint16;
class AFPSCharacter;
class UAbilitySystemComponent;
class UFPSAbilitySystemComponent;
class UFPSAttributeSet;
class UGameplayAbility;
class UGameplayEffect;

//For Notifying UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchEnded, bool, bWon);

// 实现 IAbilitySystemInterface：ASC 与 AttributeSet 挂在 PlayerState 上，而不是 Character 上。
// 挂在 PlayerState 上则跨死亡、跨重生保留 —— 代价是重生时必须显式把属性和能力重置，
UCLASS()
class FPS_NETWORK0_API AFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AFPSPlayerState();
	
#pragma region GAS
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
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
	
	//服务器专用、幂等：只在第一次调用时授予角色能力并初始化属性。
	void EnsureStartupState(AFPSCharacter* Character);

	//服务器专用：施加 ResetAttributesEffect，把属性和回满值。重生流程调用。 
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	void ApplyResetAttributesEffect();
	
#pragma endregion GAS
	
#pragma region BaseVariable
	
	void AddScoredElim();
	int32 GetScoredElims() const;
	//被击败次数
	void AddDefeat();
	int32 GetDefeats() const { return Defeats; }
	void AddHit();
	void AddMiss();
	void AddHeadShotElim();
	void AddSequentialElim(int32 SequenceCount);
	void UpdateHighestStreak(int32 StreakCount);
	int32 GetHighestStreak() const { return HighestStreak; }
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
	bool IsWinner() const { return bWinner; }
	
#pragma endregion BaseVariable
	
#pragma region UI
	
	UPROPERTY(BlueprintAssignable)
	FScoreChanged OnScoreChanged;
	/** 结算 UI 的钩子。要在蓝图里接就绑这个；LeaderBoard 走的是轮询。 */
	UPROPERTY(BlueprintAssignable)
	FMatchEnded OnMatchEnded;
	
	//FromServer to Client for showing ElimUI
	UFUNCTION(Client, Reliable)
	void Client_LostTheLead();
	
	UFUNCTION(Client, Reliable)
	void Client_ScoredElim(int32 ElimScore);
	
	UFUNCTION(Client, Reliable)
	void Client_SpecialElim(const ESpecialElimType& SpecialElim, int32 SequentialElimCount, int32 StreakCount, int32 ElimScore);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|SpecialElims")
	TObjectPtr<USpecialElimData> SpecialElimData;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|SpecialElims")
	TSubclassOf<USpecialElim> SpecialElimWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|SpecialElims")
	float ElimDisplayTime;
	
#pragma endregion UI
	
#pragma region Profile
	
	/**
	 * 把本局当前战绩推进 GameInstance 内存里的档案（不写盘，见 UFPSGameInstance::UpdateProfileProgress）。
	 *
	 *	// {Tips}：理解底层。。。
	 * 只有"本机玩家自己的那个 PlayerState"才真正干活，其余情况（专用服务器、以及别人机器上
	 * 那个玩家的 PlayerState）直接 return。判据用 GameInstance 的 GetFirstLocalPlayerController，
	 * 而不是 GetPlayerController()->IsLocalController()：两者等价，但前者不依赖 Owner 属性
	 * 的复制到达时机（PS 自己属性的 OnRep 与 PC 的 PlayerState 属性走的是不同 channel）。
	 *
	 * 调用点有三处：
	 *   - AddScoredElim / UpdateHighestStreak（服务端路径）—— listen server 宿主是自己改属性的，
	 *     OnRep 永远不会替他触发，必须显式推；
	 *   - OnRep_ScoredElims / OnRep_HighestStreak（客户端路径）；
	 *   - AFPS_Controller::RequestLeaveMatch（离开前补一次）。
	 */
	void PushProgressToLocalProfile();

	//ScoredElims 是无条件复制（LeaderBoard 要读别人的击杀数），所以每个客户端上每个PlayerState的OnRep都会响——过滤靠 PushProgressToLocalProfile 里的守卫。 
	UFUNCTION()
	void OnRep_ScoredElims();

	//COND_OwnerOnly，只有本机玩家自己的客户端会收到，所以这个OnRep只在自己机器上响
	UFUNCTION()
	void OnRep_HighestStreak();
	
	//本局结束通知。AFPSGameMode 判胜后对每个 PlayerState 调一次。 
	//-----SaveGameData-----
	UFUNCTION(Client, Reliable)
	void Client_MatchEnded(bool bWon);

#pragma endregion Profile
	
private:
	
#pragma region BaseVariable
	
	UPROPERTY(ReplicatedUsing = OnRep_ScoredElims)
	int32 ScoredElims;
	// 只有本机玩家自己需要（写盘时读），所以是 COND_OwnerOnly 复制。
	UPROPERTY(Replicated)
	int32 Defeats;
	UPROPERTY(ReplicatedUsing = OnRep_HighestStreak)
	int32 HighestStreak;
	
	bool bWinner;
	int32 Hits;
	int32 Misses;
	bool bOnStreak; 
	int32 HeadShotElims;
	TMap<int32, int32> SequentialElims; 
	int32 RevengeElims;
	int32 DethroneElims;
	int32 ShowStopperElims;
	bool bFirstBlood;
	TWeakObjectPtr<APlayerState> LastAttacker;
	
#pragma endregion BaseVariable
	
	//-----对外部数据进行接收、解析，用于public函数使用-----
	TArray<ESpecialElimType> DecodeElimBitmask(ESpecialElimType ElimTypeBitmask);
	TQueue<FSpecialElimInfo> SpecialElimQueue;
	bool bIsProcessingQueue;
	void ProcessNextSpecialElim();
	void ShowSpecialElim(const FSpecialElimInfo& ElimMessageInfo);
	
	// -----GAS 初始化标记（仅服务器有意义，不复制）-----
	// 放在 PlayerState 而不是 Character 上：Character 每次重生都会重建，标记放它身上会导致重生后重复 GiveAbility（能力会叠成两份）。
	bool bStartupAbilitiesGranted;
	bool bAttributesInitialized;
};
