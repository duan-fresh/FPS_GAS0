// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Smoke/FPSmokeCube.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AFPSmokeCube::AFPSmokeCube()
{
	PrimaryActorTick.bCanEverTick = false;

	// 服务端生成，自动复制到所有客户端；位置靠移动复制同步。
	bReplicates = true;
	SetReplicateMovement(true);

	// 引擎自带的Cube与基础材质，保证即使蓝图里什么都不填也能看见东西。
	// 真正使用的材质请在 BP_SmokeCube里覆盖成M_Smoke。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (FallbackMaterialFinder.Succeeded())
	{
		SmokeMaterial = FallbackMaterialFinder.Object;
	}

	// ---------------- 网格 ----------------
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SmokeMesh"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetStaticMesh(CubeMesh);
	if (IsValid(SmokeMaterial))
	{
		MeshComponent->SetMaterial(0, SmokeMaterial);
	}

	// 碰撞策略：除了地面以外谁都不挡。
	//   - 对 WorldStatic  Block：让 ProjectileMovement 有一次落地判定
	//   - 对其它一切     Ignore：玩家可以走进去、子弹可以穿过去（Weapon 自定义通道也被这条覆盖）
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCastShadow(false);

	// 飞行途中的体积：小方块
	MeshComponent->SetRelativeScale3D(FVector(FlyingScale));

	// ---------------- 投射物运动 ----------------
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(MeshComponent);
	// 初速度由 AFPSmokeCube::Launch() 直接写 Velocity，这里保持 0。
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 0.f; // 0 = 不限制
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.f;
}

void AFPSmokeCube::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SetLifeSpan(LifeSeconds);
		ProjectileMovement->OnProjectileStop.AddDynamic(this, &AFPSmokeCube::OnProjectileStopped);
	}
	else
	{
		// 客户端不做本地模拟：位置完全跟随服务器的移动复制，
		// 否则两端各跑一套积分，落点会不一致。
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetComponentTickEnabled(false);
	}
}

void AFPSmokeCube::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 只复制"是否已落地"，够客户端切换视觉即可。
	DOREPLIFETIME(AFPSmokeCube, bDeployed);
}

//在蓝图调用的函数
void AFPSmokeCube::Launch(const FVector& InitialVelocity)
{
	if (!HasAuthority() || !IsValid(ProjectileMovement))
	{
		return;
	}
	// 把内部计算的Velocity 同步给 UpdatedComponent（物理速度 /GetVelocity()），它不是"标记移动复制"的开关
	//bReplicateMovement的复制是每帧自己打包FRepMovement，跟这个调用无关。
	ProjectileMovement->Velocity = InitialVelocity;
	ProjectileMovement->UpdateComponentVelocity();
}

void AFPSmokeCube::OnProjectileStopped(const FHitResult& ImpactResult)
{
	// 落地：立刻停住，避免静止后还被滑动/重力带着走。
	if (IsValid(ProjectileMovement))
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->SetComponentTickEnabled(false);
	}

	// 服务器自己先切一次视觉；bDeployed 复制到客户端后由 OnRep_Deployed 再切一次。
	bDeployed = true;
	ApplyDeployedVisuals();
	ExecuteLandingCue();
}

void AFPSmokeCube::OnRep_Deployed()
{
	if (bDeployed)
	{
		ApplyDeployedVisuals();
	}
}

void AFPSmokeCube::ApplyDeployedVisuals()
{
	//不放缩
	// if (IsValid(MeshComponent))
	// {
	// 	MeshComponent->SetRelativeScale3D(FVector(DeployedScale));
	// }

	// 专用服务器上没有渲染，物理/特效都没意义，直接跳过。
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnSmokeDeployed();
	}
}

void AFPSmokeCube::ExecuteLandingCue()
{
	if (!LandingCueTag.IsValid())
	{
		return;
	}
	// 用【投掷者的 ASC】而不是本 Actor 自己：
	//   - ASC 挂在 AFPSPlayerState 上，对所有客户端都"相关"，NetMulticast 能覆盖到每个人；
	//   - 若把 ASC 挂在投掷物上，投掷物一旦离开相关性范围，部分客户端就收不到 Cue。
	AActor* InstigatorActor = Cast<AActor>(GetInstigator());
	UAbilitySystemComponent* InstigatorASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (!IsValid(InstigatorASC))
	{
		return;
	}
	FGameplayCueParameters CueParameters;
	CueParameters.Location = GetActorLocation();
	CueParameters.Normal = FVector::UpVector;
	CueParameters.Instigator = InstigatorActor;
	CueParameters.EffectCauser = this;
	// 服务端调用：引擎内部走 NetMulticast_InvokeGameplayCueExecuted_WithParams，
	// 服务端自己与所有客户端都会执行一次。
	InstigatorASC->ExecuteGameplayCue(LandingCueTag, CueParameters);
}
//可以额外加GC_Smoke，使用Cue，或是实现蓝图函数，使用粒子效果
	