// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/FPS_Controller.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/FPSGameInstance.h"
#include "Game/FPSGameMode.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "InputCoreTypes.h"

AFPS_Controller::AFPS_Controller()
{
	bReplicates = true;
	bPawnAlive = true;
}

void AFPS_Controller::BeginPlay()
{
	Super::BeginPlay();
	
	// 把本机玩家在菜单里输入的名字报给服务端。
	// （InitPlayerState 整段被 GetNetMode() != NM_Client 挡掉，Controller.cpp:654）。
	// 但这条 RPC 是发给服务端的，而服务端在 PC spawn 期间就已经建好 PlayerState
	// （PlayerController.cpp:1073-1077），并且早于 BeginPlay（Actor.cpp:4501 vs :4529）。
	if (IsLocalController())
	{
		UWorld* World = GetWorld();
		UFPSGameInstance* FPSGameInstance = World ? Cast<UFPSGameInstance>(World->GetGameInstance()) : nullptr;

		if (IsValid(FPSGameInstance))
		{
			const FString LocalPlayerName = FPSGameInstance->GetLocalPlayerName();
			if (!LocalPlayerName.IsEmpty())
			{
				GEngine->AddOnScreenDebugMessage(
				-1,              
				5.0f,            
				FColor::Yellow,  
				TEXT("Set name Debug!") 
			);
				Server_SetPlayerName(LocalPlayerName);
			}
		}
	}

	UEnhancedInputLocalPlayerSubsystem*Subsystem=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(Subsystem)) return;
	Subsystem->AddMappingContext(FPSIMC,0);
}

void AFPS_Controller::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// 房主"开始对局"键。用 BindKey 直连而不是 Enhanced Input，是为了不额外新增
	// UInputAction / IMC 资产。想改成 Enhanced Input 的话，在 BP_FPSController 里
	// 每个客户端都会触发，服务端只认房主 —— 校验在 AFPSGameMode::RequestStartMatch 里。
	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AFPS_Controller::Input_RequestStartMatch);

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

void AFPS_Controller::Input_RequestStartMatch()
{
	Server_RequestStartMatch();
}

void AFPS_Controller::Server_RequestStartMatch_Implementation()
{
	if (AFPSGameMode* FPSGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFPSGameMode>() : nullptr)
	{
		FPSGameMode->RequestStartMatch(this);
	}
}

void AFPS_Controller::Client_ShowMatchMessage_Implementation(const FString& Message)
{
	// 先用屏幕调试信息顶着。要做成正式 UI 的话，在这里创建对应的 widget 即可。
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
	}
}

void AFPS_Controller::Server_SetPlayerName_Implementation(const FString& InName)
{
	if (!IsValid(PlayerState) || InName.IsEmpty())
	{
		return;
	}
	// SetPlayerName 内部会替服务端补一次 OnRep_PlayerName（PlayerState.cpp:280-285），
	// 所以宿主自己的 UI 会立刻刷新；其余客户端走正常的属性复制。
	PlayerState->SetPlayerName(InName.Left(20));
}

