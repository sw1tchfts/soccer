#include "Field/SoccerGoal.h"
#include "Ball/SoccerBall.h"
#include "GameModes/SoccerGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ASoccerGoal::ASoccerGoal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Root
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Goal frame mesh (posts + crossbar)
	GoalFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GoalFrame"));
	GoalFrame->SetupAttachment(RootScene);
	GoalFrame->SetCollisionProfileName(TEXT("BlockAll"));

	// Trigger volume inside the goal mouth
	GoalTriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GoalTrigger"));
	GoalTriggerVolume->SetupAttachment(RootScene);
	GoalTriggerVolume->SetBoxExtent(FVector(100.0f, GoalWidth * 0.5f, GoalHeight * 0.5f));
	GoalTriggerVolume->SetRelativeLocation(FVector(-100.0f, 0.0f, GoalHeight * 0.5f));
	GoalTriggerVolume->SetCollisionProfileName(TEXT("OverlapAll"));
	GoalTriggerVolume->SetGenerateOverlapEvents(true);
	GoalTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Net mesh (visual only)
	NetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetMesh"));
	NetMesh->SetupAttachment(RootScene);
	NetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DefendingTeam = ETeamID::Home;
	bGoalScoredThisPlay = false;
}

void ASoccerGoal::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GoalTriggerVolume->OnComponentBeginOverlap.AddDynamic(
			this, &ASoccerGoal::OnGoalTriggerOverlapBegin);
	}
}

void ASoccerGoal::OnGoalTriggerOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                            AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp,
                                            int32 OtherBodyIndex, bool bFromSweep,
                                            const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (bGoalScoredThisPlay) return;

	ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
	if (!Ball) return;

	// Only count goals when ball is in play
	if (Ball->BallState == EBallState::Dead) return;

	bGoalScoredThisPlay = true;

	// The scoring team is the opposite of the defending team
	ETeamID ScoringTeam = GetScoringTeam();

	// Stop the ball
	Ball->SetDead();

	// Notify the game mode
	ASoccerGameMode* GameMode = Cast<ASoccerGameMode>(
		UGameplayStatics::GetGameMode(GetWorld()));

	if (GameMode)
	{
		GameMode->OnGoalScored(ScoringTeam, Ball->LastKickedBy, nullptr);
	}

	BP_OnGoalScored(ScoringTeam);
}

void ASoccerGoal::ResetGoal()
{
	bGoalScoredThisPlay = false;
}

ETeamID ASoccerGoal::GetScoringTeam() const
{
	return (DefendingTeam == ETeamID::Home) ? ETeamID::Away : ETeamID::Home;
}
