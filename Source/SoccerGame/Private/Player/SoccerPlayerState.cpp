#include "Player/SoccerPlayerState.h"
#include "Net/UnrealNetwork.h"

ASoccerPlayerState::ASoccerPlayerState()
{
	TeamID = ETeamID::None;
	Position = EPlayerPosition::Midfielder;
	bIsBot = false;
	JerseyNumber = 0;
	bIsReady = false;
}

void ASoccerPlayerState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASoccerPlayerState, TeamID);
	DOREPLIFETIME(ASoccerPlayerState, Position);
	DOREPLIFETIME(ASoccerPlayerState, MatchStats);
	DOREPLIFETIME(ASoccerPlayerState, bIsBot);
	DOREPLIFETIME(ASoccerPlayerState, JerseyNumber);
	DOREPLIFETIME(ASoccerPlayerState, bIsReady);
}

void ASoccerPlayerState::OnRep_TeamID()
{
	BP_OnTeamAssigned(TeamID);
}

// ---- Stat Tracking ----

void ASoccerPlayerState::AddGoal()
{
	if (HasAuthority())
	{
		MatchStats.Goals++;
		BP_OnGoalScored();
	}
}

void ASoccerPlayerState::AddAssist()
{
	if (HasAuthority())
	{
		MatchStats.Assists++;
		BP_OnAssistRecorded();
	}
}

void ASoccerPlayerState::AddSave()
{
	if (HasAuthority())
	{
		MatchStats.Saves++;
	}
}

void ASoccerPlayerState::AddTackle()
{
	if (HasAuthority())
	{
		MatchStats.Tackles++;
	}
}

void ASoccerPlayerState::AddPass()
{
	if (HasAuthority())
	{
		MatchStats.Passes++;
	}
}

void ASoccerPlayerState::AddShot(bool bOnTarget)
{
	if (HasAuthority())
	{
		MatchStats.Shots++;
		if (bOnTarget)
		{
			MatchStats.ShotsOnTarget++;
		}
	}
}

void ASoccerPlayerState::ResetStats()
{
	if (HasAuthority())
	{
		MatchStats = FPlayerMatchStats();
	}
}

// ---- Team Management ----

void ASoccerPlayerState::SetTeam(ETeamID NewTeam)
{
	if (HasAuthority())
	{
		TeamID = NewTeam;
		OnRep_TeamID();
	}
}

void ASoccerPlayerState::SetPosition(EPlayerPosition NewPosition)
{
	if (HasAuthority())
	{
		Position = NewPosition;
	}
}

void ASoccerPlayerState::SetReady(bool bNewReady)
{
	if (HasAuthority())
	{
		bIsReady = bNewReady;
	}
}
