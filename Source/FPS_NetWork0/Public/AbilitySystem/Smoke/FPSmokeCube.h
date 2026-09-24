// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "FPSmokeCube.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * AFPSmokeCube —— 烟雾弹本体。
 *
 * 生命周期：
 *   服务端 SpawnActor（bReplicates = true）
 *     → Launch() 给一个抛物线初速度
 *     → ProjectileMovement 带着它飞（碰撞只对 WorldStatic 生效，玩家和子弹都能穿过去）
 *     → 落地触发 OnProjectileStop
 *     → 停住、从 FlyingScale 展开到 DeployedScale、SetLifeSpan 计时销毁
 *     → 通过【投掷者的 ASC】执行一次 GameplayCue.Smoke
 *
 * 为什么视觉不挂在 GameplayCue 上，而挂在这个 Actor 上：
 *   引擎的 GameplayCue 的 Executed 事件走的是 `NetMulticast, unreliable` RPC
 *   （见 AbilitySystemComponent.h 里 NetMulticast_InvokeGameplayCueExecuted_* 的声明），
 *   丢包或网络相关性丢失就会有人看不到。而本 Actor 是 Replicates 的，
 *   位置与存在性都走可靠的属性复制 —— 所以"能不能看到烟"这件事必须由它保证，
 *   Cue 只承担落地瞬间的一次性表现（砰的一下）。
 *
 * 为什么客户端不做本地模拟：
 *   两端各自跑 ProjectileMovement 会因浮点/帧率差异走出不同轨迹，
 *   所以这里让服务器独家模拟、客户端靠移动复制跟随（BeginPlay 里关掉客户端的模拟）。
 */
UCLASS()
class FPS_NETWORK0_API AFPSmokeCube : public AActor
{
	GENERATED_BODY()

public:
	AFPSmokeCube();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 由 UFPSAbility_Smoke 在生成后立刻调用，给投射物一个初速度。只在服务器有意义。 */
	void Launch(const FVector& InitialVelocity);

protected:
	// ---------------- 组件 ----------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Smoke")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS|Smoke")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// ---------------- 可调参数（都在 BP_SmokeCube 里改） ----------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	TObjectPtr<UMaterialInterface> SmokeMaterial;

	/** 飞行途中的缩放。Cube 原始边长 100cm，0.5 = 50cm 的小方块。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float FlyingScale = 0.5f;

	/** 落地后展开的缩放。5.0 = 500cm 边长，即 5 米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float DeployedScale = 5.0f;

	/** 落地后存活多少秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	float LifeSeconds = 12.0f;

	/** 落地时执行的 GameplayCue 标签，蓝图里设成 GameplayCue.Smoke。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Smoke")
	FGameplayTag LandingCueTag;

	// ---------------- 落地流程 ----------------

	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	/**
	 * 落地后的视觉切换。
	 * 服务器在 OnProjectileStopped 里直接调；客户端通过 bDeployed 的 OnRep 调到。
	 * 缩放不在 FRepMovement 的可靠复制范围内，所以必须自己用 ReplicatedUsing 带过去。
	 */
	void ApplyDeployedVisuals();

	UFUNCTION()
	void OnRep_Deployed();

	/** 由投掷者的 ASC 执行一次落地 Cue（引擎内部走 NetMulticast，所有客户端都会播）。 */
	void ExecuteLandingCue();

	/** 蓝图钩子：落地瞬间加粒子、换材质、播声音都挂这里。服务器与各客户端都会触发。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Smoke")
	void OnSmokeDeployed();

private:
	/** 是否已经落地展开。复制到客户端后由 OnRep_Deployed 触发视觉切换。 */
	UPROPERTY(ReplicatedUsing = OnRep_Deployed)
	bool bDeployed = false;
};

/* ============================================================================
 * 【UE 编辑器操作说明 — AFPSmokeCube】
 *
 * ── 1. 建烟雾材质 M_Smoke ─────────────────────────────────────────
 *   Content Browser → Content/FPS_NetWork0/AbilitySystem/ 下新建文件夹 Materials
 *   → 右键 → Material → 命名 M_Smoke → 双击打开：
 *     - Details → Blend Mode = Translucent
 *     - Details → Shading Model = Unlit（烟雾自发光更省，也更像烟）
 *     - Details → Two Sided = 勾上（玩家可能从内部看）
 *     - 把 Opacity 设为 0.6（常量节点直接接 Base Color 也行，先能看就行）
 *     - Base Color 用深灰白（0.7, 0.7, 0.72）
 *   保存。
 *
 * ── 2. 建烟雾 Actor 蓝图 BP_SmokeCube ──────────────────────────────
 *   Content/FPS_NetWork0/AbilitySystem/Smoke/ 右键 → Blueprint Class
 *   → 搜索 "FPSmokeCube" → 命名 BP_SmokeCube
 *
 *   双击打开后，Class Defaults 面板里改：
 *     Category "FPS|Smoke"：
 *       Cube Mesh        = /Engine/BasicShapes/Cube（默认已是这个，可不动）
 *       Smoke Material   = M_Smoke          ← 【必改】，默认是引擎灰材质
 *       Flying Scale     = 0.5
 *       Deployed Scale   = 5.0
 *       Life Seconds     = 12.0
 *       Landing Cue Tag  = GameplayCue.Smoke  ← 【必填】，下拉里能选到
 *
 *   可选：在事件图里实现 On Smoke Deployed，接 Spawn Emitter at Location 加粒子。
 *        （工程里现成可用的有 Content/Effects/Particles/Weapons/Emitters/
 *          NE_MuzzleFlashSmokePuff，作为占位够用。）
 *
 * ── 3. 之后再把 BP_SmokeCube 填到 BP_GA_Smoke 的 Smoke Cube Class 字段 ──
 *   （见 Abilities/FPSAbility_Smoke.h 末尾的说明）
 * ==========================================================================*/
