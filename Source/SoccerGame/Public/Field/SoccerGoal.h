#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/SoccerTypes.h"
#include "SoccerGoal.generated.h"

class ASoccerBall;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerGoal : public AActor
{
	GENERATED_BODY()

public:
	ASoccerGoal();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UStaticMeshComponent> GoalFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UBoxComponent> GoalTriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal")
	TObjectPtr<UStaticMeshComponent> NetMesh;

	// ---- Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	ETeamID DefendingTeam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	FVector GoalKickSpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	float GoalWidth = SoccerConstants::GoalWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Goal")
	float GoalHeight = SoccerConstants::GoalHeight;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGoalTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp,
	                               AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                               int32 OtherBodyIndex, bool bFromSweep,
	                               const FHitResult& SweepResult);

	bool bGoalScoredThisPlay;

public:
	UFUNCTION(BlueprintCallable, Category = "Goal")
	void ResetGoal();

	UFUNCTION(BlueprintPure, Category = "Goal")
	ETeamID GetScoringTeam() const;

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Goal")
	void BP_OnGoalScored(ETeamID ScoringTeam);
};
