
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPS_Controller.generated.h"


class UInputMappingContext;
struct FInputActionValue;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerStateReplicated);

UCLASS()
class FPS_NETWORK0_API AFPS_Controller : public APlayerController
{
	GENERATED_BODY()
public:
	AFPS_Controller();
	
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	UPROPERTY(BlueprintAssignable)
	FPlayerStateReplicated OnPlayerStateReplicated;
	
	virtual void OnRep_PlayerState() override;
	
	//房主按下"开始对局"键 → 服务端。 
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "FPS|Match")
	void Server_RequestStartMatch();
	
	UFUNCTION(Client, Reliable)
	void Client_ShowMatchMessage(const FString& Message);

	/**
	 * 为什么挂在 PlayerController 上：
	 *   1. Server RPC 只能由"有 Owner 的 Actor"发出 —— GameMode / GameState 都发不了；
	 *   2. 判据 IsLocalController() 正好就是"这台机器上本机玩家自己的那个 PC"：
	 */
	UFUNCTION(Server, Reliable)
	void Server_SetPlayerName(const FString& InName);

	bool bPawnAlive;

private:
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputMappingContext> FPSIMC;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	void Input_Look(const FInputActionValue& Look);
	void Input_Crouch();
	void Input_Jump();
	void Input_Move(const FInputActionValue& Move);
	void Input_RequestStartMatch();
};
