#include "Character/FPSCharacter.h"

#include "EnhancedInputComponent.h"
#include "VectorTypes.h"
#include "Camera/CameraComponent.h"
#include "Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/WeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnitConversion.h"
#include "ShooterTypes/ShooterTypes.h"
#include "Weapon/Weapon.h"


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
	
	DefaultFieldOfView=90.0f;
	bWeaponFirstReplicated=false;
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
}

void AFPSCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	if (IsValid(CombatComponent))
	{
		CombatComponent->InitializeWeaponWidget();
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
		OnWeaponFirstReplicated.Broadcast(CombatComponent->CurrentWeapon);
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





