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

//Modify//
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/FPSAttributeSet.h"
#include "GameplayEffect.h"
#include "Player/FPSPlayerState.h"
#include "Tags/ShooterGameplayTags.h"
//Modify//


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

		//Modify//
		// 打上 State.Dead 标签（LooseGameplayTag，不占 GE 槽位）：
		//   - UFPSDamageOverTimeEffect 的 ApplicationTagRequirements 靠它挡住"对尸体挂 DoT"；
		//   - 同一 GE 的 OngoingTagRequirements 靠它让"目标死亡瞬间停止跳数"，
		//     所以不需要额外写一段"遍历并移除所有 DoT"的代码。
		// 重生时在 InitializeAbilitySystem() 里移除。
		if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent())
		{
			AbilitySystemComponent->AddLooseGameplayTag(FPSTags::TAG_State_Dead.GetTag());
		}
		//Modify//
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

	//Modify//
	// 烟雾弹：沿用工程现有的"输入绑在角色上"的风格（Q10 的既定方案），
	// 不用参考工程那套"Controller + TryActivateAbilitiesByTag"的做法，避免两种风格并存。
	EnhancedInputComponent->BindAction(IA_Smoke,ETriggerEvent::Started,this,&AFPSCharacter::Input_Smoke);
	//Modify//
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

	//Modify//
	// 服务器侧初始化：Owner = PlayerState，Avatar = 本角色。
	// 客户端侧走下面的 OnPlayerStateChanged（PlayerState 复制到客户端时才会触发）。
	InitializeAbilitySystem();
	//Modify//
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

	//Modify//
	// 客户端侧初始化。服务器上本函数也会被调到，重复执行无害：
	//   InitAbilityActorInfo 是幂等的；EnsureStartupState 内部有 HasAuthority + 一次性标记。
	InitializeAbilitySystem();
	//Modify//
}

//Modify//
UAbilitySystemComponent* AFPSCharacter::GetAbilitySystemComponent() const
{
	// ASC 挂在 PlayerState 上（跨死亡重生保留），这里只做转发。
	if (const AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>())
	{
		return FPSPlayerState->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AFPSCharacter::InitializeAbilitySystem()
{
	AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>();
	if (!IsValid(FPSPlayerState))
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = FPSPlayerState->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	// Owner = PlayerState（ASC 真正的拥有者），Avatar = 本角色（表现与移动的载体）。
	// 服务器与客户端都必须调用，否则客户端的能力/属性拿不到 Avatar，什么都不生效。
	AbilitySystemComponent->InitAbilityActorInfo(FPSPlayerState, this);

	if (HasAuthority())
	{
		// 授予启动能力 + 初始化属性。两个动作各只做一次，
		// 标记存在 PlayerState 上（跨重生保留），避免重生后能力被重复授予。
		FPSPlayerState->EnsureStartupState(this);

		// 重生出来的新角色不应该带着上一局的死亡标签。
		AbilitySystemComponent->RemoveLooseGameplayTag(FPSTags::TAG_State_Dead.GetTag());
	}

	// 把 GAS 的当前属性同步给本角色的 UHealthComponent。
	// 每次换角色（重生）都要做一次，否则新血条会停在 UHealthComponent 构造函数的默认值上。
	if (const UFPSAttributeSet* AttributeSet = FPSPlayerState->GetFPSAttributeSet())
	{
		if (IsValid(Health))
		{
			Health->SetFromGAS(
				AttributeSet->GetHealth(),
				AttributeSet->GetMaxHealth(),
				AttributeSet->GetShield(),
				AttributeSet->GetMaxShield());
		}
	}
}

void AFPSCharacter::Input_Smoke()
{
	// 按键 -> 按标签激活能力。能力本身是 ServerOnly，
	// 客户端这一次调用会被 GAS 转成 ServerTryActivateAbility RPC，由服务器执行。
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent())
	{
		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(FPSTags::TAG_Ability_Smoke.GetTag());
		AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
	}
}
//Modify//

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
	/*
	//original code
	if (!IsValid(Health)) return false;

	if (Health->ChangeHealthByAmount(-DamageAmount, DamageInstigator))
	{
		return true;
	}

	const int32 MontageSelection = FMath::RandRange(0, HitReacts.Num() - 1);
	Multicast_HitReact(MontageSelection);
	return false;
	*/
	//Modify//
	// 改造后血量不再直接改 UHealthComponent，而是走统一的 GAS 伤害通路：
	//   ApplyGameplayEffectSpecToSelf(UFPSDamageEffect, SetByCaller Data.Damage.Bullet = -DamageAmount)
	//     -> UFPSAttributeSet::PostGameplayEffectExecute
	//     -> 先扣护盾、溢出部分才扣血
	//     -> UHealthComponent::SetFromGAS 广播原有的 OnHealthChanged
	//
	// 接口签名与调用方（UCombatComponent::Sever_Fire）完全不变，
	// 返回值语义也不变：true = 这一击致命。
	// 注意：本函数在服务器上执行（Sever_Fire 是 Server RPC），属性改动是权威的。
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();

	if (!IsValid(AbilitySystemComponent) || !IsValid(DamageEffectClass))
	{
		// 兜底：GAS 还没接好（例如 DamageEffectClass 忘配）时退回旧逻辑，
		// 否则会出现"能开枪但完全打不掉血"这种最难排查的状态。
		if (!IsValid(Health)) return false;

		if (Health->ChangeHealthByAmount(-DamageAmount, DamageInstigator))
		{
			return true;
		}

		if (HitReacts.Num() > 0)
		{
			const int32 MontageSelection = FMath::RandRange(0, HitReacts.Num() - 1);
			Multicast_HitReact(MontageSelection);
		}
		return false;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	// 记录伤害来源，供以后的击杀提示 / 伤害来源显示取用。
	EffectContext.AddInstigator(DamageInstigator, DamageInstigator);

	FGameplayEffectSpecHandle DamageSpec =
		AbilitySystemComponent->MakeOutgoingSpec(DamageEffectClass, 1.f, EffectContext);

	if (!DamageSpec.IsValid())
	{
		return false;
	}

	// 约定：SetByCaller 一律传【负值】（沿用参考工程 GASCrashCourse 的做法）。
	DamageSpec = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(
		DamageSpec, FPSTags::TAG_Data_Damage_Bullet.GetTag(), -DamageAmount);

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data.Get());

	// 结算后立刻读结果：Instant GE 是同步执行的，此刻属性已经是最终值。
	const UFPSAttributeSet* AttributeSet = nullptr;
	if (const AFPSPlayerState* FPSPlayerState = GetPlayerState<AFPSPlayerState>())
	{
		AttributeSet = FPSPlayerState->GetFPSAttributeSet();
	}
	const bool bLethal = IsValid(AttributeSet) && AttributeSet->GetHealth() <= 0.f;

	// 与改造前一致：只有没打死才播受击蒙太奇（打死了走死亡流程的 DeathEffects）。
	if (!bLethal && HitReacts.Num() > 0)
	{
		const int32 MontageSelection = FMath::RandRange(0, HitReacts.Num() - 1);
		Multicast_HitReact(MontageSelection);
	}

	return bLethal;
	//Modify//
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







