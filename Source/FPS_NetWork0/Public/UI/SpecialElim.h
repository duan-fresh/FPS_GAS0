

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpecialElim.generated.h"


class UTextBlock;
class UImage;

UCLASS()
class FPS_NETWORK0_API USpecialElim : public UUserWidget
{
	GENERATED_BODY()
public:
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ElimMessage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_ElimType;
	
	void InitializeWidget(const FString& ElimMessage, UTexture2D* ElimTexture);
	
	UFUNCTION(BlueprintCallable)
	static void CenterWidget(UUserWidget* Widget, float VerticalRatio = 0.f);
};
