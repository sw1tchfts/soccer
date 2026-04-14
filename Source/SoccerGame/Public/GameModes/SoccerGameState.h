#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Types/SoccerTypes.h"
#include "SoccerGameState.generated.h"

class ASoccerPlayerState;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASoccerGameState();

	// ---- Replicated Match State ----

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Match")
	EMatchPhase CurrentMatchPhase;

	UPROPERTY(ReplicatedUsing = OnRep_HomeScore, BlueprintReadOnly, Category = "Score")
	int32 HomeScore;

	UPROPERTY(ReplicatedUsing = OnRep_AwayScore, BlueprintReadOnly, Category = "Score")
	int32 AwayScore;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	float MatchTimeRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	int32 CurrentHalf;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	ETeamID LastGoalTeam;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	EBallState CurrentBallState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	FMatchSettings MatchSettings;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match")
	FString LastScorerName;

	// ---- Rep Notifies ----

	UFUNCTION()
	void OnRep_MatchPhase();

	UFUNCTION()
	void OnRep_HomeScore();

	UFUNCTION()
	void OnRep_AwayScore();

	// ---- Accessors ----

	UFUNCTION(BlueprintCallable, Category = "Score")
	void AddGoal(ETeamID Team);

	UFUNCTION(BlueprintPure, Category = "Score")
	int32 GetScore(ETeamID Team) const;

	UFUNCTION(BlueprintPure, Category = "Match")
	FString GetMatchTimeFormatted() const;

	UFUNCTION(BlueprintPure, Category = "Match")
	bool IsMatchInProgress() const;

	UFUNCTION(BlueprintCallable, Category = "Teams")
	TArray<ASoccerPlayerState*> GetPlayersOnTeam(ETeamID Team) const;

	UFUNCTION(BlueprintPure, Category = "Teams")
	int32 GetPlayerCountOnTeam(ETeamID Team) const;

	UFUNCTION(BlueprintPure, Category = "Match")
	ETeamID GetWinningTeam() const;

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnScoreChanged(ETeamID Team, int32 NewScore);

	UFUNCTION(BlueprintImplementableEvent, Category = "Match")
	void BP_OnPhaseChanged(EMatchPhase NewPhase);

	// ---- Replication ----

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
