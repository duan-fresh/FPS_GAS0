#include "Character/FPSCharacter.h"

#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Combat/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/WeaponData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
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
}

void AFPSCharacter::Input_AimWeapon_Released()
{
	CombatComponent->Initiate_AimWeapon_Released();
}

void AFPSCharacter::Input_CycleWeapon()
{
	CombatComponent->Initiate_CycleWeapon();
}

void AFPSCharacter::Input_ReloadWeapon()
{
	CombatComponent->Initiate_ReloadWeapon();
}



