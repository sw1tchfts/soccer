#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Types/SoccerTypes.h"
#include "SoccerAIController.generated.h"

class ASoccerCharacter;
class ASoccerBall;
class ASoccerGameState;

UENUM(BlueprintType)
enum class EAIDecision : uint8
{
	Idle,
	ChaseBall,
	ReturnToFormation,
	PositionForPass,
	ShootAtGoal,
	PassToTeammate,
	DefendGoal,
	TackleOpponent,
	InterceptBall
};

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASoccerAIController();

	// ---- Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DecisionInterval = SoccerConstants::AIDecisionInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AggressionFactor = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ReactionDelay = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float KickInaccuracyDegrees = SoccerConstants::AIKickInaccuracy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float SpeedMultiplier = 0.85f;

	// ---- State ----

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EPlayerPosition AssignedPosition;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	FVector HomePosition;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EAIDecision CurrentDecision;

protected:
	FTimerHandle DecisionTimerHandle;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void MakeDecision();

	// ---- Position-Specific Decision Logic ----

	void DecideAsGoalkeeper();
	void DecideAsDefender();
	void DecideAsMidfielder();
	void DecideAsForward();

	// ---- Actions ----

	void MoveToPosition(FVector Target);
	void ChaseBall();
	void PassToTeammate();
	void ShootAtGoal();
	void TackleNearestOpponent();
	void ReturnToFormation();
	void PositionForPass();
	void DefendGoal();

	// ---- Utility ----

	ASoccerCharacter* GetSoccerCharacter() const;
	ASoccerBall* GetMatchBall() const;
	ASoccerGameState* GetSoccerGameState() const;
	float GetDistanceToBall() const;
	FVector GetOpponentGoalCenter() const;
	FVector GetOwnGoalCenter() const;
	bool IsClosestTeammateToBall() const;
	ASoccerCharacter* FindBestPassTarget() const;
	ASoccerCharacter* FindNearestOpponent() const;
	bool ShouldShoot() const;
	bool ShouldPass() const;
	FVector AddInaccuracy(FVector Direction) const;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetHomePosition(FVector NewHome);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAssignedPosition(EPlayerPosition NewPosition);

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "AI")
	void BP_OnDecisionMade(EAIDecision Decision);
};
