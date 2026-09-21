#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interfaces/PlayerInterface.h"
//Modify//
#include "AbilitySystemInterface.h"
//Modify//
#include "FPSCharacter.generated.h"

class UEliminationComponent;
class UHealthComponent;
class UCombatComponent;
class UInputAction;
class UCameraComponent;
class USpringArmComponent;
//Modify//
class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
//Modify//
enum class ETurning: uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponFirstReplicated, AWeapon*,Weapon, bool, bTargetingPlayer);

/*
//original code
UCLASS()
class FPS_NETWORK0_API AFPSCharacter : public ACharacter,public IPlayerInterface
{
	GENERATED_BODY()
*/
//Modify//
// 额外实现 IAbilitySystemInterface。
// ASC 实际挂在 AFPSPlayerState 上（跨死亡重生保留），这里只做转发：
//   - 蓝图里对角色调 GetAbilitySystemComponent 就能直接拿到；
//   - UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(角色) 也能识别，
//     UCombatComponent 施加 DoT 时依赖这一点。
UCLASS()
class FPS_NETWORK0_API AFPSCharacter : public ACharacter, public IPlayerInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
//Modify//
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

	//Modify//
	/*IAbilitySystemInterface*/
	// 转发到 AFPSPlayerState，见 Player/FPSPlayerState.cpp
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	/*IAbilitySystemInterface*/

	// ---------------- GAS ----------------
	/** 开局授予的能力列表，把 BP_GA_Smoke 填进来。服务器授予一次，之后跨重生保留。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	/** 子弹直击用的伤害 GE，指向 BP_GE_BulletDamage。留空 = 打不掉血。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/**
	 * 初始化 ASC：绑定 Owner(PlayerState)/Avatar(本角色)、服务器授予能力并初始化属性、
	 * 把属性同步给本角色的 UHealthComponent。
	 * 服务器在 PossessedBy、客户端在 OnPlayerStateChanged 里各调用一次。
	 */
	void InitializeAbilitySystem();
	//Modify//

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

	//Modify//
	/** 烟雾弹按键。填 Content/FPS_NetWork0/Input/InputAction/IA_Smoke。 */
	UPROPERTY(EditAnywhere, Category = "FPS|Input")
	TObjectPtr<UInputAction> IA_Smoke;
	//Modify//

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

	//Modify//
	UFUNCTION()
	void Input_Smoke();
	//Modify//

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
