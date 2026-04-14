#include "AI/SoccerAIController.h"
#include "Player/SoccerCharacter.h"
#include "Player/SoccerPlayerState.h"
#include "Ball/SoccerBall.h"
#include "GameModes/SoccerGameState.h"
#include "GameModes/SoccerGameMode.h"
#include "Field/SoccerPitchManager.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

ASoccerAIController::ASoccerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	CurrentDecision = EAIDecision::Idle;
}

void ASoccerAIController::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(DecisionTimerHandle, this,
		&ASoccerAIController::MakeDecision, DecisionInterval, true, DecisionInterval);
}

void ASoccerAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Execute current decision continuously
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	ASoccerBall* Ball = GetMatchBall();
	if (!Ball) return;

	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS || !GS->IsMatchInProgress()) return;

	switch (CurrentDecision)
	{
	case EAIDecision::ChaseBall:
		MoveToPosition(Ball->GetActorLocation());
		break;

	case EAIDecision::ReturnToFormation:
		MoveToPosition(HomePosition);
		break;

	case EAIDecision::PositionForPass:
		PositionForPass();
		break;

	case EAIDecision::DefendGoal:
		DefendGoal();
		break;

	case EAIDecision::InterceptBall:
		{
			// Predict where ball will be
			FVector BallPos = Ball->GetActorLocation();
			FVector BallVel = Ball->GetBallVelocity();
			FVector PredictedPos = BallPos + BallVel * 0.5f;
			MoveToPosition(PredictedPos);
		}
		break;

	default:
		break;
	}

	// Check if we're close enough to ball to act
	if (Character->IsInBallRange() && Character->bHasBall)
	{
		if (ShouldShoot())
		{
			ShootAtGoal();
		}
		else if (ShouldPass())
		{
			PassToTeammate();
		}
	}
}

void ASoccerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ASoccerCharacter* Character = Cast<ASoccerCharacter>(InPawn);
	if (Character)
	{
		// Apply AI speed multiplier
		Character->BaseWalkSpeed *= SpeedMultiplier;
		Character->SprintSpeed *= SpeedMultiplier;
		Character->GetCharacterMovement()->MaxWalkSpeed = Character->BaseWalkSpeed;
	}

	// Get assigned position from player state
	ASoccerPlayerState* PS = GetPlayerState<ASoccerPlayerState>();
	if (PS)
	{
		AssignedPosition = PS->Position;
	}
}

void ASoccerAIController::MakeDecision()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS || !GS->IsMatchInProgress())
	{
		CurrentDecision = EAIDecision::Idle;
		return;
	}

	// Update assigned position from player state
	ASoccerPlayerState* PS = GetPlayerState<ASoccerPlayerState>();
	if (PS)
	{
		AssignedPosition = PS->Position;
	}

	// Delegate to position-specific logic
	switch (AssignedPosition)
	{
	case EPlayerPosition::Goalkeeper:
		DecideAsGoalkeeper();
		break;
	case EPlayerPosition::Defender:
		DecideAsDefender();
		break;
	case EPlayerPosition::Midfielder:
		DecideAsMidfielder();
		break;
	case EPlayerPosition::Forward:
		DecideAsForward();
		break;
	}

	BP_OnDecisionMade(CurrentDecision);
}

// ---- Position-Specific Decision Logic ----

void ASoccerAIController::DecideAsGoalkeeper()
{
	ASoccerBall* Ball = GetMatchBall();
	if (!Ball) return;

	float DistToBall = GetDistanceToBall();
	FVector OwnGoal = GetOwnGoalCenter();
	float DistBallToGoal = FVector::Dist(Ball->GetActorLocation(), OwnGoal);

	if (DistToBall < 300.0f)
	{
		// Ball is very close, try to get it
		CurrentDecision = EAIDecision::ChaseBall;
	}
	else if (DistBallToGoal < 1500.0f)
	{
		// Ball approaching our goal, intercept
		CurrentDecision = EAIDecision::InterceptBall;
	}
	else
	{
		// Stay near goal
		CurrentDecision = EAIDecision::DefendGoal;
	}
}

void ASoccerAIController::DecideAsDefender()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return;

	float DistToBall = GetDistanceToBall();
	FVector OwnGoal = GetOwnGoalCenter();
	float DistBallToGoal = FVector::Dist(Ball->GetActorLocation(), OwnGoal);

	if (Character->bHasBall)
	{
		if (ShouldPass())
		{
			CurrentDecision = EAIDecision::PassToTeammate;
		}
		else
		{
			// Clear the ball
			CurrentDecision = EAIDecision::ShootAtGoal;
		}
	}
	else if (DistToBall < 500.0f && IsClosestTeammateToBall())
	{
		// We're close and no teammate is closer, chase it
		CurrentDecision = EAIDecision::ChaseBall;
	}
	else if (DistBallToGoal < 2500.0f)
	{
		// Ball is in our zone, help defend
		CurrentDecision = EAIDecision::InterceptBall;
	}
	else
	{
		CurrentDecision = EAIDecision::ReturnToFormation;
	}
}

void ASoccerAIController::DecideAsMidfielder()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return;

	float DistToBall = GetDistanceToBall();

	if (Character->bHasBall)
	{
		if (ShouldShoot())
		{
			CurrentDecision = EAIDecision::ShootAtGoal;
		}
		else if (ShouldPass())
		{
			CurrentDecision = EAIDecision::PassToTeammate;
		}
		else
		{
			// Dribble forward
			CurrentDecision = EAIDecision::ChaseBall;
		}
	}
	else if (DistToBall < 700.0f && IsClosestTeammateToBall())
	{
		CurrentDecision = EAIDecision::ChaseBall;
	}
	else if (Ball->PossessingPlayer && Ball->PossessingPlayer->TeamID == Character->TeamID)
	{
		// Teammate has ball, get open for a pass
		CurrentDecision = EAIDecision::PositionForPass;
	}
	else
	{
		// Ball is far, return to formation
		CurrentDecision = EAIDecision::ReturnToFormation;
	}
}

void ASoccerAIController::DecideAsForward()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return;

	float DistToBall = GetDistanceToBall();

	if (Character->bHasBall)
	{
		if (ShouldShoot())
		{
			CurrentDecision = EAIDecision::ShootAtGoal;
		}
		else
		{
			// Keep dribbling toward goal
			FVector GoalDir = GetOpponentGoalCenter() - Character->GetActorLocation();
			GoalDir.Normalize();
			MoveToPosition(Character->GetActorLocation() + GoalDir * 200.0f);
			CurrentDecision = EAIDecision::ChaseBall;
		}
	}
	else if (DistToBall < 800.0f && IsClosestTeammateToBall())
	{
		CurrentDecision = EAIDecision::ChaseBall;
	}
	else if (Ball->PossessingPlayer && Ball->PossessingPlayer->TeamID == Character->TeamID)
	{
		CurrentDecision = EAIDecision::PositionForPass;
	}
	else
	{
		// Press opponent defenders
		if (DistToBall < 1500.0f)
		{
			CurrentDecision = EAIDecision::ChaseBall;
		}
		else
		{
			CurrentDecision = EAIDecision::ReturnToFormation;
		}
	}
}

// ---- Actions ----

void ASoccerAIController::MoveToPosition(FVector Target)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	FVector Direction = Target - Character->GetActorLocation();
	Direction.Z = 0.0f;

	float Distance = Direction.Size();
	if (Distance < 50.0f) return; // Close enough

	Direction.Normalize();

	// Sprint if far away and have stamina
	if (Distance > 500.0f && Character->CanSprint())
	{
		Character->StartSprint();
	}
	else
	{
		Character->StopSprint();
	}

	Character->AddMovementInput(Direction, 1.0f);

	// Face movement direction
	FRotator TargetRotation = Direction.Rotation();
	Character->SetActorRotation(FMath::RInterpTo(
		Character->GetActorRotation(), TargetRotation,
		GetWorld()->GetDeltaSeconds(), 10.0f));
}

void ASoccerAIController::ChaseBall()
{
	ASoccerBall* Ball = GetMatchBall();
	if (Ball)
	{
		MoveToPosition(Ball->GetActorLocation());
	}
}

void ASoccerAIController::PassToTeammate()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character || !Character->bHasBall) return;

	ASoccerCharacter* Target = FindBestPassTarget();
	if (Target)
	{
		FVector PassDir = Target->GetActorLocation() - Character->GetActorLocation();
		PassDir = AddInaccuracy(PassDir);
		Character->PerformPass(PassDir);
	}
}

void ASoccerAIController::ShootAtGoal()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character || !Character->bHasBall) return;

	FVector GoalCenter = GetOpponentGoalCenter();
	FVector ShootDir = GoalCenter - Character->GetActorLocation();

	// Add some randomness to aim
	ShootDir = AddInaccuracy(ShootDir);

	// Power based on distance
	float Distance = FVector::Dist(Character->GetActorLocation(), GoalCenter);
	float Power = FMath::Clamp(Distance / 3000.0f, 0.4f, 1.0f);

	float KickForce = FMath::Lerp(SoccerConstants::KickForceMin,
	                               SoccerConstants::KickForceMax, Power);

	if (Character->NearbyBall)
	{
		Character->NearbyBall->Kick(ShootDir.GetSafeNormal(), KickForce, Character);
	}
}

void ASoccerAIController::TackleNearestOpponent()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	ASoccerCharacter* Opponent = FindNearestOpponent();
	if (Opponent)
	{
		FVector TackleDir = Opponent->GetActorLocation() - Character->GetActorLocation();
		if (TackleDir.Size() < SoccerConstants::TackleRange && Character->CanTackle())
		{
			Character->PerformTackle(TackleDir);
		}
	}
}

void ASoccerAIController::ReturnToFormation()
{
	MoveToPosition(HomePosition);
}

void ASoccerAIController::PositionForPass()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	// Move to a position that's forward and laterally offset from current position
	FVector GoalDir = GetOpponentGoalCenter() - Character->GetActorLocation();
	GoalDir.Z = 0.0f;
	GoalDir.Normalize();

	// Offset sideways
	FVector SideDir = FVector::CrossProduct(GoalDir, FVector::UpVector);
	float SideOffset = FMath::RandRange(-500.0f, 500.0f);

	FVector TargetPos = HomePosition + GoalDir * 300.0f + SideDir * SideOffset;
	MoveToPosition(TargetPos);
}

void ASoccerAIController::DefendGoal()
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return;

	FVector GoalCenter = GetOwnGoalCenter();
	FVector BallPos = Ball->GetActorLocation();

	// Position between ball and goal, close to goal line
	FVector BallToGoal = GoalCenter - BallPos;
	BallToGoal.Z = 0.0f;
	BallToGoal.Normalize();

	// Stay close to goal but shift toward ball laterally
	FVector DefendPos = GoalCenter + (BallPos - GoalCenter).GetSafeNormal() * 200.0f;
	DefendPos.Z = Character->GetActorLocation().Z;

	// Clamp to goal area width
	float GoalHalfWidth = SoccerConstants::GoalWidth * 0.5f + 100.0f;
	DefendPos.Y = FMath::Clamp(DefendPos.Y, GoalCenter.Y - GoalHalfWidth,
	                            GoalCenter.Y + GoalHalfWidth);

	MoveToPosition(DefendPos);
}

// ---- Utility ----

ASoccerCharacter* ASoccerAIController::GetSoccerCharacter() const
{
	return Cast<ASoccerCharacter>(GetPawn());
}

ASoccerBall* ASoccerAIController::GetMatchBall() const
{
	ASoccerGameMode* GM = Cast<ASoccerGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));
	return GM ? GM->MatchBall : nullptr;
}

ASoccerGameState* ASoccerAIController::GetSoccerGameState() const
{
	return Cast<ASoccerGameState>(
		UGameplayStatics::GetGameState(GetWorld()));
}

float ASoccerAIController::GetDistanceToBall() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return MAX_FLT;
	return FVector::Dist(Character->GetActorLocation(), Ball->GetActorLocation());
}

FVector ASoccerAIController::GetOpponentGoalCenter() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return FVector::ZeroVector;

	ASoccerGameMode* GM = Cast<ASoccerGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return FVector::ZeroVector;

	// Our opponent's goal is the one our team attacks
	if (Character->TeamID == ETeamID::Home)
	{
		return GM->AwayGoal ? GM->AwayGoal->GetActorLocation() : FVector(4000.0f, 0.0f, 122.0f);
	}
	return GM->HomeGoal ? GM->HomeGoal->GetActorLocation() : FVector(-4000.0f, 0.0f, 122.0f);
}

FVector ASoccerAIController::GetOwnGoalCenter() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return FVector::ZeroVector;

	ASoccerGameMode* GM = Cast<ASoccerGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));
	if (!GM) return FVector::ZeroVector;

	if (Character->TeamID == ETeamID::Home)
	{
		return GM->HomeGoal ? GM->HomeGoal->GetActorLocation() : FVector(-4000.0f, 0.0f, 122.0f);
	}
	return GM->AwayGoal ? GM->AwayGoal->GetActorLocation() : FVector(4000.0f, 0.0f, 122.0f);
}

bool ASoccerAIController::IsClosestTeammateToBall() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	ASoccerBall* Ball = GetMatchBall();
	if (!Character || !Ball) return false;

	float MyDist = GetDistanceToBall();

	for (TActorIterator<ASoccerCharacter> It(GetWorld()); It; ++It)
	{
		ASoccerCharacter* Other = *It;
		if (Other == Character) continue;
		if (Other->TeamID != Character->TeamID) continue;

		float OtherDist = FVector::Dist(Other->GetActorLocation(), Ball->GetActorLocation());
		if (OtherDist < MyDist)
		{
			return false;
		}
	}
	return true;
}

ASoccerCharacter* ASoccerAIController::FindBestPassTarget() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return nullptr;

	ASoccerCharacter* BestTarget = nullptr;
	float BestScore = -MAX_FLT;
	FVector GoalDir = GetOpponentGoalCenter() - Character->GetActorLocation();
	GoalDir.Normalize();

	for (TActorIterator<ASoccerCharacter> It(GetWorld()); It; ++It)
	{
		ASoccerCharacter* Other = *It;
		if (Other == Character) continue;
		if (Other->TeamID != Character->TeamID) continue;

		FVector ToOther = Other->GetActorLocation() - Character->GetActorLocation();
		float Distance = ToOther.Size();

		// Prefer teammates who are forward and not too far
		float ForwardScore = FVector::DotProduct(ToOther.GetSafeNormal(), GoalDir);
		float DistanceScore = 1.0f - FMath::Clamp(Distance / 3000.0f, 0.0f, 1.0f);

		float Score = ForwardScore * 0.6f + DistanceScore * 0.4f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Other;
		}
	}

	return BestTarget;
}

ASoccerCharacter* ASoccerAIController::FindNearestOpponent() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return nullptr;

	ASoccerCharacter* Nearest = nullptr;
	float NearestDist = MAX_FLT;

	for (TActorIterator<ASoccerCharacter> It(GetWorld()); It; ++It)
	{
		ASoccerCharacter* Other = *It;
		if (Other->TeamID == Character->TeamID) continue;
		if (Other->TeamID == ETeamID::None) continue;

		float Dist = FVector::Dist(Character->GetActorLocation(), Other->GetActorLocation());
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			Nearest = Other;
		}
	}

	return Nearest;
}

bool ASoccerAIController::ShouldShoot() const
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return false;

	float DistToGoal = FVector::Dist(Character->GetActorLocation(), GetOpponentGoalCenter());

	// Shoot if close enough to goal
	if (DistToGoal < 2000.0f) return true;

	// Also shoot if medium range with good angle
	if (DistToGoal < 3000.0f && FMath::FRand() < AggressionFactor * 0.5f) return true;

	return false;
}

bool ASoccerAIController::ShouldPass() const
{
	ASoccerCharacter* Target = FindBestPassTarget();
	if (!Target) return false;

	// Pass if there's a teammate in a better position
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return false;

	float MyDistToGoal = FVector::Dist(Character->GetActorLocation(), GetOpponentGoalCenter());
	float TargetDistToGoal = FVector::Dist(Target->GetActorLocation(), GetOpponentGoalCenter());

	// Pass if teammate is closer to goal
	return TargetDistToGoal < MyDistToGoal * 0.8f;
}

FVector ASoccerAIController::AddInaccuracy(FVector Direction) const
{
	Direction.Normalize();

	float InaccuracyRad = FMath::DegreesToRadians(
		FMath::RandRange(-KickInaccuracyDegrees, KickInaccuracyDegrees));

	FVector RotatedDir = Direction.RotateAngleAxis(
		FMath::RadiansToDegrees(InaccuracyRad), FVector::UpVector);

	return RotatedDir;
}

void ASoccerAIController::SetHomePosition(FVector NewHome)
{
	HomePosition = NewHome;
}

void ASoccerAIController::SetAssignedPosition(EPlayerPosition NewPosition)
{
	AssignedPosition = NewPosition;
}
