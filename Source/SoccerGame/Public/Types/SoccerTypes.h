#pragma once

#include "CoreMinimal.h"
#include "SoccerTypes.generated.h"

// ---- Match Phase ----

UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	WaitingForPlayers   UMETA(DisplayName = "Waiting For Players"),
	Countdown           UMETA(DisplayName = "Countdown"),
	FirstHalf           UMETA(DisplayName = "First Half"),
	Halftime            UMETA(DisplayName = "Halftime"),
	SecondHalf          UMETA(DisplayName = "Second Half"),
	Overtime            UMETA(DisplayName = "Overtime"),
	PenaltyShootout     UMETA(DisplayName = "Penalty Shootout"),
	PostMatch           UMETA(DisplayName = "Post Match")
};

// ---- Team Identification ----

UENUM(BlueprintType)
enum class ETeamID : uint8
{
	None    UMETA(DisplayName = "None"),
	Home    UMETA(DisplayName = "Home"),
	Away    UMETA(DisplayName = "Away")
};

// ---- Player Position ----

UENUM(BlueprintType)
enum class EPlayerPosition : uint8
{
	Goalkeeper   UMETA(DisplayName = "GK"),
	Defender     UMETA(DisplayName = "DEF"),
	Midfielder   UMETA(DisplayName = "MID"),
	Forward      UMETA(DisplayName = "FWD")
};

// ---- Ball State ----

UENUM(BlueprintType)
enum class EBallState : uint8
{
	Free        UMETA(DisplayName = "Free"),
	Possessed   UMETA(DisplayName = "Possessed"),
	InFlight    UMETA(DisplayName = "In Flight"),
	Dead        UMETA(DisplayName = "Dead")
};

// ---- Match Settings ----

USTRUCT(BlueprintType)
struct FMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float HalfDurationSeconds = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	bool bOvertimeEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float OvertimeDurationSeconds = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	int32 PlayersPerTeam = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float CountdownDurationSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float HalftimeDurationSeconds = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	float GoalCelebrationDurationSeconds = 3.0f;
};

// ---- Per-Player Match Stats ----

USTRUCT(BlueprintType)
struct FPlayerMatchStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Goals = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Assists = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Saves = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Tackles = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Passes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Shots = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShotsOnTarget = 0;
};

// ---- Game Constants ----

namespace SoccerConstants
{
	// Kick forces (Newtons in UE units)
	constexpr float KickForceMin = 800.0f;
	constexpr float KickForceMax = 3000.0f;
	constexpr float PassForce = 1500.0f;
	constexpr float HeaderForce = 1200.0f;

	// Kick timing
	constexpr float TapKickThreshold = 0.2f;       // seconds; below = pass, above = power shot
	constexpr float MaxKickChargeTime = 1.5f;       // seconds to reach full power

	// Movement
	constexpr float BaseWalkSpeed = 600.0f;         // units/sec
	constexpr float SprintSpeed = 960.0f;           // units/sec
	constexpr float SprintSpeedMultiplier = 1.6f;
	constexpr float DribbleSpeedPenalty = 0.92f;    // multiplier when carrying ball

	// Stamina
	constexpr float MaxStamina = 100.0f;
	constexpr float SprintStaminaDrain = 20.0f;     // per second
	constexpr float StaminaRegen = 15.0f;           // per second
	constexpr float StaminaMinToSprint = 10.0f;     // minimum to start sprinting
	constexpr float StaminaReenterThreshold = 30.0f;// must reach this to re-enter sprint

	// Tackle
	constexpr float TackleRange = 200.0f;           // units
	constexpr float TackleDistance = 300.0f;         // lunge distance
	constexpr float TackleDuration = 0.3f;          // seconds
	constexpr float TackleCooldown = 1.5f;          // seconds

	// Goalkeeper
	constexpr float GoalkeeperDiveDistance = 400.0f;
	constexpr float GoalkeeperDiveForce = 1500.0f;

	// Ball
	constexpr float BallRadius = 22.0f;             // cm (UE units)
	constexpr float BallMass = 0.45f;               // kg
	constexpr float BallBounceRestitution = 0.6f;
	constexpr float BallFriction = 0.4f;
	constexpr float BallLinearDamping = 0.5f;
	constexpr float BallAngularDamping = 0.3f;
	constexpr float BallPossessionRadius = 120.0f;  // overlap sphere radius

	// Field dimensions (UE units, 1 unit = 1 cm)
	constexpr float PitchLengthX = 8000.0f;         // 80m
	constexpr float PitchWidthY = 5000.0f;          // 50m
	constexpr float GoalWidth = 500.0f;             // 5m
	constexpr float GoalHeight = 244.0f;            // 2.44m (standard)

	// AI
	constexpr float AIDecisionInterval = 0.3f;      // seconds between AI re-evaluations
	constexpr float AIKickInaccuracy = 15.0f;       // degrees of random offset

	// Network
	constexpr float BallNetUpdateFrequency = 60.0f;
	constexpr float BallSnapThreshold = 50.0f;      // units; above this, snap instead of interpolate
	constexpr float BallInterpSpeed = 15.0f;
}
