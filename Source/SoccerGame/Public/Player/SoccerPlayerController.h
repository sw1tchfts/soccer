#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "Types/SoccerTypes.h"
#include "SoccerPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ASoccerCharacter;
class ASoccerPlayerState;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASoccerPlayerController();

	// ---- Enhanced Input Assets (set in Blueprint) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Kick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Tackle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Dive;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Scoreboard;

	// ---- UI References ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> MatchHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> MatchHUDWidget;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	// ---- Input Handlers ----

	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleSprintStarted(const FInputActionValue& Value);
	void HandleSprintCompleted(const FInputActionValue& Value);
	void HandleKickStarted(const FInputActionValue& Value);
	void HandleKickCompleted(const FInputActionValue& Value);
	void HandleTackle(const FInputActionValue& Value);
	void HandleDive(const FInputActionValue& Value);
	void HandleJump(const FInputActionValue& Value);
	void HandleScoreboardPressed(const FInputActionValue& Value);
	void HandleScoreboardReleased(const FInputActionValue& Value);

public:
	// ---- Server RPCs ----

	UFUNCTION(Server, Reliable, WithValidation, Category = "Actions")
	void Server_RequestKick(FVector Direction, float Power);

	UFUNCTION(Server, Reliable, WithValidation, Category = "Actions")
	void Server_RequestTackle(FVector Direction);

	UFUNCTION(Server, Reliable, WithValidation, Category = "Actions")
	void Server_RequestDive(FVector Direction);

	UFUNCTION(Server, Reliable, Category = "Lobby")
	void Server_SetReady(bool bReady);

	// ---- Client RPCs ----

	UFUNCTION(Client, Reliable, Category = "UI")
	void Client_ShowGoalNotification(ETeamID ScoringTeam, const FString& ScorerName);

	UFUNCTION(Client, Reliable, Category = "UI")
	void Client_ShowMatchPhaseNotification(EMatchPhase NewPhase);

	// ---- UI Management ----

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMatchHUD();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMatchHUD();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void TogglePauseMenu();

	// ---- Helpers ----

	UFUNCTION(BlueprintPure, Category = "Player")
	ASoccerPlayerState* GetSoccerPlayerState() const;

	UFUNCTION(BlueprintPure, Category = "Player")
	ASoccerCharacter* GetSoccerCharacter() const;

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_OnMatchHUDCreated(UUserWidget* HUDWidget);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_OnGoalNotification(ETeamID ScoringTeam, const FString& ScorerName);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void BP_OnMatchPhaseNotification(EMatchPhase NewPhase);
};
