
#include "Game/FPSGameInstance.h"

#include "Data/FPSPlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Shader/Preshader2.h"

//const FString UFPSGameInstance::ProfileSlotName = TEXT("PlayerProfile");{Tips}:使用内联替换.cpp中定义

void UFPSGameInstance::Init()
{
	Super::Init();

	// 必须早于任何 PlayerState 的创建：AFPSPlayerState::BeginPlay 会来这里取历史最高连杀。
	LoadProfile();
}

void UFPSGameInstance::Shutdown()
{
	// 兜底写一次。防止退游戏后，当前记录无法保存
	WriteToDisk();

	Super::Shutdown();
}

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
//之后要获得SvaeGame数据就在Profile获得数据

void UFPSGameInstance::SaveMatchResult(const FString& InPlayerName, int32 KillsInMatch, int32 BestStreak, bool bWon)
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

	// 三项都是"取历史最大值"，不存在回退的可能 —— 所以不需要比较当前值和传入值以外的东西。
	Profile->HighestKillsInMatch = FMath::Max(Profile->HighestKillsInMatch, KillsInMatch);
	Profile->HighestKillStreak = FMath::Max(Profile->HighestKillStreak, BestStreak);

	if (bWon)
	{
		++Profile->Wins;
	}

	WriteToDisk();
}

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


void UFPSGameInstance::WriteToDisk()
{
	if (Profile)
	{
		UGameplayStatics::SaveGameToSlot(Profile, ProfileSlotName, 0);
	}
}
