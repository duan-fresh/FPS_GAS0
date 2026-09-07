
#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "GameFramework/PlayerState.h"
#include "FPSPlayerState.generated.h"

struct FSpecialElimInfo;
class USpecialElim;
class USpecialElimData;
enum class ESpecialElimType : uint16;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScoreChanged, int32, NewScore);

UCLASS()
class FPS_NETWORK0_API AFPSPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AFPSPlayerState();
	
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
};
