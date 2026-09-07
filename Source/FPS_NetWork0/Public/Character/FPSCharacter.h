#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"
#include "FPSCharacter.generated.h"

class UEliminationComponent;
class UHealthComponent;
class UCombatComponent;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
enum class ETurning: uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponFirstReplicated, AWeapon*,Weapon, bool, bTargetingPlayer);

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
	virtual AWeapon* GetCurrentWeapon_Implementation() override;
	virtual int32 GetReserveAmmo_Implementation() const override;
	virtual void Notify_CycleWeapon_Implementation() override;
	virtual void Notify_ReloadWeapon_Implementation() override;
	virtual void AddAmmo_Implementation(const FGameplayTag& WeaponType, int32 AmmoAmount) override;
	virtual bool DoDamage_Implementation(float DamageAmount, AActor* DamageInstigator) override;
	/*Interface*/
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="FPS|Combat")
	TObjectPtr<UCombatComponent> CombatComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Elimination")
	TObjectPtr<UEliminationComponent> Elimination;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Health")
	TObjectPtr<UHealthComponent> Health;
	
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
	
#pragma region Death
	UFUNCTION()
	void OnDeathStarted();
	
	UFUNCTION(BlueprintImplementableEvent)
	void DeathEffects();
	
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Respawn")
	float RespawnTime;
#pragma endregion Death
	
protected:
	virtual void BeginPlay() override;
	
	virtual void BeginDestroy() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Mesh")
	TObjectPtr<USkeletalMeshComponent> Mesh1P;
	
	FTimerHandle DeathTimer;
	
	void DeathTimerFinished();
	
private:
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
	
#pragma region HitReact
	UPROPERTY(EditDefaultsOnly, Category = "FPS|HitReact")
	TArray<TObjectPtr<UAnimMontage>> HitReacts;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HitReact(int32 MontageIndex);
#pragma endregion HitReact
	
};
