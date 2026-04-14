#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Types/SoccerTypes.h"
#include "SoccerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class USphereComponent;
class ASoccerBall;

UCLASS(Blueprintable)
class SOCCERGAME_API ASoccerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASoccerCharacter();

	// ---- Components ----

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<USphereComponent> BallDetectionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<USceneComponent> BallCarrySocket;

	// ---- Replicated State ----

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ball")
	bool bHasBall;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bIsTackling;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bIsDiving;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "State")
	bool bIsKicking;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
	ETeamID TeamID;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Team")
	EPlayerPosition Position;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentStamina, BlueprintReadOnly, Category = "Movement")
	float CurrentStamina;

	// ---- Movement Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BaseWalkSpeed = SoccerConstants::BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = SoccerConstants::SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintStaminaCostPerSecond = SoccerConstants::SprintStaminaDrain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float StaminaRegenPerSecond = SoccerConstants::StaminaRegen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxStamina = SoccerConstants::MaxStamina;

	// ---- Kick Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kick")
	float MaxKickChargeTime = SoccerConstants::MaxKickChargeTime;

	// ---- Tackle Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tackle")
	float TackleDistance = SoccerConstants::TackleDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tackle")
	float TackleCooldownTime = SoccerConstants::TackleCooldown;

	// ---- GK Config ----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GK")
	float DiveDistance = SoccerConstants::GoalkeeperDiveDistance;

	// ---- Runtime State ----

	UPROPERTY(BlueprintReadOnly, Category = "Ball")
	TObjectPtr<ASoccerBall> NearbyBall;

	UPROPERTY(BlueprintReadOnly, Category = "Kick")
	float KickChargeProgress;

protected:
	float KickChargeStartTime;
	bool bIsChargingKick;
	float LastTackleTime;
	bool bStaminaLockedOut;

	FTimerHandle TackleTimerHandle;
	FTimerHandle DiveTimerHandle;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateStamina(float DeltaTime);
	void UpdateSprintSpeed();

public:
	// ---- Rep Notifies ----

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION()
	void OnRep_CurrentStamina();

	// ---- Movement Actions ----

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveForward(float Value);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveRight(float Value);

	// ---- Ball Actions ----

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void StartKickCharge();

	UFUNCTION(BlueprintCallable, Category = "Ball")
	float ReleaseKick(FVector AimDirection);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void PerformPass(FVector TargetDirection);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void PerformHeader();

	// ---- Combat Actions ----

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void PerformTackle(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void PerformGoalkeeperDive(FVector Direction);

	// ---- Ball Possession ----

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void GainBallPossession(ASoccerBall* Ball);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void LoseBallPossession();

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool IsInBallRange() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool CanKick() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool CanTackle() const;

	UFUNCTION(BlueprintPure, Category = "Ball")
	bool CanSprint() const;

	// ---- Overlap Callbacks ----

	UFUNCTION()
	void OnBallDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp,
	                                  AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                                  int32 OtherBodyIndex, bool bFromSweep,
	                                  const FHitResult& SweepResult);

	UFUNCTION()
	void OnBallDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp,
	                                AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                                int32 OtherBodyIndex);

	// ---- Multicast RPCs (Cosmetic) ----

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlayKickEffect(FVector Location);

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlayTackleEffect(FVector Location);

	UFUNCTION(NetMulticast, Unreliable, Category = "Effects")
	void Multicast_PlaySprintEffect(bool bEnable);

	// ---- Blueprint Events ----

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnBallPossessionGained();

	UFUNCTION(BlueprintImplementableEvent, Category = "Ball")
	void BP_OnBallPossessionLost();

	UFUNCTION(BlueprintImplementableEvent, Category = "Actions")
	void BP_OnKickCharged(float NormalizedPower);

	UFUNCTION(BlueprintImplementableEvent, Category = "Movement")
	void BP_OnStaminaChanged(float NewStamina, float StaminaMax);

	UFUNCTION(BlueprintImplementableEvent, Category = "Actions")
	void BP_OnTacklePerformed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Actions")
	void BP_OnDivePerformed(FVector Direction);

	UFUNCTION(BlueprintImplementableEvent, Category = "Actions")
	void BP_OnKickReleased(float NormalizedPower);

private:
	void FinishTackle();
	void FinishDive();
};
