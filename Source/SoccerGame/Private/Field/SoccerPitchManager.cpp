#include "Field/SoccerPitchManager.h"
#include "Field/SoccerGoal.h"
#include "Ball/SoccerBall.h"
#include "Components/BoxComponent.h"

ASoccerPitchManager::ASoccerPitchManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Main pitch bounds (used for general checks)
	PitchBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("PitchBounds"));
	PitchBounds->SetupAttachment(RootScene);
	PitchBounds->SetBoxExtent(FVector(PitchLength * 0.5f, PitchWidth * 0.5f, 500.0f));
	PitchBounds->SetCollisionProfileName(TEXT("OverlapAll"));
	PitchBounds->SetGenerateOverlapEvents(false);
	PitchBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Sideline triggers (thin volumes along long edges)
	LeftSideline = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftSideline"));
	LeftSideline->SetupAttachment(RootScene);
	LeftSideline->SetBoxExtent(FVector(PitchLength * 0.5f, 50.0f, 300.0f));
	LeftSideline->SetRelativeLocation(FVector(0.0f, -(PitchWidth * 0.5f + 50.0f), 150.0f));
	LeftSideline->SetCollisionProfileName(TEXT("OverlapAll"));
	LeftSideline->SetGenerateOverlapEvents(true);

	RightSideline = CreateDefaultSubobject<UBoxComponent>(TEXT("RightSideline"));
	RightSideline->SetupAttachment(RootScene);
	RightSideline->SetBoxExtent(FVector(PitchLength * 0.5f, 50.0f, 300.0f));
	RightSideline->SetRelativeLocation(FVector(0.0f, PitchWidth * 0.5f + 50.0f, 150.0f));
	RightSideline->SetCollisionProfileName(TEXT("OverlapAll"));
	RightSideline->SetGenerateOverlapEvents(true);

	// Endline triggers (behind each goal)
	HomeEndline = CreateDefaultSubobject<UBoxComponent>(TEXT("HomeEndline"));
	HomeEndline->SetupAttachment(RootScene);
	HomeEndline->SetBoxExtent(FVector(50.0f, PitchWidth * 0.5f, 300.0f));
	HomeEndline->SetRelativeLocation(FVector(-(PitchLength * 0.5f + 50.0f), 0.0f, 150.0f));
	HomeEndline->SetCollisionProfileName(TEXT("OverlapAll"));
	HomeEndline->SetGenerateOverlapEvents(true);

	AwayEndline = CreateDefaultSubobject<UBoxComponent>(TEXT("AwayEndline"));
	AwayEndline->SetupAttachment(RootScene);
	AwayEndline->SetBoxExtent(FVector(50.0f, PitchWidth * 0.5f, 300.0f));
	AwayEndline->SetRelativeLocation(FVector(PitchLength * 0.5f + 50.0f, 0.0f, 150.0f));
	AwayEndline->SetCollisionProfileName(TEXT("OverlapAll"));
	AwayEndline->SetGenerateOverlapEvents(true);
}

void ASoccerPitchManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultPositions();
}

void ASoccerPitchManager::InitializeDefaultPositions()
{
	float HalfLength = PitchLength * 0.5f;
	float HalfWidth = PitchWidth * 0.5f;

	// Center spot
	CenterSpot = GetActorLocation();

	// Home kickoff positions (1-1-2-1 formation: GK, DEF, 2 MID, FWD)
	FVector Origin = GetActorLocation();
	HomeKickoffPositions.Empty();
	HomeKickoffPositions.Add(Origin + FVector(-HalfLength + 200.0f, 0.0f, 0.0f));           // GK
	HomeKickoffPositions.Add(Origin + FVector(-HalfLength * 0.6f, 0.0f, 0.0f));              // DEF
	HomeKickoffPositions.Add(Origin + FVector(-HalfLength * 0.25f, -HalfWidth * 0.4f, 0.0f)); // MID L
	HomeKickoffPositions.Add(Origin + FVector(-HalfLength * 0.25f, HalfWidth * 0.4f, 0.0f));  // MID R
	HomeKickoffPositions.Add(Origin + FVector(-200.0f, 0.0f, 0.0f));                         // FWD

	// Away kickoff positions (mirror)
	AwayKickoffPositions.Empty();
	AwayKickoffPositions.Add(Origin + FVector(HalfLength - 200.0f, 0.0f, 0.0f));            // GK
	AwayKickoffPositions.Add(Origin + FVector(HalfLength * 0.6f, 0.0f, 0.0f));               // DEF
	AwayKickoffPositions.Add(Origin + FVector(HalfLength * 0.25f, HalfWidth * 0.4f, 0.0f));   // MID L
	AwayKickoffPositions.Add(Origin + FVector(HalfLength * 0.25f, -HalfWidth * 0.4f, 0.0f));  // MID R
	AwayKickoffPositions.Add(Origin + FVector(200.0f, 0.0f, 0.0f));                          // FWD

	// Default formation positions during play
	HomeFormationPositions = HomeKickoffPositions;
	AwayFormationPositions = AwayKickoffPositions;

	// Goal kick spots
	HomeGoalKickSpot = Origin + FVector(-HalfLength + 600.0f, 0.0f, 0.0f);
	AwayGoalKickSpot = Origin + FVector(HalfLength - 600.0f, 0.0f, 0.0f);

	// Corner spots
	HomeCornerSpots.Empty();
	HomeCornerSpots.Add(Origin + FVector(-HalfLength, -HalfWidth, 0.0f));
	HomeCornerSpots.Add(Origin + FVector(-HalfLength, HalfWidth, 0.0f));

	AwayCornerSpots.Empty();
	AwayCornerSpots.Add(Origin + FVector(HalfLength, -HalfWidth, 0.0f));
	AwayCornerSpots.Add(Origin + FVector(HalfLength, HalfWidth, 0.0f));
}

bool ASoccerPitchManager::IsBallInBounds(FVector BallLocation) const
{
	FVector Origin = GetActorLocation();
	float HalfLength = PitchLength * 0.5f;
	float HalfWidth = PitchWidth * 0.5f;

	return FMath::Abs(BallLocation.X - Origin.X) <= HalfLength &&
	       FMath::Abs(BallLocation.Y - Origin.Y) <= HalfWidth;
}

FVector ASoccerPitchManager::GetNearestInBoundsPosition(FVector Position) const
{
	FVector Origin = GetActorLocation();
	float HalfLength = PitchLength * 0.5f;
	float HalfWidth = PitchWidth * 0.5f;

	FVector Clamped = Position;
	Clamped.X = FMath::Clamp(Clamped.X, Origin.X - HalfLength, Origin.X + HalfLength);
	Clamped.Y = FMath::Clamp(Clamped.Y, Origin.Y - HalfWidth, Origin.Y + HalfWidth);
	return Clamped;
}

TArray<FVector> ASoccerPitchManager::GetKickoffPositions(ETeamID Team) const
{
	return (Team == ETeamID::Home) ? HomeKickoffPositions : AwayKickoffPositions;
}

FVector ASoccerPitchManager::GetFormationPosition(ETeamID Team, int32 PositionIndex) const
{
	const TArray<FVector>& Positions = (Team == ETeamID::Home) ?
		HomeFormationPositions : AwayFormationPositions;

	if (Positions.IsValidIndex(PositionIndex))
	{
		return Positions[PositionIndex];
	}
	return CenterSpot;
}

FVector ASoccerPitchManager::GetThrowInPosition(FVector OutLocation, ETeamID ThrowingTeam) const
{
	FVector Origin = GetActorLocation();
	float HalfWidth = PitchWidth * 0.5f;

	// Throw-in from the sideline where ball went out
	FVector ThrowPos = OutLocation;
	ThrowPos.Y = (OutLocation.Y > Origin.Y) ? (Origin.Y + HalfWidth) : (Origin.Y - HalfWidth);
	ThrowPos.Z = Origin.Z;
	return ThrowPos;
}

FVector ASoccerPitchManager::GetGoalKickPosition(ETeamID Team) const
{
	return (Team == ETeamID::Home) ? HomeGoalKickSpot : AwayGoalKickSpot;
}

FVector ASoccerPitchManager::GetCornerKickPosition(FVector OutLocation, ETeamID KickingTeam) const
{
	FVector Origin = GetActorLocation();
	const TArray<FVector>& Corners = (KickingTeam == ETeamID::Home) ?
		AwayCornerSpots : HomeCornerSpots;

	if (Corners.Num() < 2) return CenterSpot;

	// Pick the corner closest to where the ball went out
	float DistLeft = FVector::Dist(OutLocation, Corners[0]);
	float DistRight = FVector::Dist(OutLocation, Corners[1]);
	return (DistLeft < DistRight) ? Corners[0] : Corners[1];
}

bool ASoccerPitchManager::IsInHomeHalf(FVector Location) const
{
	return Location.X < GetActorLocation().X;
}

void ASoccerPitchManager::OnBallExitSideline(UPrimitiveComponent* OverlappedComp,
                                             AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp,
                                             int32 OtherBodyIndex)
{
	// Handled by GameMode via polling or delegate
}

void ASoccerPitchManager::OnBallExitEndline(UPrimitiveComponent* OverlappedComp,
                                            AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp,
                                            int32 OtherBodyIndex)
{
	// Handled by GameMode via polling or delegate
}
