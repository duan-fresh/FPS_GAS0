
#include "Game/FPSGameInstance.h"

#include "Data/FPSPlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Shader/Preshader2.h"

#pragma region Override

void UFPSGameInstance::Init()
{
	Super::Init();
	// 必须早于任何 PlayerState 的创建：AFPSPlayerState::BeginPlay 会来这里取历史最高连杀。
	LoadProfile();
}

void UFPSGameInstance::Shutdown()
{
	//{Modify_存档触发点}
	// 兜底写一次。防止退游戏后，当前记录无法保存。
	//现在 UpdateProfileProgress 会随着击杀实时合并，内存里就是最新战绩
	// 时机（UGameEngine::PreExit）：World->EndPlay(Quit) → 【本函数】→ World->CleanupWorld()。
	// 也就是说本机 PC 的 EndPlay 比这里更早，两条路互为冗余，谁先到都行。
	SaveProfileNow();
	//-----End-----

	Super::Shutdown();
}

#pragma endregion Override

void UFPSGameInstance::SetLocalPlayerName(const FString& InName)
{
	if (InName.IsEmpty())
	{
		return;
	}

	if (!Profile)
	{
		LoadProfile();
	}
	if (!Profile) return;
	
	Profile->PlayerName = InName;
}

FString UFPSGameInstance::GetLocalPlayerName() const
{
	return Profile ? Profile->PlayerName : FString();
}

#pragma region HandleProfile

//之后要获得SvaeGame数据就在Profile获得数据
void UFPSGameInstance::LoadProfile()
{
	if (UGameplayStatics::DoesSaveGameExist(ProfileSlotName, 0))
	{
		Profile = Cast<UFPSPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(ProfileSlotName, 0));
	}

	if (!Profile)
	{
		// 首次运行，或者磁盘上的存档版本对不上（Cast 失败）——两种情况都退回空白档案。
		Profile = Cast<UFPSPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UFPSPlayerSaveGame::StaticClass()));
	}
}

void UFPSGameInstance::UpdateProfileProgress(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak)
{
	if (!Profile)
	{
		LoadProfile();
	}
	if (!Profile)
	{
		return;
	}

	if (!InPlayerName.IsEmpty())
	{
		Profile->PlayerName = InPlayerName;
	}
	
	Profile->HighestKillsInMatch = FMath::Max(Profile->HighestKillsInMatch, KillsInMatch);
	Profile->HighestKillStreak = FMath::Max(Profile->HighestKillStreak, BestStreak);
}

void UFPSGameInstance::SaveProfileNow()
{
	if (Profile)
	{
		UGameplayStatics::SaveGameToSlot(Profile, ProfileSlotName, 0);
	}
}

void UFPSGameInstance::SaveMatchResult(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak, bool bWon)
{
	UpdateProfileProgress(InPlayerName, KillsInMatch, BestStreak);

	if (!Profile)
	{
		return;
	}

	if (bWon)
	{
		++Profile->Wins;
	}
	SaveProfileNow();
}

#pragma endregion HandleProfile