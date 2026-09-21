#include "Tags/ShooterGameplayTags.h"


namespace WeaponTypeTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_WeaponType_None,"Weapon.Type.None","No Weapon Type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_WeaponType_Rifle,"Weapon.Type.Rifle","Rifle Weapon Type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_WeaponType_Pistol,"Weapon.Type.Pistol","Pistol Weapon Type");
}

//Modify//{BadHandling}：应该为示例项目中如同标签一样建立好命名空间分层，而不是以下划线_
namespace FPSTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_Smoke, "Ability.Smoke", "烟雾弹能力：按键激活，ServerOnly");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_ActivateOnGiven, "Ability.ActivateOnGiven", "带此标签的能力在被授予时自动激活");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Cooldown_Smoke, "Cooldown.Smoke", "烟雾弹冷却中");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_Bullet, "Data.Damage.Bullet", "SetByCaller：子弹直击伤害（负值）");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Data_Damage_Fire, "Data.Damage.Fire", "SetByCaller：持续伤害每跳伤害（负值）");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_State_Dead, "State.Dead", "已死亡（LooseGameplayTag，重生时移除）");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Effect_DamageOverTime_Burn, "Effect.DamageOverTime.Burn", "燃烧类持续伤害");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_Smoke, "GameplayCue.Smoke", "烟雾弹落地爆发表现");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_GameplayCue_DamageOverTime_Burn, "GameplayCue.DamageOverTime.Burn", "燃烧持续表现");
}
//Modify//
