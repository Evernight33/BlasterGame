// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountDownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

		if (CountDownTime <= 0.0f)
		{
			StartMatch();
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.0f);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
		VictimPlayerState->ElimTextVisibility(true);
	}
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Eliminate();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();
		EliminatedCharacter->Destroy();
	}

	if (EliminatedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);

		if (EliminatedCharacter->GetPlayerState())
		{
			ABlasterPlayerState* EliminatedCharPlayerState = Cast<ABlasterPlayerState>(EliminatedCharacter->GetPlayerState());
			if (EliminatedCharPlayerState)
			{
				EliminatedCharPlayerState->ElimTextVisibility(false);
			}
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; It++)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It);
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}