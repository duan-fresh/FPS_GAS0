// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "FPS_NetWork0/FPS_NetWork0.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"
#include "Kismet/KismetMathLibrary.h"


AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	
	Mesh1P=CreateDefaultSubobject<USkeletalMeshComponent>("Mesh1P");
	Mesh1P->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh1P->bReceivesDecals = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetHiddenInGame(true);
	SetRootComponent(Mesh1P);
	
	Mesh3P=CreateDefaultSubobject<USkeletalMeshComponent>("Mesh3P");
	Mesh3P->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh3P->bReceivesDecals = false;
	Mesh3P->CastShadow = true;
	Mesh3P->SetHiddenInGame(true);
	Mesh3P->SetupAttachment(Mesh1P);
	
	TraceRadius=5.0f;
	FireTime=0.1f;
}

void AWeapon::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	SetupAttachment();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void AWeapon::SetupAttachment()
{
	ACharacter* WeaponOwner = Cast<ACharacter>(GetOwner());
	if (!IsValid(WeaponOwner)||!WeaponOwner->Implements<UPlayerInterface>()) return;
	SetupVisibility(WeaponOwner);
	const FName GripPoint=IPlayerInterface::Execute_GetWeaponGripPoint(WeaponOwner,WeaponType);
	USkeletalMeshComponent* OwnerMesh1P=IPlayerInterface::Execute_GetMesh1P(WeaponOwner);
	USkeletalMeshComponent* OwnerMesh3P=IPlayerInterface::Execute_GetMesh3P(WeaponOwner);
	Mesh1P->AttachToComponent(OwnerMesh1P,FAttachmentTransformRules::KeepRelativeTransform,GripPoint);
	Mesh3P->AttachToComponent(OwnerMesh3P,FAttachmentTransformRules::KeepRelativeTransform,GripPoint);
}

void AWeapon::SetupVisibility(const APawn* OwningPawn) const
{
	if (OwningPawn->IsLocallyControlled())
	{
		Mesh1P->SetHiddenInGame(false);
		Mesh3P->SetHiddenInGame(true);
	}
	else
	{
		Mesh1P->SetHiddenInGame(true);
		Mesh3P->SetHiddenInGame(false);
	}
}

USkeletalMeshComponent* AWeapon::GetMesh3P()
{
	return Mesh3P;
}

USkeletalMeshComponent* AWeapon::GetMesh1P()
{
	return Mesh1P;
}

void AWeapon::FireTrace(FHitResult& Hit,const float FireRange)
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.bReturnPhysicalMaterial=true;
	
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn,ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic,ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_WorldDynamic,ECR_Block);
	ResponseParams.CollisionResponse.SetResponse(ECC_PhysicsBody,ECR_Block);
	ensure(GetInstigator());
	if (APlayerController* PC=Cast<APlayerController>(GetInstigator()->GetController());IsValid(PC))
	{
		FVector Start;
		FRotator EyeRotation;
		PC->GetActorEyesViewPoint(Start,EyeRotation);
		FVector EyeForwardRotation=UKismetMathLibrary::GetForwardVector(EyeRotation);
		FVector End=Start+EyeForwardRotation*FireRange;
		const bool bHit=GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			FPSTraceChannels::ECC_Weapon,
			FCollisionShape::MakeSphere(TraceRadius),
			CollisionParams,
			ResponseParams
			);
		if (!bHit)
		{
			Hit.ImpactPoint=End;
		}
	}
}

void AWeapon::Local_Fire(const FVector& ImpactPoint, const FVector& ImpactNormal,
	TEnumAsByte<EPhysicalSurface> ImpactSurfaceType, bool bIsFistPerson)
{
	FireEffects(ImpactPoint,ImpactNormal,ImpactSurfaceType,bIsFistPerson);
}





