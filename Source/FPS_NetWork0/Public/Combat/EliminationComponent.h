
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EliminationComponent.generated.h"

class AFPSGameState;
class AShooterGameStateBase;
enum class ESpecialElimType : uint16;
class AFPSPlayerState;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPS_NETWORK0_API UEliminationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEliminationComponent();

	UFUNCTION()
	void OnRoundReported(AActor* Attacker, AActor* Victim, bool bHit, bool bHeadShot, bool bLethal);
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Elimination")
	float SequentialElimInterval;
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Elimination")
	int32 ElimsNeededForStreak;
private:
	float LastElimTime;
	int32 SequentialElims;
	int32 Streak;
	
	AFPSPlayerState* GetPlayerStateFromActor(AActor* Actor);
	void ProcessHitOrMiss(bool bHit, AFPSPlayerState* AttackerPS);
	void ProcessElimination(bool bHeadShot, AFPSPlayerState* AttackerPS, AFPSPlayerState* VictimPS);
	void ProcessHeadshot(bool bHeadShot, ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS);
	void ProcessSequentialEliminations(ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS);
	void ProcessStreaks(ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS, AFPSPlayerState* VictimPS);
	void HandleFirstBlood(AFPSGameState* GameState, ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS);
	void UpdateLeaderStatus(AFPSGameState* GameState, ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS, AFPSPlayerState* VictimPS);
	bool HasSpecialElimTypes(const ESpecialElimType& SpecialElimType) const;
};
