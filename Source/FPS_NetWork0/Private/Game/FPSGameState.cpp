
#include "Game/FPSGameState.h"

#include "Player/FPSPlayerState.h"

AFPSGameState::AFPSGameState()
{
	bHasFirstBloodBeenHad = false;
}

bool AFPSGameState::HasFirstBloodBeenHad() const
{
	return bHasFirstBloodBeenHad;
}

void AFPSGameState::MarkFirstBloodAsHad()
{
	bHasFirstBloodBeenHad = true;
}

void AFPSGameState::UpdateLeader()
{
	TArray<APlayerState*> LocalSortedPlayers = PlayerArray;
	LocalSortedPlayers.Sort([](const APlayerState& A, const APlayerState& B)
	{
		const AFPSPlayerState* PlayerA = Cast<AFPSPlayerState>(&A);
		const AFPSPlayerState* PlayerB = Cast<AFPSPlayerState>(&B);
		return PlayerA->GetScoredElims() > PlayerB->GetScoredElims();
	});
	Leaders.Empty();
	if (LocalSortedPlayers.Num() > 0)
	{
		int32 HighestScore = 0;
		for (APlayerState* Player : LocalSortedPlayers)
		{
			AFPSPlayerState* ShooterPS = Cast<AFPSPlayerState>(Player);
			if (IsValid(ShooterPS))
			{
				int32 PlayerScore = ShooterPS->GetScoredElims();
				if (Leaders.Num() == 0)
				{
					HighestScore = PlayerScore;
					Leaders.Add(ShooterPS);
				}
				else if (PlayerScore == HighestScore)
				{
					Leaders.Add(ShooterPS);
				}
				else
				{
					break;
				}
			}
		}
	}
}

AFPSPlayerState* AFPSGameState::GetSoleLeader() const
{
	if (Leaders.Num() == 1)
	{
		return Leaders[0];
	}
	return nullptr;
}

bool AFPSGameState::IsTiedForTheLead(AFPSPlayerState* PlayerState)
{
	return Leaders.Contains(PlayerState)&& Leaders.Num() > 1;
}
