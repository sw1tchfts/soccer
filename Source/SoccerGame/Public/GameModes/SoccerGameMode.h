#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Types/SoccerTypes.h"
#include "SoccerGameMode.generated.h"

class ASoccerBall;
class ASoccerCharacter;
class ASoccerPlayerState;
class ASoccerGameState;
class ASoccerGoal;
class ASoccerPitchManager;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASoccerGameMode();

	// ---- Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	FMatchSettings MatchSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	TSubclassOf<ASoccerBall> BallClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	TSubclassOf<AController> BotControllerClass;

	// ---- Runtime References ----

	UPROPERTY(BlueprintReadOnly, Category = "Match")
	TObjectPtr<ASoccerBall> MatchBall;

	UPROPERTY(BlueprintReadOnly, Category = "Field")
	TObjectPtr<ASoccerPitchManager> PitchManager;

	UPROPERTY(BlueprintReadOnly, Category = "Field")
	TObjectPtr<ASoccerGoal> HomeGoal;

	UPROPERTY(BlueprintReadOnly, Category = "Field")
	TObjectPtr<ASoccerGoal> AwayGoal;

protected:
	FTimerHandle MatchTimerHandle;
	FTimerHandle CountdownTimerHandle;
	FTimerHandle GoalCelebrationHandle;
	FTimerHandle HalftimeTimerHandle;

	int32 CountdownValue;
	int32 NextJerseyNumberHome;
	int32 NextJerseyNumberAway;

public:
	// ---- Overrides ----

	virtual void InitGame(const FString& MapName, const FString& Options,
	                      FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(
		AController* InController) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	// ---- Match Flow ----

	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartMatch();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartCountdown();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void StartKickoff(ETeamID KickingTeam);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void OnGoalScored(ETeamID ScoringTeam, ASoccerCharacter* Scorer,
	                  ASoccerCharacter* Assister);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void TransitionToPhase(EMatchPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void EndMatch();

	UFUNCTION()
	void TickMatchTimer();

	UFUNCTION()
	void TickCountdown();

	// ---- Team Management ----

	UFUNCTION(BlueprintCallable, Category = "Teams")
	ETeamID AssignTeam(APlayerController* Player);

	UFUNCTION(BlueprintCallable, Category = "Teams")
	void AssignPosition(ASoccerPlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category = "Teams")
	void FillEmptySlotsWithBots();

	UFUNCTION(BlueprintCallable, Category = "Teams")
	void RemoveBot(ETeamID Team);

	// ---- Ball Management ----

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SpawnBall(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ResetBallToCenter();

	// ---- Player Management ----

	UFUNCTION(BlueprintCallable, Category = "Match")
	void ResetAllPlayerPositions();

	UFUNCTION(BlueprintCallable, Category = "Match")
	void FreezeAllPlayers(bool bFreeze);

	// ---- Helpers ----

	UFUNCTION(BlueprintPure, Category = "Match")
	ASoccerGameState* GetSoccerGameState() const;

	UFUNCTION(BlueprintPure, Category = "Match")
	int32 GetTotalPlayerCount() const;

	UFUNCTION(BlueprintPure, Category = "Match")
	bool AreTeamsFull() const;

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnGoalScored(ETeamID ScoringTeam);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnMatchPhaseChanged(EMatchPhase NewPhase);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnMatchEnded(ETeamID WinningTeam);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnCountdownTick(int32 SecondsRemaining);

private:
	void FindFieldActors();
	void OnGoalCelebrationFinished();
	void OnHalftimeFinished();
	void StartSecondHalf();
	void NotifyAllClients_GoalScored(ETeamID ScoringTeam, const FString& ScorerName);
	void NotifyAllClients_PhaseChanged(EMatchPhase NewPhase);
};
