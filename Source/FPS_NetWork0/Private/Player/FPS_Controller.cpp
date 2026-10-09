
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
#include "Components/InputComponent.h"   
#include "Kismet/KismetSystemLibrary.h"  // UKismetSystemLibrary::QuitGame
#include "Player/FPSPlayerState.h"       

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

#pragma region UI

//确保Widget的ScoreWidget可以获得PS的数据
void AFPS_Controller::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnPlayerStateReplicated.Broadcast();
}

void AFPS_Controller::Client_ShowMatchMessage_Implementation(const FString& Message)
{
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
	// SetPlayerName 内部会替服务端补一次 OnRep_PlayerName（PlayerState.cpp:280-285）
	// 所以宿主自己的 UI 会立刻刷新；其余客户端走正常的属性复制。
	PlayerState->SetPlayerName(InName.Left(20));
}

#pragma endregion UI

void AFPS_Controller::Server_RequestStartMatch_Implementation()
{
	if (AFPSGameMode* FPSGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AFPSGameMode>() : nullptr)
	{
		FPSGameMode->RequestStartMatch(this);
	}
}

//在Actor彻底析构前保存数据
void AFPS_Controller::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		UWorld* World = GetWorld();
		if (UFPSGameInstance* FPSGameInstance = World ? Cast<UFPSGameInstance>(World->GetGameInstance()) : nullptr)
		{
			FPSGameInstance->SaveProfileNow();
		}
	}
	Super::EndPlay(EndPlayReason);
}

//房主解散全部解散
void AFPS_Controller::RequestLeaveMatch(EFPSLeaveMatchMode Mode)
{
	if (!IsLocalController())
	{
		return;
	}
	// 1) 离开前补推一次：补上"最后一次击杀的数据还在路上"的窗口期。这里能安全读 PlayerState —— 此刻世界还完好，PS 还在。
	if (AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>())
	{
		FPSPlayerState->PushProgressToLocalProfile();
	}

	// 2) 落盘保存。必须在跳转前做：ClientTravel / ServerTravel 会拆掉当前世界，之后再去拿 GameInstance 就不稳了。
	UWorld* World = GetWorld();
	if (UFPSGameInstance* FPSGameInstance = World ? Cast<UFPSGameInstance>(World->GetGameInstance()) : nullptr)
	{
		FPSGameInstance->SaveProfileNow();
	}

	// 3) 去哪儿。
	switch (Mode)
	{
	case EFPSLeaveMatchMode::ReturnToLobby:
		if (GetLocalRole() == ROLE_Authority)//房主解散全部解散
		{
			// listen server 宿主：走 ServerTravel。不带 "?listen"，
			// 所以不会把其余客户端一起带到 Lobby —— 他们会被断开，
			// 各自靠自己的 AFPS_Controller::EndPlay 兜底写盘。
			if (World)
			{
				World->ServerTravel(TEXT("/Game/Maps/Lobby"));
			}
		}
		else
		{
			ClientTravel(TEXT("/Game/Maps/Lobby"), ETravelType::TRAVEL_Absolute);
		}
		break;

	case EFPSLeaveMatchMode::QuitGame:
		// bIgnorePlatformRestrictions = false：尊重平台限制（主机上不允许退出时就是 no-op）。
		UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, /*bIgnorePlatformRestrictions=*/false);
		break;
	}
}


#pragma region Input

void AFPS_Controller::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EnhancedInputComponent=Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&AFPS_Controller::Input_Look);
	EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AFPS_Controller::Input_Move);
	EnhancedInputComponent->BindAction(CrouchAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_Crouch);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_Jump);
	// 每个客户端都会触发，服务端只认房主 —— 校验在 AFPSGameMode::RequestStartMatch 里。
	EnhancedInputComponent->BindAction(StartGameAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_RequestStartMatch);
	// Esc 打开暂停菜单。C++ 只负责把按键转成事件，菜单本体在蓝图里做。
	// （1）【蓝图侧必须用 FInputModeGameAndUI，不能用 FInputModeUIOnly】：
	// FInputModeUIOnly::ApplyInputMode 内部会调 GameViewportClient.SetIgnoreInput(true)，
	// 而 UGameViewportClient::InputKey 在 IgnoreInput() 时直接 return —— 那样这条 BindKey
	// 永远收不到键。要锁移动/视角就单独调 SetIgnoreMoveInput / SetIgnoreLookInput。
	//
	// （2）本方案不调 SetPause（listen server 上 SetPause 会冻结所有客户端，还会停掉
	// AGameMode::Tick，而判胜正是靠 Tick 轮询 ReadyToEndMatch 的），加着只为防御将来的改动。
	EnhancedInputComponent->BindAction(QuitGameAction,ETriggerEvent::Started,this,&AFPS_Controller::Input_Pause);
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

// UI 全交给蓝图。C++ 只把事件抛出去。
void AFPS_Controller::Input_Pause()
{
	OnPauseRequested();
}

void AFPS_Controller::Input_RequestStartMatch()
{
	Server_RequestStartMatch();
}

#pragma endregion Input 