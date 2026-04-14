#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/SoccerTypes.h"
#include "SoccerBall.generated.h"

class ASoccerCharacter;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerBall : public AActor
{
	GENERATED_BODY()

public:
	ASoccerBall();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<UStaticMeshComponent> BallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<USphereComponent> CollisionSphere;

	// ---- Replicated State ----

	UPROPERTY(ReplicatedUsing = OnRep_BallState, BlueprintReadOnly, Category = "Ball")
	EBallState BallState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<ASoccerCharacter> PossessingPlayer;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<ASoccerCharacter> LastTouchedBy;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	ETeamID LastTouchedByTeam;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<ASoccerCharacter> LastKickedBy;

	// ---- Physics Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float BallMassValue = SoccerConstants::BallMass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float BounceRestitution = SoccerConstants::BallBounceRestitution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float BallFrictionValue = SoccerConstants::BallFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float LinearDamping = SoccerConstants::BallLinearDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float AngularDamping = SoccerConstants::BallAngularDamping;

	// ---- Replication: Smoothing ----

	UPROPERTY(Replicated)
	FVector ReplicatedLocation;

	UPROPERTY(Replicated)
	FVector ReplicatedVelocity;

	UPROPERTY(Replicated)
	FRotator ReplicatedRotation;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InterpolateClientPosition(float DeltaTime);
	void UpdateReplicatedState();

public:
	// ---- Rep Notifies ----

	UFUNCTION()
	void OnRep_BallState();

	// ---- Actions (Server Authority) ----

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Kick(FVector Direction, float Force, ASoccerCharacter* Kicker);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Pass(FVector Direction, float Force, ASoccerCharacter* Passer);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void Header(FVector Direction, float Force, ASoccerCharacter* HeaderPlayer);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetPossessedBy(ASoccerCharacter* Player);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ClearPossession();

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ResetToPosition(FVector NewLocation);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetDead();

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetFree();

	UFUNCTION(BlueprintPure, Category = "Ball")
	FVector GetBallVelocity() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	float GetBallSpeed() const;

	// ---- Collision ----

	UFUNCTION()
	void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	               UPrimitiveComponent* OtherComp, FVector NormalImpulse,
	               const FHitResult& Hit);

	// ---- Multicast RPCs (Cosmetic) ----

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlayKickEffect(FVector Location, float NormalizedPower);

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlayBounceEffect(FVector Location, float ImpactForce);

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlayTrailEffect(bool bEnable);

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnKicked(float NormalizedPower);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnBounced(FVector Location);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnPossessionChanged(ASoccerCharacter* NewPossessor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnBallStateChanged(EBallState NewState);
};
