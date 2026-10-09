
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPS_Controller.generated.h"

class UInputMappingContext;
struct FInputActionValue;
class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerStateReplicated);

/** 中途离开对局的落点。 */
UENUM(BlueprintType)
enum class EFPSLeaveMatchMode : uint8
{
	/** 返回主菜单 Lobby（不结束进程）。 */
	ReturnToLobby	UMETA(DisplayName = "返回主菜单"),
	/** 直接退出游戏进程。 */
	QuitGame		UMETA(DisplayName = "退出游戏"),
};

UCLASS()
class FPS_NETWORK0_API AFPS_Controller : public APlayerController
{
	GENERATED_BODY()
public:
	AFPS_Controller();
	
#pragma region UI
	
	//ScoreWidget
	UPROPERTY(BlueprintAssignable)
	FPlayerStateReplicated OnPlayerStateReplicated;
	virtual void OnRep_PlayerState() override;
	
	//UI在BeginPlay中读取角色名后发给服务器再复制到本地
	UFUNCTION(Server, Reliable)
	void Server_SetPlayerName(const FString& InName);
	
	//用于显示人数不够打不开游戏的请求
	UFUNCTION(Client, Reliable)
	void Client_ShowMatchMessage(const FString& Message);
	
#pragma endregion UI
	
#pragma region GameStart_End
	
	//房主按下"开始对局"键 → 服务端。 
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "FPS|Match")
	void Server_RequestStartMatch();
	
	//{Modify_存档触发点}
	/**
	* 离开世界时的兜底写盘。
	* 三条路径都会走到这里：
	* 进程退出（Alt+F4 / QuitGame）   → EndPlayReason::Quit
	* ClientTravel 回主菜单            → EndPlayReason::Destroyed
	* 编辑器停止 PIE                     → EndPlayReason::EndPlayInEditor
	* 不按 reason 过滤：上面三条里没有 LevelTransition，
	* 只认它会把返回主菜单和进程退出全漏掉。
	* 按引擎实现 UWorld::EndPlay 会遍历所有 Actor 调 RouteEndPlay，所以本函数一定会被调到。
	* 但只认本机玩家的 PC。服务器上别的玩家 PC 被销毁时也会进这里，但 IsLocalController() 为假。
	* 【两条硬约束，改这个函数前务必先读】：
	*   1. 这里【不能读 PlayerState】。宿主的 PC 被销毁时，AController::Destroyed 会先跑
	*      GameMode->Logout() 再跑 CleanupPlayerState()，等走到 EndPlay 时 GetPlayerState()
	*      已经是 nullptr。所以本局数据只能靠 AFPSPlayerState::PushProgressToLocalProfile()
	*      的渐进式更新保证 —— 这正是那个函数必须存在的原因。
	*      
	*   2. 不要按 EndPlayReason 过滤。ClientTravel 时 PC 是被 UEngine::LoadMap 里的
	*      DestroyActor 干掉的，拿到的是 Destroyed 而不是 LevelTransition；只认
	*      LevelTransition 会把返回主菜单和进程退出两条主路径全漏掉。
	*
	* 这里【只写盘，绝不 Wins++】—— 胜场只在服务端判胜时 +1，重复写盘是幂等的。
	*/
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	//蓝图中调用，通过UI中的选项执行操作
	UFUNCTION(BlueprintCallable, Category = "FPS|Match")
	void RequestLeaveMatch(EFPSLeaveMatchMode Mode);

	/** 按下 Esc 时抛给蓝图：在这里创建你自己做的暂停菜单 Widget。C++ 不碰任何 UI。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Input")
	void OnPauseRequested();
	
#pragma endregion GameStart_End
	
	bool bPawnAlive;
	
protected:
	
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
private:
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputMappingContext> FPSIMC;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> CrouchAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> QuitGameAction;
	
	UPROPERTY(EditAnywhere,Category="FPS|Input")
	TObjectPtr<UInputAction> StartGameAction;
	
	void Input_Look(const FInputActionValue& Look);
	void Input_Crouch();
	void Input_Jump();
	void Input_Move(const FInputActionValue& Move);
	void Input_RequestStartMatch();
	/** Esc 的 C++ 侧处理：只把事件转发给蓝图的 OnPauseRequested，不自己开菜单。 */
	void Input_Pause();
};
