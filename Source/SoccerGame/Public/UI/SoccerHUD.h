#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SoccerHUD.generated.h"

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerHUD : public AHUD
{
	GENERATED_BODY()

public:
	ASoccerHUD();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MatchWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> MatchWidget;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMatchWidget();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMatchWidget();
};
