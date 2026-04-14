#include "Player/SoccerCharacter.h"
#include "Ball/SoccerBall.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

ASoccerCharacter::ASoccerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Ball detection sphere
	BallDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BallDetection"));
	BallDetectionSphere->SetupAttachment(RootComponent);
	BallDetectionSphere->SetSphereRadius(SoccerConstants::BallPossessionRadius);
	BallDetectionSphere->SetCollisionProfileName(TEXT("OverlapAll"));
	BallDetectionSphere->SetGenerateOverlapEvents(true);
	BallDetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Ball carry socket (at feet, slightly forward)
	BallCarrySocket = CreateDefaultSubobject<USceneComponent>(TEXT("BallCarrySocket"));
	BallCarrySocket->SetupAttachment(RootComponent);
	BallCarrySocket->SetRelativeLocation(FVector(80.0f, 0.0f, -80.0f));

	// Character movement config
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.2f;
	GetCharacterMovement()->MaxAcceleration = 2048.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;

	// Don't rotate character to match controller
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Defaults
	bIsSprinting = false;
	bHasBall = false;
	bIsTackling = false;
	bIsDiving = false;
	bIsKicking = false;
	bIsChargingKick = false;
	bStaminaLockedOut = false;
	CurrentStamina = SoccerConstants::MaxStamina;
	KickChargeProgress = 0.0f;
	KickChargeStartTime = 0.0f;
	LastTackleTime = -SoccerConstants::TackleCooldown;
	TeamID = ETeamID::None;
	Position = EPlayerPosition::Midfielder;
	NearbyBall = nullptr;
}

void ASoccerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BallDetectionSphere->OnComponentBeginOverlap.AddDynamic(
		this, &ASoccerCharacter::OnBallDetectionOverlapBegin);
	BallDetectionSphere->OnComponentEndOverlap.AddDynamic(
		this, &ASoccerCharacter::OnBallDetectionOverlapEnd);
}

void ASoccerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateStamina(DeltaTime);
	UpdateSprintSpeed();

	// Update kick charge progress
	if (bIsChargingKick)
	{
		float ChargeTime = GetWorld()->GetTimeSeconds() - KickChargeStartTime;
		KickChargeProgress = FMath::Clamp(ChargeTime / MaxKickChargeTime, 0.0f, 1.0f);
		BP_OnKickCharged(KickChargeProgress);
	}
}

void ASoccerCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASoccerCharacter, bIsSprinting);
	DOREPLIFETIME(ASoccerCharacter, bHasBall);
	DOREPLIFETIME(ASoccerCharacter, bIsTackling);
	DOREPLIFETIME(ASoccerCharacter, bIsDiving);
	DOREPLIFETIME(ASoccerCharacter, bIsKicking);
	DOREPLIFETIME(ASoccerCharacter, TeamID);
	DOREPLIFETIME(ASoccerCharacter, Position);
	DOREPLIFETIME_CONDITION(ASoccerCharacter, CurrentStamina, COND_OwnerOnly);
}

void ASoccerCharacter::UpdateStamina(float DeltaTime)
{
	if (!HasAuthority()) return;

	if (bIsSprinting)
	{
		CurrentStamina -= SprintStaminaCostPerSecond * DeltaTime;
		if (CurrentStamina <= 0.0f)
		{
			CurrentStamina = 0.0f;
			StopSprint();
			bStaminaLockedOut = true;
		}
	}
	else
	{
		CurrentStamina = FMath::Min(CurrentStamina + StaminaRegenPerSecond * DeltaTime, MaxStamina);
		if (bStaminaLockedOut && CurrentStamina >= SoccerConstants::StaminaReenterThreshold)
		{
			bStaminaLockedOut = false;
		}
	}

	BP_OnStaminaChanged(CurrentStamina, MaxStamina);
}

void ASoccerCharacter::UpdateSprintSpeed()
{
	float TargetSpeed = bIsSprinting ? SprintSpeed : BaseWalkSpeed;

	// Slow down slightly when carrying ball
	if (bHasBall)
	{
		TargetSpeed *= SoccerConstants::DribbleSpeedPenalty;
	}

	GetCharacterMovement()->MaxWalkSpeed = TargetSpeed;
}

// ---- Rep Notifies ----

void ASoccerCharacter::OnRep_IsSprinting()
{
	Multicast_PlaySprintEffect(bIsSprinting);
}

void ASoccerCharacter::OnRep_CurrentStamina()
{
	BP_OnStaminaChanged(CurrentStamina, MaxStamina);
}

// ---- Movement Actions ----

void ASoccerCharacter::StartSprint()
{
	if (!CanSprint()) return;

	bIsSprinting = true;
	UpdateSprintSpeed();
	Multicast_PlaySprintEffect(true);
}

void ASoccerCharacter::StopSprint()
{
	bIsSprinting = false;
	UpdateSprintSpeed();
	Multicast_PlaySprintEffect(false);
}

void ASoccerCharacter::MoveForward(float Value)
{
	if (bIsTackling || bIsDiving) return;

	if (Controller && Value != 0.0f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ASoccerCharacter::MoveRight(float Value)
{
	if (bIsTackling || bIsDiving) return;

	if (Controller && Value != 0.0f)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

// ---- Ball Actions ----

void ASoccerCharacter::StartKickCharge()
{
	if (!CanKick()) return;

	bIsChargingKick = true;
	KickChargeStartTime = GetWorld()->GetTimeSeconds();
	KickChargeProgress = 0.0f;
}

float ASoccerCharacter::ReleaseKick(FVector AimDirection)
{
	if (!bIsChargingKick) return 0.0f;

	float ChargeTime = GetWorld()->GetTimeSeconds() - KickChargeStartTime;
	bIsChargingKick = false;
	bIsKicking = true;

	float NormalizedPower = FMath::Clamp(ChargeTime / MaxKickChargeTime, 0.0f, 1.0f);

	if (ChargeTime < SoccerConstants::TapKickThreshold)
	{
		// Tap = pass
		if (NearbyBall && HasAuthority())
		{
			AimDirection.Normalize();
			NearbyBall->Pass(AimDirection, SoccerConstants::PassForce, this);
		}
	}
	else
	{
		// Hold = power shot
		float KickForce = FMath::Lerp(SoccerConstants::KickForceMin,
		                               SoccerConstants::KickForceMax,
		                               NormalizedPower);
		if (NearbyBall && HasAuthority())
		{
			AimDirection.Normalize();
			NearbyBall->Kick(AimDirection, KickForce, this);
		}
	}

	Multicast_PlayKickEffect(GetActorLocation());
	BP_OnKickReleased(NormalizedPower);

	// Reset kick state after brief delay
	FTimerHandle KickResetTimer;
	GetWorldTimerManager().SetTimer(KickResetTimer, [this]()
	{
		bIsKicking = false;
	}, 0.4f, false);

	KickChargeProgress = 0.0f;
	return NormalizedPower;
}

void ASoccerCharacter::PerformPass(FVector TargetDirection)
{
	if (!CanKick()) return;

	if (NearbyBall && HasAuthority())
	{
		TargetDirection.Normalize();
		NearbyBall->Pass(TargetDirection, SoccerConstants::PassForce, this);
		LoseBallPossession();
	}

	Multicast_PlayKickEffect(GetActorLocation());
}

void ASoccerCharacter::PerformHeader()
{
	if (!NearbyBall || !HasAuthority()) return;

	FVector Direction = GetActorForwardVector();
	NearbyBall->Header(Direction, SoccerConstants::HeaderForce, this);

	Multicast_PlayKickEffect(GetActorLocation() + FVector(0.0f, 0.0f, 150.0f));
}

// ---- Combat Actions ----

void ASoccerCharacter::PerformTackle(FVector Direction)
{
	if (!CanTackle()) return;

	bIsTackling = true;
	LastTackleTime = GetWorld()->GetTimeSeconds();

	Direction.Normalize();
	LaunchCharacter(Direction * TackleDistance / SoccerConstants::TackleDuration,
	                true, true);

	BP_OnTacklePerformed();
	Multicast_PlayTackleEffect(GetActorLocation());

	GetWorldTimerManager().SetTimer(TackleTimerHandle, this,
		&ASoccerCharacter::FinishTackle, SoccerConstants::TackleDuration, false);
}

void ASoccerCharacter::FinishTackle()
{
	bIsTackling = false;
}

void ASoccerCharacter::PerformGoalkeeperDive(FVector Direction)
{
	if (Position != EPlayerPosition::Goalkeeper) return;
	if (bIsDiving) return;

	bIsDiving = true;
	Direction.Normalize();

	LaunchCharacter(Direction * DiveDistance / 0.5f + FVector(0.0f, 0.0f, 200.0f),
	                true, true);

	BP_OnDivePerformed(Direction);

	GetWorldTimerManager().SetTimer(DiveTimerHandle, this,
		&ASoccerCharacter::FinishDive, 0.8f, false);
}

void ASoccerCharacter::FinishDive()
{
	bIsDiving = false;
}

// ---- Ball Possession ----

void ASoccerCharacter::GainBallPossession(ASoccerBall* Ball)
{
	if (!HasAuthority() || !Ball) return;

	bHasBall = true;
	NearbyBall = Ball;
	Ball->SetPossessedBy(this);

	BP_OnBallPossessionGained();
}

void ASoccerCharacter::LoseBallPossession()
{
	if (!HasAuthority()) return;

	bHasBall = false;

	if (NearbyBall)
	{
		NearbyBall->ClearPossession();
	}

	BP_OnBallPossessionLost();
}

bool ASoccerCharacter::IsInBallRange() const
{
	return NearbyBall != nullptr;
}

bool ASoccerCharacter::CanKick() const
{
	return IsInBallRange() && !bIsTackling && !bIsDiving;
}

bool ASoccerCharacter::CanTackle() const
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return !bIsTackling && !bIsDiving && !bIsKicking &&
	       (CurrentTime - LastTackleTime) >= TackleCooldownTime;
}

bool ASoccerCharacter::CanSprint() const
{
	return !bStaminaLockedOut && CurrentStamina > SoccerConstants::StaminaMinToSprint &&
	       !bIsTackling && !bIsDiving;
}

// ---- Overlap Callbacks ----

void ASoccerCharacter::OnBallDetectionOverlapBegin(UPrimitiveComponent* OverlappedComp,
                                                    AActor* OtherActor,
                                                    UPrimitiveComponent* OtherComp,
                                                    int32 OtherBodyIndex, bool bFromSweep,
                                                    const FHitResult& SweepResult)
{
	ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
	if (!Ball) return;

	NearbyBall = Ball;

	// Auto-possess if ball is free and on the server
	if (HasAuthority() && Ball->BallState == EBallState::Free)
	{
		GainBallPossession(Ball);
	}
}

void ASoccerCharacter::OnBallDetectionOverlapEnd(UPrimitiveComponent* OverlappedComp,
                                                  AActor* OtherActor,
                                                  UPrimitiveComponent* OtherComp,
                                                  int32 OtherBodyIndex)
{
	ASoccerBall* Ball = Cast<ASoccerBall>(OtherActor);
	if (Ball && Ball == NearbyBall)
	{
		if (bHasBall && HasAuthority())
		{
			LoseBallPossession();
		}
		NearbyBall = nullptr;
	}
}

// ---- Multicast RPCs ----

void ASoccerCharacter::Multicast_PlayKickEffect_Implementation(FVector Location)
{
	// Blueprint handles actual VFX
}

void ASoccerCharacter::Multicast_PlayTackleEffect_Implementation(FVector Location)
{
	// Blueprint handles actual VFX
}

void ASoccerCharacter::Multicast_PlaySprintEffect_Implementation(bool bEnable)
{
	// Blueprint handles sprint trail VFX
}
