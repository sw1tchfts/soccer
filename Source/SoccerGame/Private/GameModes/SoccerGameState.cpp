#include "GameModes/SoccerGameState.h"
#include "Player/SoccerPlayerState.h"
#include "Net/UnrealNetwork.h"

ASoccerGameState::ASoccerGameState()
{
	CurrentMatchPhase = EMatchPhase::WaitingForPlayers;
	HomeScore = 0;
	AwayScore = 0;
	MatchTimeRemaining = 300.0f;
	CurrentHalf = 1;
	LastGoalTeam = ETeamID::None;
	CurrentBallState = EBallState::Dead;
}

void ASoccerGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASoccerGameState, CurrentMatchPhase);
	DOREPLIFETIME(ASoccerGameState, HomeScore);
	DOREPLIFETIME(ASoccerGameState, AwayScore);
	DOREPLIFETIME(ASoccerGameState, MatchTimeRemaining);
	DOREPLIFETIME(ASoccerGameState, CurrentHalf);
	DOREPLIFETIME(ASoccerGameState, LastGoalTeam);
	DOREPLIFETIME(ASoccerGameState, CurrentBallState);
	DOREPLIFETIME(ASoccerGameState, MatchSettings);
	DOREPLIFETIME(ASoccerGameState, LastScorerName);
}

// ---- Rep Notifies ----

void ASoccerGameState::OnRep_MatchPhase()
{
	BP_OnPhaseChanged(CurrentMatchPhase);
}

void ASoccerGameState::OnRep_HomeScore()
{
	BP_OnScoreChanged(ETeamID::Home, HomeScore);
}

void ASoccerGameState::OnRep_AwayScore()
{
	BP_OnScoreChanged(ETeamID::Away, AwayScore);
}

// ---- Accessors ----

void ASoccerGameState::AddGoal(ETeamID Team)
{
	if (!HasAuthority()) return;

	if (Team == ETeamID::Home)
	{
		HomeScore++;
		OnRep_HomeScore();
	}
	else if (Team == ETeamID::Away)
	{
		AwayScore++;
		OnRep_AwayScore();
	}

	LastGoalTeam = Team;
}

int32 ASoccerGameState::GetScore(ETeamID Team) const
{
	if (Team == ETeamID::Home) return HomeScore;
	if (Team == ETeamID::Away) return AwayScore;
	return 0;
}

FString ASoccerGameState::GetMatchTimeFormatted() const
{
	int32 TotalSeconds = FMath::CeilToInt(MatchTimeRemaining);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

bool ASoccerGameState::IsMatchInProgress() const
{
	return CurrentMatchPhase == EMatchPhase::FirstHalf ||
	       CurrentMatchPhase == EMatchPhase::SecondHalf ||
	       CurrentMatchPhase == EMatchPhase::Overtime;
}

TArray<ASoccerPlayerState*> ASoccerGameState::GetPlayersOnTeam(ETeamID Team) const
{
	TArray<ASoccerPlayerState*> TeamPlayers;
	for (APlayerState* PS : PlayerArray)
	{
		ASoccerPlayerState* SPS = Cast<ASoccerPlayerState>(PS);
		if (SPS && SPS->TeamID == Team)
		{
			TeamPlayers.Add(SPS);
		}
	}
	return TeamPlayers;
}

int32 ASoccerGameState::GetPlayerCountOnTeam(ETeamID Team) const
{
	return GetPlayersOnTeam(Team).Num();
}

ETeamID ASoccerGameState::GetWinningTeam() const
{
	if (HomeScore > AwayScore) return ETeamID::Home;
	if (AwayScore > HomeScore) return ETeamID::Away;
	return ETeamID::None; // Draw
}
