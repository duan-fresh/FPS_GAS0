// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FPS_Controller.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AFPS_Controller::AFPS_Controller()
{
	bReplicates = true;
	bPawnAlive = true;
}

void AFPS_Controller::BeginPlay()
{
	Super::BeginPlay();
	UEnhancedInputLocalPlayerSubsystem*Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(Subsystem)) return;
	Subsystem->AddMappingContext(FPSIMC,0);
}

void AFPS_Controller::SetupInputComponent()
{
	Super::SetupInputComponent();
	UEnhancedInputComponent* EnhancedInputComponent=Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&AFPS_Controller::Input_Look);
	EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AFPS_Controller::Input_Move);
	EnhancedInputComponent->BindAction(CrouchAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_Crouch);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_Jump);
}

void AFPS_Controller::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnPlayerStateReplicated.Broadcast();
}

void AFPS_Controller::Input_Look(const FInputActionValue& Look)
{
	
	if (!bPawnAlive) return;
	const FVector2D LookValue=Look.Get<FVector2D>();
	AddYawInput(LookValue.X);
	AddPitchInput(LookValue.Y);
}

void AFPS_Controller::Input_Crouch()
{
	if (!IsValid(GetCharacter())) return;
	if (!bPawnAlive) return;
	if (UCharacterMovementComponent* CMC=GetCharacter()->GetCharacterMovement();IsValid(CMC))
	{
		CMC->bWantsToCrouch=!CMC->bWantsToCrouch;
	}
}

void AFPS_Controller::Input_Jump()
{
	if (!IsValid(GetCharacter())) return;
	if (!bPawnAlive) return;
	UCharacterMovementComponent* CMC=GetCharacter()->GetCharacterMovement();
	if (!IsValid(CMC)) return;
	if (CMC->bWantsToCrouch)
	{
		CMC->bWantsToCrouch=false;
	}
	else
	{
		GetCharacter()->Jump();
	}
}

void AFPS_Controller::Input_Move(const FInputActionValue& Move)
{
	if (!bPawnAlive) return;
	const FVector2D MoveValue=Move.Get<FVector2D>();
	const FRotator CurRotation=GetControlRotation();
	const FRotator SwitchRotation(0.f,CurRotation.Yaw,0.f);
	const FVector ForwardVector=FRotationMatrix(SwitchRotation).GetUnitAxis(EAxis::X);
	const FVector RightVector=FRotationMatrix(SwitchRotation).GetUnitAxis(EAxis::Y);
	if (APawn* ControlledPawn=GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardVector,MoveValue.X);
		ControlledPawn->AddMovementInput(RightVector,MoveValue.Y);
	}
}
