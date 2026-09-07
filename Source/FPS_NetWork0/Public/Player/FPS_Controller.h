// Fill out your copyright notice in the Description page of Project Settings.

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
};
