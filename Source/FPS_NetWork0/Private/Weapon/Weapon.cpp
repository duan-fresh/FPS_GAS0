// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"


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
	if (!IsValid(WeaponOwner)&&!WeaponOwner->Implements<UPlayerInterface>()) return ;
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


