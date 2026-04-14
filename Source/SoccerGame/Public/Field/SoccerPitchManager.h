#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/SoccerTypes.h"
#include "SoccerPitchManager.generated.h"

class ASoccerGoal;
class ASoccerBall;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerPitchManager : public AActor
{
	GENERATED_BODY()

public:
	ASoccerPitchManager();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> PitchBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> LeftSideline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> RightSideline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> HomeEndline;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Field")
	TObjectPtr<UBoxComponent> AwayEndline;

	// ---- Key Positions ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	FVector CenterSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> HomeKickoffPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> AwayKickoffPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> HomeFormationPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> AwayFormationPositions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	FVector HomeGoalKickSpot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	FVector AwayGoalKickSpot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> HomeCornerSpots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TArray<FVector> AwayCornerSpots;

	// ---- Field Dimensions ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	float PitchLength = SoccerConstants::PitchLengthX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	float PitchWidth = SoccerConstants::PitchWidthY;

	// ---- Goal References ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TObjectPtr<ASoccerGoal> HomeGoal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	TObjectPtr<ASoccerGoal> AwayGoal;

	// ---- Functions ----

	UFUNCTION(BlueprintPure, Category = "Field")
	bool IsBallInBounds(FVector BallLocation) const;

	UFUNCTION(BlueprintPure, Category = "Field")
	FVector GetNearestInBoundsPosition(FVector Position) const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	TArray<FVector> GetKickoffPositions(ETeamID Team) const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetFormationPosition(ETeamID Team, int32 PositionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetThrowInPosition(FVector OutLocation, ETeamID ThrowingTeam) const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetGoalKickPosition(ETeamID Team) const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetCornerKickPosition(FVector OutLocation, ETeamID KickingTeam) const;

	UFUNCTION(BlueprintPure, Category = "Field")
	bool IsInHomeHalf(FVector Location) const;

protected:
	virtual void BeginPlay() override;

	void InitializeDefaultPositions();

	UFUNCTION()
	void OnBallExitSideline(UPrimitiveComponent* OverlappedComp,
	                        AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                        int32 OtherBodyIndex);

	UFUNCTION()
	void OnBallExitEndline(UPrimitiveComponent* OverlappedComp,
	                       AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                       int32 OtherBodyIndex);
};
