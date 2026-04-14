#include "Player/SoccerPlayerController.h"
#include "Player/SoccerCharacter.h"
#include "Player/SoccerPlayerState.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"

ASoccerPlayerController::ASoccerPlayerController()
{
}

void ASoccerPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASoccerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput =
		CastChecked<UEnhancedInputComponent>(InputComponent);

	if (IA_Move)
	{
		EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this,
			&ASoccerPlayerController::HandleMove);
	}

	if (IA_Look)
	{
		EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this,
			&ASoccerPlayerController::HandleLook);
	}

	if (IA_Sprint)
	{
		EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleSprintStarted);
		EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Completed, this,
			&ASoccerPlayerController::HandleSprintCompleted);
	}

	if (IA_Kick)
	{
		EnhancedInput->BindAction(IA_Kick, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleKickStarted);
		EnhancedInput->BindAction(IA_Kick, ETriggerEvent::Completed, this,
			&ASoccerPlayerController::HandleKickCompleted);
	}

	if (IA_Tackle)
	{
		EnhancedInput->BindAction(IA_Tackle, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleTackle);
	}

	if (IA_Dive)
	{
		EnhancedInput->BindAction(IA_Dive, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleDive);
	}

	if (IA_Jump)
	{
		EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleJump);
	}

	if (IA_Scoreboard)
	{
		EnhancedInput->BindAction(IA_Scoreboard, ETriggerEvent::Started, this,
			&ASoccerPlayerController::HandleScoreboardPressed);
		EnhancedInput->BindAction(IA_Scoreboard, ETriggerEvent::Completed, this,
			&ASoccerPlayerController::HandleScoreboardReleased);
	}
}

void ASoccerPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsLocalController())
	{
		ShowMatchHUD();
	}
}

// ---- Input Handlers ----

void ASoccerPlayerController::HandleMove(const FInputActionValue& Value)
{
	FVector2D MoveValue = Value.Get<FVector2D>();
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (Character)
	{
		Character->MoveForward(MoveValue.Y);
		Character->MoveRight(MoveValue.X);
	}
}

void ASoccerPlayerController::HandleLook(const FInputActionValue& Value)
{
	FVector2D LookValue = Value.Get<FVector2D>();
	AddYawInput(LookValue.X);
	AddPitchInput(LookValue.Y);
}

void ASoccerPlayerController::HandleSprintStarted(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (Character)
	{
		Character->StartSprint();
	}
}

void ASoccerPlayerController::HandleSprintCompleted(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (Character)
	{
		Character->StopSprint();
	}
}

void ASoccerPlayerController::HandleKickStarted(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (Character)
	{
		Character->StartKickCharge();
	}
}

void ASoccerPlayerController::HandleKickCompleted(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	// Aim direction based on camera forward projected onto XY plane
	FVector CameraForward = Character->FollowCamera->GetForwardVector();
	CameraForward.Z = 0.0f;
	CameraForward.Normalize();

	float Power = Character->ReleaseKick(CameraForward);

	// If we're a client, also send to server
	if (!HasAuthority() && Power > 0.0f)
	{
		Server_RequestKick(CameraForward, Power);
	}
}

void ASoccerPlayerController::HandleTackle(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	FVector TackleDir = Character->GetActorForwardVector();

	if (HasAuthority())
	{
		Character->PerformTackle(TackleDir);
	}
	else
	{
		Server_RequestTackle(TackleDir);
	}
}

void ASoccerPlayerController::HandleDive(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	// Dive in the direction of current movement input
	FVector DiveDir = Character->GetLastMovementInputVector();
	if (DiveDir.IsNearlyZero())
	{
		DiveDir = Character->GetActorForwardVector();
	}

	if (HasAuthority())
	{
		Character->PerformGoalkeeperDive(DiveDir);
	}
	else
	{
		Server_RequestDive(DiveDir);
	}
}

void ASoccerPlayerController::HandleJump(const FInputActionValue& Value)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (Character)
	{
		Character->Jump();
	}
}

void ASoccerPlayerController::HandleScoreboardPressed(const FInputActionValue& Value)
{
	// Blueprint handles scoreboard visibility
}

void ASoccerPlayerController::HandleScoreboardReleased(const FInputActionValue& Value)
{
	// Blueprint handles scoreboard visibility
}

// ---- Server RPCs ----

void ASoccerPlayerController::Server_RequestKick_Implementation(FVector Direction, float Power)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	// Validate: does the player have the ball or is near it?
	if (!Character->IsInBallRange()) return;

	// Validate power is in valid range
	Power = FMath::Clamp(Power, 0.0f, 1.0f);

	float KickForce = FMath::Lerp(SoccerConstants::KickForceMin,
	                               SoccerConstants::KickForceMax, Power);

	Direction.Normalize();
	if (Character->NearbyBall)
	{
		Character->NearbyBall->Kick(Direction, KickForce, Character);
	}
}

bool ASoccerPlayerController::Server_RequestKick_Validate(FVector Direction, float Power)
{
	return Power >= 0.0f && Power <= 1.0f && !Direction.IsZero();
}

void ASoccerPlayerController::Server_RequestTackle_Implementation(FVector Direction)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	if (!Character->CanTackle()) return;

	Direction.Normalize();
	Character->PerformTackle(Direction);
}

bool ASoccerPlayerController::Server_RequestTackle_Validate(FVector Direction)
{
	return !Direction.IsZero();
}

void ASoccerPlayerController::Server_RequestDive_Implementation(FVector Direction)
{
	ASoccerCharacter* Character = GetSoccerCharacter();
	if (!Character) return;

	if (Character->Position != EPlayerPosition::Goalkeeper) return;

	Direction.Normalize();
	Character->PerformGoalkeeperDive(Direction);
}

bool ASoccerPlayerController::Server_RequestDive_Validate(FVector Direction)
{
	return !Direction.IsZero();
}

void ASoccerPlayerController::Server_SetReady_Implementation(bool bReady)
{
	ASoccerPlayerState* PS = GetSoccerPlayerState();
	if (PS)
	{
		PS->SetReady(bReady);
	}
}

// ---- Client RPCs ----

void ASoccerPlayerController::Client_ShowGoalNotification_Implementation(
	ETeamID ScoringTeam, const FString& ScorerName)
{
	BP_OnGoalNotification(ScoringTeam, ScorerName);
}

void ASoccerPlayerController::Client_ShowMatchPhaseNotification_Implementation(
	EMatchPhase NewPhase)
{
	BP_OnMatchPhaseNotification(NewPhase);
}

// ---- UI Management ----

void ASoccerPlayerController::ShowMatchHUD()
{
	if (!IsLocalController()) return;

	if (!MatchHUDWidget && MatchHUDWidgetClass)
	{
		MatchHUDWidget = CreateWidget<UUserWidget>(this, MatchHUDWidgetClass);
		if (MatchHUDWidget)
		{
			MatchHUDWidget->AddToViewport();
			BP_OnMatchHUDCreated(MatchHUDWidget);
		}
	}
	else if (MatchHUDWidget)
	{
		MatchHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ASoccerPlayerController::HideMatchHUD()
{
	if (MatchHUDWidget)
	{
		MatchHUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ASoccerPlayerController::TogglePauseMenu()
{
	// Blueprint handles pause menu creation and toggle
}

// ---- Helpers ----

ASoccerPlayerState* ASoccerPlayerController::GetSoccerPlayerState() const
{
	return GetPlayerState<ASoccerPlayerState>();
}

ASoccerCharacter* ASoccerPlayerController::GetSoccerCharacter() const
{
	return Cast<ASoccerCharacter>(GetPawn());
}
