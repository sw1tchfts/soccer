#include "Ball/SoccerBall.h"
#include "Player/SoccerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ASoccerBall::ASoccerBall()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false); // We handle our own replication for tighter control

	NetUpdateFrequency = SoccerConstants::BallNetUpdateFrequency;
	NetPriority = 3.0f;

	// Collision sphere (root)
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(SoccerConstants::BallRadius);
	CollisionSphere->SetCollisionProfileName(TEXT("PhysicsActor"));
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->BodyInstance.bLockXRotation = false;
	CollisionSphere->BodyInstance.bLockYRotation = false;
	CollisionSphere->BodyInstance.bLockZRotation = false;
	CollisionSphere->BodyInstance.SetMassOverride(SoccerConstants::BallMass);
	CollisionSphere->SetLinearDamping(SoccerConstants::BallLinearDamping);
	CollisionSphere->SetAngularDamping(SoccerConstants::BallAngularDamping);
	CollisionSphere->OnComponentHit.AddDynamic(this, &ASoccerBall::OnBallHit);
	SetRootComponent(CollisionSphere);

	// Visual mesh
	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	BallMesh->SetupAttachment(CollisionSphere);
	BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BallMesh->SetRelativeScale3D(FVector(SoccerConstants::BallRadius / 50.0f)); // Scale sphere mesh

	// Default state
	BallState = EBallState::Free;
	PossessingPlayer = nullptr;
	LastTouchedBy = nullptr;
	LastTouchedByTeam = ETeamID::None;
	LastKickedBy = nullptr;
}

void ASoccerBall::BeginPlay()
{
	Super::BeginPlay();

	// Only server simulates physics
	if (!HasAuthority())
	{
		CollisionSphere->SetSimulatePhysics(false);
	}
}

void ASoccerBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// Server: update replicated state from physics simulation
		UpdateReplicatedState();

		// If ball is possessed, move it toward the possessing player's carry socket
		if (BallState == EBallState::Possessed && PossessingPlayer != nullptr)
		{
			USceneComponent* CarrySocket = PossessingPlayer->FindComponentByClass<USceneComponent>();
			if (CarrySocket)
			{
				FVector TargetLocation = PossessingPlayer->GetActorLocation()
					+ PossessingPlayer->GetActorForwardVector() * 80.0f
					+ FVector(0.0f, 0.0f, -60.0f);

				FVector CurrentLocation = GetActorLocation();
				FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 15.0f);

				CollisionSphere->SetSimulatePhysics(false);
				SetActorLocation(NewLocation);
			}
		}
	}
	else
	{
		// Client: interpolate toward replicated state
		InterpolateClientPosition(DeltaTime);
	}
}

void ASoccerBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASoccerBall, BallState);
	DOREPLIFETIME(ASoccerBall, PossessingPlayer);
	DOREPLIFETIME(ASoccerBall, LastTouchedBy);
	DOREPLIFETIME(ASoccerBall, LastTouchedByTeam);
	DOREPLIFETIME(ASoccerBall, LastKickedBy);
	DOREPLIFETIME(ASoccerBall, ReplicatedLocation);
	DOREPLIFETIME(ASoccerBall, ReplicatedVelocity);
	DOREPLIFETIME(ASoccerBall, ReplicatedRotation);
}

void ASoccerBall::InterpolateClientPosition(float DeltaTime)
{
	FVector CurrentLocation = GetActorLocation();
	float Distance = FVector::Dist(CurrentLocation, ReplicatedLocation);

	if (Distance > SoccerConstants::BallSnapThreshold)
	{
		// Snap to server position if too far off
		SetActorLocation(ReplicatedLocation);
		SetActorRotation(ReplicatedRotation);
	}
	else
	{
		// Smooth interpolation
		FVector NewLocation = FMath::VInterpTo(CurrentLocation, ReplicatedLocation,
		                                       DeltaTime, SoccerConstants::BallInterpSpeed);
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), ReplicatedRotation,
		                                         DeltaTime, SoccerConstants::BallInterpSpeed);
		SetActorLocation(NewLocation);
		SetActorRotation(NewRotation);
	}
}

void ASoccerBall::UpdateReplicatedState()
{
	ReplicatedLocation = GetActorLocation();
	ReplicatedRotation = GetActorRotation();

	if (CollisionSphere->IsSimulatingPhysics())
	{
		ReplicatedVelocity = CollisionSphere->GetPhysicsLinearVelocity();
	}
	else
	{
		ReplicatedVelocity = FVector::ZeroVector;
	}
}

// ---- Rep Notifies ----

void ASoccerBall::OnRep_BallState()
{
	BP_OnBallStateChanged(BallState);

	// Toggle physics on client based on state
	if (BallState == EBallState::Possessed)
	{
		CollisionSphere->SetSimulatePhysics(false);
	}
}

// ---- Actions ----

void ASoccerBall::Kick(FVector Direction, float Force, ASoccerCharacter* Kicker)
{
	if (!HasAuthority()) return;

	// Clear possession
	ClearPossession();

	// Normalize direction and apply force
	Direction.Normalize();
	FVector KickImpulse = Direction * Force;

	// Add slight upward component for loft
	KickImpulse.Z += Force * 0.15f;

	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector); // Reset velocity first
	CollisionSphere->AddImpulse(KickImpulse, NAME_None, true);

	BallState = EBallState::InFlight;
	LastTouchedBy = Kicker;
	LastKickedBy = Kicker;

	if (Kicker)
	{
		// TODO: Get team from Kicker's player state
	}

	// Calculate normalized power for effects
	float NormalizedPower = FMath::Clamp(
		(Force - SoccerConstants::KickForceMin) /
		(SoccerConstants::KickForceMax - SoccerConstants::KickForceMin),
		0.0f, 1.0f);

	BP_OnKicked(NormalizedPower);
	Multicast_PlayKickEffect(GetActorLocation(), NormalizedPower);

	// Enable trail for powerful kicks
	if (NormalizedPower > 0.5f)
	{
		Multicast_PlayTrailEffect(true);
	}
}

void ASoccerBall::Pass(FVector Direction, float Force, ASoccerCharacter* Passer)
{
	if (!HasAuthority()) return;

	ClearPossession();

	Direction.Normalize();
	FVector PassImpulse = Direction * Force;
	// Passes stay on the ground
	PassImpulse.Z = FMath::Max(PassImpulse.Z, 0.0f);

	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->AddImpulse(PassImpulse, NAME_None, true);

	BallState = EBallState::InFlight;
	LastTouchedBy = Passer;
	LastKickedBy = Passer;

	Multicast_PlayKickEffect(GetActorLocation(), 0.3f);
}

void ASoccerBall::Header(FVector Direction, float Force, ASoccerCharacter* HeaderPlayer)
{
	if (!HasAuthority()) return;

	ClearPossession();

	Direction.Normalize();
	FVector HeaderImpulse = Direction * Force;

	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->AddImpulse(HeaderImpulse, NAME_None, true);

	BallState = EBallState::InFlight;
	LastTouchedBy = HeaderPlayer;

	Multicast_PlayKickEffect(GetActorLocation(), 0.4f);
}

void ASoccerBall::SetPossessedBy(ASoccerCharacter* Player)
{
	if (!HasAuthority() || Player == nullptr) return;

	PossessingPlayer = Player;
	BallState = EBallState::Possessed;
	LastTouchedBy = Player;

	// Disable physics while possessed
	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);

	BP_OnPossessionChanged(Player);
}

void ASoccerBall::ClearPossession()
{
	if (!HasAuthority()) return;

	ASoccerCharacter* PreviousPossessor = PossessingPlayer;
	PossessingPlayer = nullptr;
	BallState = EBallState::Free;

	// Re-enable physics
	CollisionSphere->SetSimulatePhysics(true);

	BP_OnPossessionChanged(nullptr);
}

void ASoccerBall::ResetToPosition(FVector NewLocation)
{
	if (!HasAuthority()) return;

	ClearPossession();

	CollisionSphere->SetSimulatePhysics(false);
	SetActorLocation(NewLocation + FVector(0.0f, 0.0f, SoccerConstants::BallRadius + 5.0f));
	SetActorRotation(FRotator::ZeroRotator);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);

	BallState = EBallState::Dead;

	// Update replicated state immediately
	UpdateReplicatedState();
}

void ASoccerBall::SetDead()
{
	if (!HasAuthority()) return;

	BallState = EBallState::Dead;
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void ASoccerBall::SetFree()
{
	if (!HasAuthority()) return;

	BallState = EBallState::Free;
	PossessingPlayer = nullptr;
	CollisionSphere->SetSimulatePhysics(true);
}

FVector ASoccerBall::GetBallVelocity() const
{
	if (CollisionSphere->IsSimulatingPhysics())
	{
		return CollisionSphere->GetPhysicsLinearVelocity();
	}
	return ReplicatedVelocity;
}

float ASoccerBall::GetBallSpeed() const
{
	return GetBallVelocity().Size();
}

// ---- Collision ----

void ASoccerBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, FVector NormalImpulse,
                            const FHitResult& Hit)
{
	if (!HasAuthority()) return;

	float ImpactForce = NormalImpulse.Size();

	// Play bounce effect for significant impacts
	if (ImpactForce > 100.0f)
	{
		BP_OnBounced(Hit.ImpactPoint);
		Multicast_PlayBounceEffect(Hit.ImpactPoint, ImpactForce);
	}

	// Disable trail when ball slows down after bounce
	if (GetBallSpeed() < 500.0f)
	{
		Multicast_PlayTrailEffect(false);
	}
}

// ---- Multicast RPCs ----

void ASoccerBall::Multicast_PlayKickEffect_Implementation(FVector Location, float NormalizedPower)
{
	BP_OnKicked(NormalizedPower);
}

void ASoccerBall::Multicast_PlayBounceEffect_Implementation(FVector Location, float ImpactForce)
{
	BP_OnBounced(Location);
}

void ASoccerBall::Multicast_PlayTrailEffect_Implementation(bool bEnable)
{
	// Blueprint handles actual trail VFX toggle
}
