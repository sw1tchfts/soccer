#include "UI/SoccerHUD.h"
#include "Blueprint/UserWidget.h"

ASoccerHUD::ASoccerHUD()
{
}

void ASoccerHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASoccerHUD::ShowMatchWidget()
{
	if (!MatchWidget && MatchWidgetClass)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (PC)
		{
			MatchWidget = CreateWidget<UUserWidget>(PC, MatchWidgetClass);
			if (MatchWidget)
			{
				MatchWidget->AddToViewport();
			}
		}
	}
	else if (MatchWidget)
	{
		MatchWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ASoccerHUD::HideMatchWidget()
{
	if (MatchWidget)
	{
		MatchWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}
