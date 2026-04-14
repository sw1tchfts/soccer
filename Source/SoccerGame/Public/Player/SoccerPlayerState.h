#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Types/SoccerTypes.h"
#include "SoccerPlayerState.generated.h"

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ASoccerPlayerState();

	// ---- Replicated Properties ----

	UPROPERTY(ReplicatedUsing = OnRep_TeamID, BlueprintReadOnly, Category = "Team")
	ETeamID TeamID;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
	EPlayerPosition Position;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Stats")
	FPlayerMatchStats MatchStats;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bIsBot;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	int32 JerseyNumber;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bIsReady;

	// ---- Rep Notifies ----

	UFUNCTION()
	void OnRep_TeamID();

	// ---- Stat Tracking ----

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddGoal();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddAssist();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddSave();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddTackle();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddPass();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddShot(bool bOnTarget);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetStats();

	// ---- Team Management ----

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeam(ETeamID NewTeam);

	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetPosition(EPlayerPosition NewPosition);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetReady(bool bNewReady);

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Team")
	void BP_OnTeamAssigned(ETeamID NewTeam);

	UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
	void BP_OnGoalScored();

	UFUNCTION(BlueprintImplementableEvent, Category = "Stats")
	void BP_OnAssistRecorded();

	// ---- Replication ----

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
