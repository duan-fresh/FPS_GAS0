#include "Character/FPSCharacter.h"

#include "EnhancedInputComponent.h"
#include "TimerManager.h"
#include "VectorTypes.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Combat/CombatComponent.h"
#include "Combat/EliminationComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/WeaponData.h"
#include "FPS_NetWork0/FPS_NetWork0.h"
#include "Game/FPSGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Health/HealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnitConversion.h"
#include "Player/FPS_Controller.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Weapon/Weapon.h"


class AFPSGameMode;

AFPSCharacter::AFPSCharacter()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	
	SpringArm1P=CreateDefaultSubobject<USpringArmComponent>(FName("SpringArm1P"));
	SpringArm1P->TargetArmLength=0.f;
	SpringArm1P->SetupAttachment(GetRootComponent());
	SpringArm1P->bUsePawnControlRotation=true;
	SpringArm1P->bEnableCameraLag=true;
	SpringArm1P->CameraLagSpeed=15.f;
	
	Camera1P = CreateDefaultSubobject<UCameraComponent>("Camera1P");
	Camera1P->SetupAttachment(SpringArm1P);
	Camera1P->bUsePawnControlRotation=false;
	
	Mesh1P=CreateDefaultSubobject<USkeletalMeshComponent>("Mesh1P");
	Mesh1P->SetupAttachment(Camera1P);
	Mesh1P->bOwnerNoSee=false;
	Mesh1P->bOnlyOwnerSee=true;
	Mesh1P->CastShadow=false;
	Mesh1P->bReceivesDecals=false;
	//仅当该网格体在屏幕上被渲染（可见）时，才更新其骨骼动画
	Mesh1P->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	//物理模拟后的位置可能与本帧动画位置不一致
	Mesh1P->PrimaryComponentTick.TickGroup=TG_PrePhysics;
	
	GetMesh()->bReceivesDecals=false;
	GetMesh()->bOwnerNoSee=true;
	GetMesh()->bOnlyOwnerSee=false;
	
	CombatComponent=CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	CombatComponent->SetIsReplicated(true);
	
	Health = CreateDefaultSubobject<UHealthComponent>("Health");
	Health->SetIsReplicated(true);
	Health->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);//在客户端与服务器的Actor都会执行死亡逻辑
	
	Elimination = CreateDefaultSubobject<UEliminationComponent>("Elimination");
	Elimination->SetIsReplicated(false);
	
	DefaultFieldOfView=90.0f;
	bWeaponFirstReplicated=false;
	RespawnTime = 3.f;
}

FRotator AFPSCharacter::GetFixedAimRotation() const
{
	FRotator AimRotation=GetBaseAimRotation();
	if (AimRotation.Pitch>90&&!IsLocallyControlled())//因为是在多人游戏传递中才会出现，只用在其他客户端修正即可
	{
		FVector2D InRange(270,360);
		FVector2D OutRange(-90,0);
		AimRotation.Pitch=FMath::GetMappedRangeValueClamped(InRange,OutRange,AimRotation.Pitch);
	}
	return AimRotation;
}

bool AFPSCharacter::HasCurrentWeapon() const
{
	return IsValid(CombatComponent)&&CombatComponent->CurrentWeapon!=nullptr;
}

bool AFPSCharacter::HasWeaponFirstReplicated() const
{
	return bWeaponFirstReplicated;
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	Camera1P->SetFieldOfView(DefaultFieldOfView);
	StartingAimRotation=FRotator(0.0f,GetBaseAimRotation().Pitch,0.0f);
	
	if (HasAuthority())//只有服务器才可以进行权威数据的管理，，？？Warning：错误要放在BeginPlay中
	{
		CombatComponent->OnRoundReported.AddUniqueDynamic(Elimination, &UEliminationComponent::OnRoundReported);
	}
}

void AFPSCharacter::BeginDestroy()
{
	Super::BeginDestroy();
	if (!IsValid(CombatComponent)) return ;
	CombatComponent->DestroyComponent();
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CalculateFABRIK_SocketTransform();
	CalculateTurnParameters(DeltaTime);
}

//死亡在客户端和服务器端都进行的函数
void AFPSCharacter::OnDeathStarted()
{
	if (HasAuthority())
	{
		CombatComponent->DestoryInventory();
		GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &ThisClass::DeathTimerFinished, RespawnTime);
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		DeathEffects();
		if (AFPS_Controller* PC = Cast<AFPS_Controller>(GetController()); IsValid(PC))
		{
			DisableInput(PC);
			if (PC->IsLocalController())
			{
				PC->bPawnAlive = false;
			}
		}
	}
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(FPSTraceChannels::ECC_Weapon, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(FPSTraceChannels::ECC_Weapon, ECR_Ignore);
}


void AFPSCharacter::DeathTimerFinished()
{
	AFPSGameMode* GM = Cast<AFPSGameMode>(UGameplayStatics::GetGameMode(this));
	if (IsValid(GM))
	{
		GM->RequestRespawn(this, GetController());
	}
}

void AFPSCharacter::CalculateFABRIK_SocketTransform()
{
	if (IsValid(CombatComponent)&&IsValid(CombatComponent->CurrentWeapon)&&IsValid(CombatComponent->CurrentWeapon->GetMesh3P()))
	{
		FABRIK_SocketTransform=CombatComponent->CurrentWeapon->GetMesh3P()->GetSocketTransform("FABRIK_Socket",RTS_World);
		FVector OutLocation;
		FRotator OutRotation;
		GetMesh()->TransformToBoneSpace(
			"hand_r",
			FABRIK_SocketTransform.GetLocation(),
			FABRIK_SocketTransform.GetRotation().Rotator(),
			OutLocation,
			OutRotation);
		FABRIK_SocketTransform.SetLocation(OutLocation);
		FABRIK_SocketTransform.SetRotation(OutRotation.Quaternion());
	}
}

void AFPSCharacter::CalculateTurnParameters(float DeltaTime)
{
	FVector NowSpeed=GetVelocity();
	NowSpeed.Z=0;
	float Speed=NowSpeed.Size();
	bool bIsAir=GetCharacterMovement()->IsFalling();
	
	if (Speed==0.0f&&!bIsAir)
	{
		FRotator CurrentAimRotation=FRotator(0.0f,GetBaseAimRotation().Yaw,0.0f);
		FRotator DeltaRotation=UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation,StartingAimRotation);
		AO_Yaw=DeltaRotation.Yaw;
		if (TurningState==ETurning::NotTurning)
		{
			InterpAO_Yaw=AO_Yaw;
		}
		TurnToMovement(DeltaTime);
	}
	if (Speed>0.0f||bIsAir)
	{
		StartingAimRotation=FRotator(0.0f,GetBaseAimRotation().Yaw,0.0f);
		AO_Yaw=0;
		FRotator AimRotation=GetBaseAimRotation();
		FRotator MovementRotation=UKismetMathLibrary::MakeRotFromX(GetVelocity());
		MovementOffsetYaw=UKismetMathLibrary::NormalizedDeltaRotator(AimRotation,MovementRotation).Yaw;
		TurningState=ETurning::NotTurning;
	}
	AO_Yaw*=-1.0f;
}

void AFPSCharacter::TurnToMovement(float DeltaTime)
{
	if (AO_Yaw>90.0f)
	{
		TurningState=ETurning::Right;
	}
	else if (AO_Yaw<-90.0f)
	{
		TurningState=ETurning::Left;
	}
	if (TurningState!=ETurning::NotTurning)
	{
		InterpAO_Yaw=FMath::FInterpTo(InterpAO_Yaw,0.0f,DeltaTime,4);
		AO_Yaw=InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw)<5.0f)
		{
			StartingAimRotation=FRotator(0.0f,GetBaseAimRotation().Yaw,0.0f);
			TurningState=ETurning::NotTurning;
			AO_Yaw=0;//Tips
		}
	}
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent * EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInputComponent->BindAction(IA_AimWeapon,ETriggerEvent::Started,this,&AFPSCharacter::Input_AimWeapon_Pressed);
	EnhancedInputComponent->BindAction(IA_AimWeapon,ETriggerEvent::Completed,this,&AFPSCharacter::Input_AimWeapon_Released);
	EnhancedInputComponent->BindAction(IA_FireWeapon,ETriggerEvent::Started,this,&AFPSCharacter::Input_FireWeapon_Pressed);
	EnhancedInputComponent->BindAction(IA_FireWeapon,ETriggerEvent::Completed,this,&AFPSCharacter::Input_FireWeapon_Released);
	EnhancedInputComponent->BindAction(IA_CycleWeapon,ETriggerEvent::Started,this,&AFPSCharacter::Input_CycleWeapon);
	EnhancedInputComponent->BindAction(IA_ReloadWeapon,ETriggerEvent::Started,this,&AFPSCharacter::Input_ReloadWeapon);
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (IsValid(CombatComponent))
	{
		CombatComponent->SpawnInventory();
	}
	if (AFPS_Controller* PC = Cast<AFPS_Controller>(NewController); IsValid(PC))
	{
		PC->bPawnAlive = true;
	}
}

void AFPSCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	if (IsValid(CombatComponent))
	{
		CombatComponent->InitializeWeaponWidget();
	}
	if (AFPS_Controller* PC = Cast<AFPS_Controller>(GetController()); IsValid(PC))
	{
		PC->bPawnAlive = true;
	}
}

FName AFPSCharacter::GetWeaponGripPoint_Implementation(const FGameplayTag& WeaponType) const
{
	checkf(CombatComponent->WeaponDataAsset,TEXT("Fill out WeaponDataAsset!"));
	return CombatComponent->WeaponDataAsset->GripPoints.FindChecked(WeaponType);
}

USkeletalMeshComponent* AFPSCharacter::GetMesh3P_Implementation() const
{
	return GetMesh();
}

USkeletalMeshComponent* AFPSCharacter::GetMesh1P_Implementation() const
{
	return Mesh1P;
}

void AFPSCharacter::WeaponReplicated_Implementation()
{
	if (!bWeaponFirstReplicated)
	{
		bWeaponFirstReplicated=true;
		OnWeaponFirstReplicated.Broadcast(CombatComponent->CurrentWeapon, CombatComponent->bHitPlayer);
	}
}

AWeapon* AFPSCharacter::GetCurrentWeapon_Implementation()
{
	return CombatComponent->CurrentWeapon;
}

int32 AFPSCharacter::GetReserveAmmo_Implementation() const
{
	return CombatComponent->CurrentReserveAmmo;
}

void AFPSCharacter::Notify_CycleWeapon_Implementation()
{
	CombatComponent->Notify_CycleWeapon();
}

void AFPSCharacter::Notify_ReloadWeapon_Implementation()
{
	CombatComponent->Notify_ReloadWeapon();
}

void AFPSCharacter::AddAmmo_Implementation(const FGameplayTag& WeaponType, int32 AmmoAmount)
{
	if (HasAuthority() && IsValid(CombatComponent))
	{
		CombatComponent->AddAmmo(WeaponType, AmmoAmount);
	}
}

bool AFPSCharacter::DoDamage_Implementation(float DamageAmount, AActor* DamageInstigator)
{
	if (!IsValid(Health)) return false;

	if (Health->ChangeHealthByAmount(-DamageAmount, DamageInstigator))
	{
		return true; 
	}
	
	const int32 MontageSelection = FMath::RandRange(0, HitReacts.Num() - 1);
	Multicast_HitReact(MontageSelection);
	return false;
}

void AFPSCharacter::Multicast_HitReact_Implementation(int32 MontageIndex)
{
	if (GetNetMode() != NM_DedicatedServer && !IsLocallyControlled())
	{
		if (HitReacts.IsValidIndex(MontageIndex))
		{
			GetMesh()->GetAnimInstance()->Montage_Play(HitReacts[MontageIndex]);
		}
	}
}

void AFPSCharacter::Input_FireWeapon_Pressed()
{
	CombatComponent->Initiate_FireWeapon_Pressed();
}

void AFPSCharacter::Input_FireWeapon_Released()
{
	CombatComponent->Initiate_FireWeapon_Released();
}

void AFPSCharacter::Input_AimWeapon_Pressed()
{
	CombatComponent->Initiate_AimWeapon_Pressed();
	OnAim(true);
}

void AFPSCharacter::Input_AimWeapon_Released()
{
	CombatComponent->Initiate_AimWeapon_Released();
	OnAim(false);
}

void AFPSCharacter::Input_CycleWeapon()
{
	CombatComponent->Initiate_CycleWeapon();
}

void AFPSCharacter::Input_ReloadWeapon()
{
	CombatComponent->Initiate_ReloadWeapon();
}







