#include "GameModes/SoccerGameMode.h"
#include "GameModes/SoccerGameState.h"
#include "Player/SoccerPlayerController.h"
#include "Player/SoccerPlayerState.h"
#include "Player/SoccerCharacter.h"
#include "Ball/SoccerBall.h"
#include "Field/SoccerGoal.h"
#include "Field/SoccerPitchManager.h"
#include "AI/SoccerAIController.h"
#include "UI/SoccerHUD.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ASoccerGameMode::ASoccerGameMode()
{
	DefaultPawnClass = ASoccerCharacter::StaticClass();
	PlayerControllerClass = ASoccerPlayerController::StaticClass();
	PlayerStateClass = ASoccerPlayerState::StaticClass();
	GameStateClass = ASoccerGameState::StaticClass();
	HUDClass = ASoccerHUD::StaticClass();

	CountdownValue = 3;
	NextJerseyNumberHome = 1;
	NextJerseyNumberAway = 1;
}

// ---- Overrides ----

void ASoccerGameMode::InitGame(const FString& MapName, const FString& Options,
                               FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FindFieldActors();
}

void ASoccerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Assign team
	ETeamID AssignedTeam = AssignTeam(NewPlayer);

	ASoccerPlayerState* PS = NewPlayer->GetPlayerState<ASoccerPlayerState>();
	if (PS)
	{
		PS->SetTeam(AssignedTeam);
		AssignPosition(PS);

		// Assign jersey number
		if (AssignedTeam == ETeamID::Home)
		{
			PS->JerseyNumber = NextJerseyNumberHome++;
		}
		else
		{
			PS->JerseyNumber = NextJerseyNumberAway++;
		}
	}
}

void ASoccerGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// Could replace with bot here if match is in progress
	ASoccerGameState* GS = GetSoccerGameState();
	if (GS && GS->IsMatchInProgress())
	{
		// TODO: Spawn bot to replace disconnected player
	}
}

UClass* ASoccerGameMode::GetDefaultPawnClassForController_Implementation(
	AController* InController)
{
	return DefaultPawnClass;
}

AActor* ASoccerGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Use default player start selection
	return Super::ChoosePlayerStart_Implementation(Player);
}

// ---- Match Flow ----

void ASoccerGameMode::StartMatch()
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	// Initialize game state
	GS->HomeScore = 0;
	GS->AwayScore = 0;
	GS->CurrentHalf = 1;
	GS->MatchTimeRemaining = MatchSettings.HalfDurationSeconds;
	GS->MatchSettings = MatchSettings;

	// Fill empty slots with bots
	FillEmptySlotsWithBots();

	// Spawn the ball at center
	SpawnBall(PitchManager ? PitchManager->CenterSpot + FVector(0.0f, 0.0f, 50.0f)
	                       : FVector(0.0f, 0.0f, 50.0f));

	// Reset positions and start countdown
	ResetAllPlayerPositions();
	StartCountdown();
}

void ASoccerGameMode::StartCountdown()
{
	TransitionToPhase(EMatchPhase::Countdown);
	FreezeAllPlayers(true);

	CountdownValue = FMath::CeilToInt(MatchSettings.CountdownDurationSeconds);

	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this,
		&ASoccerGameMode::TickCountdown, 1.0f, true);
}

void ASoccerGameMode::TickCountdown()
{
	CountdownValue--;
	BP_OnCountdownTick(CountdownValue);

	if (CountdownValue <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);

		ASoccerGameState* GS = GetSoccerGameState();
		if (GS)
		{
			if (GS->CurrentHalf == 1)
			{
				TransitionToPhase(EMatchPhase::FirstHalf);
			}
			else
			{
				TransitionToPhase(EMatchPhase::SecondHalf);
			}
		}

		FreezeAllPlayers(false);

		// Start match timer
		GetWorldTimerManager().SetTimer(MatchTimerHandle, this,
			&ASoccerGameMode::TickMatchTimer, 1.0f, true);

		// Set ball free
		if (MatchBall)
		{
			MatchBall->SetFree();
		}
	}
}

void ASoccerGameMode::StartKickoff(ETeamID KickingTeam)
{
	ResetAllPlayerPositions();
	ResetBallToCenter();
	StartCountdown();
}

void ASoccerGameMode::OnGoalScored(ETeamID ScoringTeam, ASoccerCharacter* Scorer,
                                   ASoccerCharacter* Assister)
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	// Stop the match timer
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	// Update score
	GS->AddGoal(ScoringTeam);

	// Track scorer stats
	FString ScorerName = TEXT("Unknown");
	if (Scorer)
	{
		ASoccerPlayerState* ScorerPS = Scorer->GetPlayerState<ASoccerPlayerState>();
		if (ScorerPS)
		{
			ScorerPS->AddGoal();
			ScorerName = ScorerPS->GetPlayerName();
		}
	}

	// Track assist
	if (Assister)
	{
		ASoccerPlayerState* AssisterPS = Assister->GetPlayerState<ASoccerPlayerState>();
		if (AssisterPS)
		{
			AssisterPS->AddAssist();
		}
	}

	GS->LastScorerName = ScorerName;

	// Freeze players during celebration
	FreezeAllPlayers(true);

	// Notify clients
	NotifyAllClients_GoalScored(ScoringTeam, ScorerName);
	BP_OnGoalScored(ScoringTeam);

	// Reset goals
	if (HomeGoal) HomeGoal->ResetGoal();
	if (AwayGoal) AwayGoal->ResetGoal();

	// Start celebration timer
	GetWorldTimerManager().SetTimer(GoalCelebrationHandle, this,
		&ASoccerGameMode::OnGoalCelebrationFinished,
		MatchSettings.GoalCelebrationDurationSeconds, false);
}

void ASoccerGameMode::OnGoalCelebrationFinished()
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	// Check if match time has expired
	if (GS->MatchTimeRemaining <= 0.0f)
	{
		if (GS->CurrentHalf == 1)
		{
			TransitionToPhase(EMatchPhase::Halftime);
			return;
		}
		else
		{
			// Check for overtime
			if (MatchSettings.bOvertimeEnabled && GS->GetWinningTeam() == ETeamID::None)
			{
				TransitionToPhase(EMatchPhase::Overtime);
				GS->MatchTimeRemaining = MatchSettings.OvertimeDurationSeconds;
			}
			else
			{
				EndMatch();
				return;
			}
		}
	}

	// Non-scoring team kicks off
	ETeamID KickingTeam = (GS->LastGoalTeam == ETeamID::Home) ? ETeamID::Away : ETeamID::Home;
	StartKickoff(KickingTeam);
}

void ASoccerGameMode::TransitionToPhase(EMatchPhase NewPhase)
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	GS->CurrentMatchPhase = NewPhase;
	GS->OnRep_MatchPhase(); // Force local notification on server

	NotifyAllClients_PhaseChanged(NewPhase);
	BP_OnMatchPhaseChanged(NewPhase);

	switch (NewPhase)
	{
	case EMatchPhase::Halftime:
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		FreezeAllPlayers(true);
		GetWorldTimerManager().SetTimer(HalftimeTimerHandle, this,
			&ASoccerGameMode::OnHalftimeFinished,
			MatchSettings.HalftimeDurationSeconds, false);
		break;

	case EMatchPhase::PostMatch:
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		FreezeAllPlayers(true);
		break;

	default:
		break;
	}
}

void ASoccerGameMode::OnHalftimeFinished()
{
	StartSecondHalf();
}

void ASoccerGameMode::StartSecondHalf()
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	GS->CurrentHalf = 2;
	GS->MatchTimeRemaining = MatchSettings.HalfDurationSeconds;

	// Away team kicks off second half
	StartKickoff(ETeamID::Away);
}

void ASoccerGameMode::TickMatchTimer()
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	GS->MatchTimeRemaining -= 1.0f;

	if (GS->MatchTimeRemaining <= 0.0f)
	{
		GS->MatchTimeRemaining = 0.0f;
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);

		if (GS->CurrentHalf == 1)
		{
			TransitionToPhase(EMatchPhase::Halftime);
		}
		else if (GS->CurrentMatchPhase == EMatchPhase::Overtime)
		{
			EndMatch();
		}
		else
		{
			// End of second half
			if (MatchSettings.bOvertimeEnabled && GS->GetWinningTeam() == ETeamID::None)
			{
				// Go to overtime
				GS->MatchTimeRemaining = MatchSettings.OvertimeDurationSeconds;
				TransitionToPhase(EMatchPhase::Overtime);
				StartKickoff(ETeamID::Home);
			}
			else
			{
				EndMatch();
			}
		}
	}
}

void ASoccerGameMode::EndMatch()
{
	TransitionToPhase(EMatchPhase::PostMatch);

	ASoccerGameState* GS = GetSoccerGameState();
	if (GS)
	{
		ETeamID Winner = GS->GetWinningTeam();
		BP_OnMatchEnded(Winner);
	}

	if (MatchBall)
	{
		MatchBall->SetDead();
	}
}

// ---- Team Management ----

ETeamID ASoccerGameMode::AssignTeam(APlayerController* Player)
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return ETeamID::Home;

	int32 HomeCount = GS->GetPlayerCountOnTeam(ETeamID::Home);
	int32 AwayCount = GS->GetPlayerCountOnTeam(ETeamID::Away);

	// Assign to team with fewer players
	if (HomeCount <= AwayCount)
	{
		return ETeamID::Home;
	}
	return ETeamID::Away;
}

void ASoccerGameMode::AssignPosition(ASoccerPlayerState* PlayerState)
{
	if (!PlayerState) return;

	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	TArray<ASoccerPlayerState*> TeamPlayers = GS->GetPlayersOnTeam(PlayerState->TeamID);
	int32 PlayerIndex = TeamPlayers.Num() - 1; // This player is last added

	// 1-1-2-1 formation: GK, DEF, MID, MID, FWD
	switch (PlayerIndex)
	{
	case 0:
		PlayerState->SetPosition(EPlayerPosition::Goalkeeper);
		break;
	case 1:
		PlayerState->SetPosition(EPlayerPosition::Defender);
		break;
	case 2:
	case 3:
		PlayerState->SetPosition(EPlayerPosition::Midfielder);
		break;
	case 4:
	default:
		PlayerState->SetPosition(EPlayerPosition::Forward);
		break;
	}
}

void ASoccerGameMode::FillEmptySlotsWithBots()
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	int32 TargetPerTeam = MatchSettings.PlayersPerTeam;

	// Fill Home team
	int32 HomeCount = GS->GetPlayerCountOnTeam(ETeamID::Home);
	for (int32 i = HomeCount; i < TargetPerTeam; i++)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ASoccerAIController* BotController = GetWorld()->SpawnActor<ASoccerAIController>(
			BotControllerClass ? BotControllerClass.Get() : ASoccerAIController::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (BotController)
		{
			RestartPlayer(BotController);

			ASoccerPlayerState* BotPS = BotController->GetPlayerState<ASoccerPlayerState>();
			if (BotPS)
			{
				BotPS->bIsBot = true;
				BotPS->SetTeam(ETeamID::Home);
				BotPS->JerseyNumber = NextJerseyNumberHome++;
				AssignPosition(BotPS);
				BotPS->SetPlayerName(FString::Printf(TEXT("Bot %d"), BotPS->JerseyNumber));
			}
		}
	}

	// Fill Away team
	int32 AwayCount = GS->GetPlayerCountOnTeam(ETeamID::Away);
	for (int32 i = AwayCount; i < TargetPerTeam; i++)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ASoccerAIController* BotController = GetWorld()->SpawnActor<ASoccerAIController>(
			BotControllerClass ? BotControllerClass.Get() : ASoccerAIController::StaticClass(),
			FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (BotController)
		{
			RestartPlayer(BotController);

			ASoccerPlayerState* BotPS = BotController->GetPlayerState<ASoccerPlayerState>();
			if (BotPS)
			{
				BotPS->bIsBot = true;
				BotPS->SetTeam(ETeamID::Away);
				BotPS->JerseyNumber = NextJerseyNumberAway++;
				AssignPosition(BotPS);
				BotPS->SetPlayerName(FString::Printf(TEXT("Bot %d"), BotPS->JerseyNumber));
			}
		}
	}
}

void ASoccerGameMode::RemoveBot(ETeamID Team)
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	TArray<ASoccerPlayerState*> TeamPlayers = GS->GetPlayersOnTeam(Team);
	for (ASoccerPlayerState* PS : TeamPlayers)
	{
		if (PS && PS->bIsBot)
		{
			AController* Controller = Cast<AController>(PS->GetOwner());
			if (Controller)
			{
				APawn* Pawn = Controller->GetPawn();
				if (Pawn)
				{
					Pawn->Destroy();
				}
				Controller->Destroy();
			}
			return; // Remove one at a time
		}
	}
}

// ---- Ball Management ----

void ASoccerGameMode::SpawnBall(FVector Location)
{
	if (MatchBall)
	{
		MatchBall->ResetToPosition(Location);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* SpawnClass = BallClass ? BallClass.Get() : ASoccerBall::StaticClass();
	MatchBall = GetWorld()->SpawnActor<ASoccerBall>(SpawnClass, Location,
	                                                 FRotator::ZeroRotator, SpawnParams);
}

void ASoccerGameMode::ResetBallToCenter()
{
	FVector CenterPos = PitchManager ?
		PitchManager->CenterSpot + FVector(0.0f, 0.0f, 50.0f) :
		FVector(0.0f, 0.0f, 50.0f);

	if (MatchBall)
	{
		MatchBall->ResetToPosition(CenterPos);
	}
	else
	{
		SpawnBall(CenterPos);
	}
}

// ---- Player Management ----

void ASoccerGameMode::ResetAllPlayerPositions()
{
	if (!PitchManager) return;

	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return;

	// Reset Home team
	TArray<ASoccerPlayerState*> HomePlayers = GS->GetPlayersOnTeam(ETeamID::Home);
	TArray<FVector> HomePositions = PitchManager->GetKickoffPositions(ETeamID::Home);
	for (int32 i = 0; i < HomePlayers.Num() && i < HomePositions.Num(); i++)
	{
		AController* Controller = Cast<AController>(HomePlayers[i]->GetOwner());
		if (Controller && Controller->GetPawn())
		{
			Controller->GetPawn()->SetActorLocation(
				HomePositions[i] + FVector(0.0f, 0.0f, 100.0f));
		}
	}

	// Reset Away team
	TArray<ASoccerPlayerState*> AwayPlayers = GS->GetPlayersOnTeam(ETeamID::Away);
	TArray<FVector> AwayPositions = PitchManager->GetKickoffPositions(ETeamID::Away);
	for (int32 i = 0; i < AwayPlayers.Num() && i < AwayPositions.Num(); i++)
	{
		AController* Controller = Cast<AController>(AwayPlayers[i]->GetOwner());
		if (Controller && Controller->GetPawn())
		{
			Controller->GetPawn()->SetActorLocation(
				AwayPositions[i] + FVector(0.0f, 0.0f, 100.0f));
		}
	}
}

void ASoccerGameMode::FreezeAllPlayers(bool bFreeze)
{
	for (TActorIterator<ASoccerCharacter> It(GetWorld()); It; ++It)
	{
		ASoccerCharacter* Character = *It;
		if (Character)
		{
			if (bFreeze)
			{
				Character->GetCharacterMovement()->DisableMovement();
			}
			else
			{
				Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
			}
		}
	}
}

// ---- Helpers ----

ASoccerGameState* ASoccerGameMode::GetSoccerGameState() const
{
	return GetGameState<ASoccerGameState>();
}

int32 ASoccerGameMode::GetTotalPlayerCount() const
{
	ASoccerGameState* GS = GetSoccerGameState();
	return GS ? GS->PlayerArray.Num() : 0;
}

bool ASoccerGameMode::AreTeamsFull() const
{
	ASoccerGameState* GS = GetSoccerGameState();
	if (!GS) return false;

	return GS->GetPlayerCountOnTeam(ETeamID::Home) >= MatchSettings.PlayersPerTeam &&
	       GS->GetPlayerCountOnTeam(ETeamID::Away) >= MatchSettings.PlayersPerTeam;
}

// ---- Private ----

void ASoccerGameMode::FindFieldActors()
{
	// Find PitchManager in the level
	for (TActorIterator<ASoccerPitchManager> It(GetWorld()); It; ++It)
	{
		PitchManager = *It;
		break;
	}

	// Find goals
	for (TActorIterator<ASoccerGoal> It(GetWorld()); It; ++It)
	{
		ASoccerGoal* Goal = *It;
		if (Goal->DefendingTeam == ETeamID::Home)
		{
			HomeGoal = Goal;
		}
		else if (Goal->DefendingTeam == ETeamID::Away)
		{
			AwayGoal = Goal;
		}
	}

	// Link goals to pitch manager
	if (PitchManager)
	{
		PitchManager->HomeGoal = HomeGoal;
		PitchManager->AwayGoal = AwayGoal;
	}
}

void ASoccerGameMode::NotifyAllClients_GoalScored(ETeamID ScoringTeam,
                                                   const FString& ScorerName)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator();
	     It; ++It)
	{
		ASoccerPlayerController* PC = Cast<ASoccerPlayerController>(It->Get());
		if (PC)
		{
			PC->Client_ShowGoalNotification(ScoringTeam, ScorerName);
		}
	}
}

void ASoccerGameMode::NotifyAllClients_PhaseChanged(EMatchPhase NewPhase)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator();
	     It; ++It)
	{
		ASoccerPlayerController* PC = Cast<ASoccerPlayerController>(It->Get());
		if (PC)
		{
			PC->Client_ShowMatchPhaseNotification(NewPhase);
		}
	}
}
