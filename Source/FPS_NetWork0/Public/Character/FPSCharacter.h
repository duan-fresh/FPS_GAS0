#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"
#include "FPSCharacter.generated.h"

class UCombatComponent;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
enum class ETurning: uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWeaponFirstReplicated, AWeapon*,Weapon);

UCLASS()
class FPS_NETWORK0_API AFPSCharacter : public ACharacter,public IPlayerInterface
{
	GENERATED_BODY()
public:
	AFPSCharacter();
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;
	/*Interface*/
	virtual FName GetWeaponGripPoint_Implementation(const FGameplayTag& WeaponType) const override;
	virtual USkeletalMeshComponent* GetMesh3P_Implementation() const override;
	virtual USkeletalMeshComponent* GetMesh1P_Implementation() const override;
	virtual void WeaponReplicated_Implementation()override;
	/*Interface*/
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|Combat")
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|Camera")
	float DefaultFieldOfView;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|Camera")
	TObjectPtr<UCameraComponent> Camera1P;
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnAim(bool PressedAim);
	
	UFUNCTION(BlueprintCallable)
	FRotator GetFixedAimRotation() const;

	UPROPERTY(BlueprintReadOnly,Category="FPS|FABRIK")
	FTransform FABRIK_SocketTransform;
	
	UFUNCTION(BlueprintCallable)
	bool HasCurrentWeapon()const;
	
	UFUNCTION(BlueprintCallable)
	bool HasWeaponFirstReplicated() const; 
	
	UPROPERTY(BlueprintAssignable)
	FWeaponFirstReplicated OnWeaponFirstReplicated;
	
#pragma region TurnAim
	UPROPERTY(BlueprintReadOnly)
	FRotator StartingAimRotation;
	
	UPROPERTY(BlueprintReadOnly)
	float AO_Yaw;//相对于Movement的朝向Yaw
	
	UPROPERTY(BlueprintReadOnly)
	float InterpAO_Yaw;
	
	UPROPERTY(BlueprintReadOnly)
	float MovementOffsetYaw;
	
	UPROPERTY(BlueprintReadOnly)
	ETurning TurningState;

#pragma endregion
protected:
	virtual void BeginPlay() override;
	
	virtual void BeginDestroy() override;
private:
	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USpringArmComponent> SpringArm1P;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> IA_FireWeapon;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> IA_ReloadWeapon;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> IA_CycleWeapon;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UInputAction> IA_AimWeapon;
	
	UFUNCTION()
	void Input_FireWeapon_Pressed();
	UFUNCTION()
	void Input_FireWeapon_Released();
	UFUNCTION()
	void Input_AimWeapon_Pressed();
	UFUNCTION()
	void Input_AimWeapon_Released();
	UFUNCTION()
	void Input_CycleWeapon();
	UFUNCTION()
	void Input_ReloadWeapon();
	
	UFUNCTION()
	void CalculateFABRIK_SocketTransform();
	
	UFUNCTION()
	void CalculateTurnParameters(float DeltaTime);
	
	UFUNCTION()
	void TurnToMovement(float DeltaTime);
	
	bool bWeaponFirstReplicated;
};
