// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABlasterPlayerState, Defeats);
	DOREPLIFETIME(ABlasterPlayerState, IsVisibleElimText);
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetHUDScore(Score);
		}
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::OnRep_ElimText()
{
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetElimTextVisibility(IsVisibleElimText);
		}
	}
}

void ABlasterPlayerState::AddToScore(float ScoreAmount, bool IsInit)
{
	SetScore(Score += ScoreAmount);

	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{			
		Controller = !Controller ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{					
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = !Controller ? Cast <ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::ElimTextVisibility(bool IsVisible)
{
	Character = !Character ? Cast<ABlasterCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = !Controller ? Cast <ABlasterPlayerController>(Character->GetController()) : Controller;
		if (Controller)
		{
			Controller->SetElimTextVisibility(IsVisible);
			IsVisibleElimText = IsVisible;
		}
	}
}