

#include "Combat/EliminationComponent.h"

#include "Engine/World.h"
#include "Game/FPSGameState.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Player/FPSPlayerState.h"
#include "ShooterTypes/ShooterTypes.h"


UEliminationComponent::UEliminationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SequentialElimInterval = 2.f;
	LastElimTime = 0.f;
	SequentialElims = 0;
	Streak = 0;
	ElimsNeededForStreak = 5;
}

void UEliminationComponent::OnRoundReported(AActor* Attacker, AActor* Victim, bool bHit, bool bHeadShot, bool bLethal)
{
	AFPSPlayerState* AttackerPS = GetPlayerStateFromActor(Attacker);
	if (!IsValid(AttackerPS)) return;
	
	ProcessHitOrMiss(bHit, AttackerPS);
	
	if (!bHit) return; 
	
	AFPSPlayerState* VictimPS = GetPlayerStateFromActor(Victim);
	if (!IsValid(VictimPS)) return;
	
	if (bLethal)
	{
		ProcessElimination(bHeadShot, AttackerPS, VictimPS);
	}
}

void UEliminationComponent::ProcessHitOrMiss(bool bHit, AFPSPlayerState* AttackerPS)
{
	if (bHit)
	{
		AttackerPS->AddHit();
	}
	else
	{
		AttackerPS->AddMiss();
	}
}

void UEliminationComponent::ProcessElimination(bool bHeadShot, AFPSPlayerState* AttackerPS, AFPSPlayerState* VictimPS)
{
	AttackerPS->AddScoredElim();
	VictimPS->AddDefeat();

	ESpecialElimType SpecialElimType{};
	ProcessHeadshot(bHeadShot,SpecialElimType,AttackerPS);
	ProcessSequentialEliminations(SpecialElimType, AttackerPS);
	ProcessStreaks(SpecialElimType, AttackerPS, VictimPS);
	
	AFPSGameState* GameState = Cast<AFPSGameState>(UGameplayStatics::GetGameState(AttackerPS));
	if (IsValid(GameState))
	{
		HandleFirstBlood(GameState, SpecialElimType, AttackerPS);
		UpdateLeaderStatus(GameState, SpecialElimType, AttackerPS, VictimPS);
	}
	if (HasSpecialElimTypes(SpecialElimType))
	{
		AttackerPS->Client_SpecialElim(SpecialElimType, SequentialElims, Streak, AttackerPS->GetScoredElims());
	}
	else
	{
		AttackerPS->Client_ScoredElim(AttackerPS->GetScoredElims());
	}
}

void UEliminationComponent::ProcessHeadshot(bool bHeadShot, ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS)
{
	if (bHeadShot)
	{
		OutElimType |= ESpecialElimType::Headshot;
		AttackerPS->AddHeadShotElim();
	}
}

void UEliminationComponent::ProcessSequentialEliminations(ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS)
{
	const float CurrentTime  = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastElimTime <= SequentialElimInterval)
	{
		++SequentialElims;
	}
	else
	{
		SequentialElims = 1;
	}
	LastElimTime = CurrentTime;
	
	if (SequentialElims > 1)
	{
		OutElimType |= ESpecialElimType::Sequential;
		AttackerPS->AddSequentialElim(SequentialElims);
	}
}

void UEliminationComponent::ProcessStreaks(ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS,
	AFPSPlayerState* VictimPS)
{
	++Streak;
	// 每次击杀都要记最高连杀。原来这句在下面的 if 里，导致 1~4 连杀永远不会被记录，
	// 而"历史最高连杀"是要写进存档的 —— 一个 3 连杀的玩家存档里会一直是 0。
	AttackerPS->UpdateHighestStreak(Streak);
	if (Streak >= ElimsNeededForStreak)
	{
		OutElimType |= ESpecialElimType::Streak;
		AttackerPS->SetOnStreak(true);
	}
	if (VictimPS->IsOnStreak())
	{
		OutElimType |= ESpecialElimType::Showstopper;
		AttackerPS->AddShowStopperElim();
		VictimPS->SetOnStreak(false);
	}
	if (AttackerPS->GetLastAttacker() == VictimPS)
	{
		OutElimType |= ESpecialElimType::Revenge;
		AttackerPS->AddRevengeElim();
		AttackerPS->SetLastAttacker(nullptr);
	}
	VictimPS->SetLastAttacker(AttackerPS);
}

void UEliminationComponent::HandleFirstBlood(AFPSGameState* GameState, ESpecialElimType& OutElimType, AFPSPlayerState* AttackerPS)
{
	if (!GameState->HasFirstBloodBeenHad())
	{
		OutElimType |= ESpecialElimType::FirstBlood;
		AttackerPS->GotFirstBlood();
		// 首杀标记在这里落位。原来这句写在 AFPSGameState::UpdateLeader() 的末尾，
		// 属于职责错位 —— 一旦有人在开局时调 UpdateLeader() 做初始化，首杀就永远发不出来。
		GameState->MarkFirstBloodAsHad();
	}
}

void UEliminationComponent::UpdateLeaderStatus(AFPSGameState* GameState, ESpecialElimType& OutElimType,
	AFPSPlayerState* AttackerPS, AFPSPlayerState* VictimPS)
{
	AFPSPlayerState* LastLeader =  GameState->GetSoleLeader();
	const bool bAttackerWasTiedForTheLead = GameState->IsTiedForTheLead(AttackerPS);
	GameState->UpdateLeader();
	if (!bAttackerWasTiedForTheLead && GameState->IsTiedForTheLead(AttackerPS))
	{
		OutElimType |= ESpecialElimType::TiedTheLeader;
	}
	AFPSPlayerState* A=GameState->GetSoleLeader();
	if (IsValid(LastLeader) && LastLeader !=A)
	{
		LastLeader->Client_LostTheLead();
		if (VictimPS == LastLeader)
		{
			OutElimType |= ESpecialElimType::Dethrone;
			AttackerPS->AddDethroneElim();
		}
	}
	
	if (AttackerPS != LastLeader && AttackerPS == GameState->GetSoleLeader())
	{
		OutElimType |= ESpecialElimType::GainedTheLead;
	}
}

bool UEliminationComponent::HasSpecialElimTypes(const ESpecialElimType& SpecialElimType) const
{
	return static_cast<uint16>(SpecialElimType) != 0;
}

AFPSPlayerState* UEliminationComponent::GetPlayerStateFromActor(AActor* Actor)
{
	APawn* Pawn = Cast<APawn>(Actor);
	if (IsValid(Pawn))
	{
		return Pawn->GetPlayerState<AFPSPlayerState>();
	}
	return nullptr;
}





