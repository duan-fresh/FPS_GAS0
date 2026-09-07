
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FPSGameState.generated.h"

class AFPSPlayerState;

UCLASS()
class FPS_NETWORK0_API AFPSGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	AFPSGameState();
	
	bool HasFirstBloodBeenHad() const;
	void UpdateLeader();
	
	AFPSPlayerState* GetSoleLeader() const;
	bool IsTiedForTheLead(AFPSPlayerState* PlayerState);
private:
	
	bool bHasFirstBloodBeenHad;
	UPROPERTY()
	TArray<TObjectPtr<AFPSPlayerState>> Leaders;
};
