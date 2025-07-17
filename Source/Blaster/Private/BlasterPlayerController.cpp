// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/Announcement.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Public/GameMode/BlasterGameMode.h"
#include "Blaster/Public/PlayerState/BlasterPlayerState.h"
#include <Kismet/GameplayStatics.h>
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/GameState/BlasterGameState.h"
#include "TimerManager.h"

void ABlasterPlayerController::BeginPlay()
{ 
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.0f;
	}
}

void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::SetHUDHealth(float CurrentHealth, float MaxHealth)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay 
		&& BlasterHUD->CharacterOverlay->HealthBar
		&& BlasterHUD->CharacterOverlay->HealthText)
	{
		const float HealthPercent = CurrentHealth / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(CurrentHealth), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}
PRAGMA_DISABLE_OPTIMIZATION
void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ShieldBar
		&& BlasterHUD->CharacterOverlay->ShieldText)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		BlasterHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ScoreAmount)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->DefeatsAmount)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountDown(float CountdownTime)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->Announcement
		&& BlasterHUD->Announcement->WarmupTime)
	{
		if (CountdownTime < 0.0f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
		}
		else
		{
			int32 Mins = FMath::FloorToInt(CountdownTime / 60.0f);
			int32 Secs = FMath::FloorToInt(CountdownTime) % 60;

			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Mins, Secs);
			BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
		}		
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = !BlasterHUD ? Cast <ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->CarryAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarryAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = !BlasterHUD ? Cast <ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->GrenadesText)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
}

void ABlasterPlayerController::SetElimTextVisibility(bool IsVisible)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->ElimText)
	{
		if (IsVisible)
		{
			BlasterHUD->CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			BlasterHUD->CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
	}
}

void ABlasterPlayerController::SetTextWeaponType(EWeaponType WeaponType)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay)
	{
		switch (WeaponType)
		{
		case EWeaponType::EWT_AssaultRifle:
			BlasterHUD->CharacterOverlay->WeaponType->SetText(FText::FromString("Rifle"));
			BlasterHUD->CharacterOverlay->WeaponType->SetVisibility(ESlateVisibility::Visible);
			break;
		}		
	}
}

void ABlasterPlayerController::SetTextWeaponTypeInvisible()
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	if (BlasterHUD && BlasterHUD->CharacterOverlay)
	{
		BlasterHUD->CharacterOverlay->WeaponType->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

	if (BlasterHUD && BlasterHUD->CharacterOverlay
		&& BlasterHUD->CharacterOverlay->MatchCountDownText)
	{
		if (CountdownTime < 0.0f)
		{
			BlasterHUD->CharacterOverlay->MatchCountDownText->SetText(FText());
		}
		else
		{
			int32 Mins = FMath::FloorToInt(CountdownTime / 60.0f);
			int32 Secs = FMath::FloorToInt(CountdownTime) % 60;

			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Mins, Secs);

			if (Mins == 0 && Secs <= 30 && bIsTimerSet == false)
			{
				bIsTimerSet = true;
				GetWorldTimerManager().SetTimer(BlinkingTimer, this, &ABlasterPlayerController::TextBlinking, 0.5f, true);
				BlasterHUD->CharacterOverlay->MatchCountDownText->SetColorAndOpacity(FLinearColor(255, 0, 0, 0.5));
			}

			BlasterHUD->CharacterOverlay->MatchCountDownText->SetText(FText::FromString(CountdownText));
		}		
	}
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
	if (MatchState == MatchState::InProgress)
	{
		BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD)
		{
			BlasterHUD->AddCharacterOverlay();

			if (BlasterHUD->Announcement)
			{
				BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	if (MatchState == MatchState::Cooldown)
	{
		BlasterHUD = !BlasterHUD ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			BlasterHUD->CharacterOverlay->RemoveFromParent();

			if (BlasterHUD->Announcement && BlasterHUD->Announcement->AnnouncementText
				&& BlasterHUD->Announcement->InfoText)
			{
				BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
				FString AnnouncementText("New Match Starts In:");
				BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

				BlasterHUD->CharacterOverlay->MatchCountDownText->SetColorAndOpacity(FLinearColor(0, 0, 0, 0));
				bIsTimerSet = false;
				GetWorldTimerManager().ClearTimer(BlinkingTimer);

				ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
				ABlasterPlayerState* BlasterPlayerState = Cast<ABlasterPlayerState>(this->PlayerState);

				if (BlasterGameState && BlasterPlayerState)
				{
					TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
					FString InfoTextString;
					if (TopPlayers.Num())
					{
						InfoTextString = FString("There is no winner");
					}
					else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
					{
						InfoTextString = FString("You are the winner!");
					}
					else if (TopPlayers.Num() == 1)
					{
						InfoTextString = FString::Printf(TEXT("Winner: /n%s"), *TopPlayers[0]->GetPlayerName());
					}
					else if (TopPlayers.Num() > 1)
					{
						InfoTextString = FString::Printf(TEXT("Players tied for the win: /n"));
						for (auto TiedPlayer : TopPlayers)
						{
							InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
						}
					}
					BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
				}				
			}
		}

		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
		if (BlasterCharacter && BlasterCharacter->GetCombat())
		{
			BlasterCharacter->bDisableGameplay = true;
			BlasterCharacter->GetCombat()->FireButtonPressed(false);
		}
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	if (!GetWorld())
	{
		return;
	}

	if (HasAuthority())
	{
		BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
		{
			LevelStartingTime = BlasterGameMode->LevelStartingTime;
		}
	}

	float TimeLeft = 0.0f;
	if (MatchState == MatchState::WaitingToStart)
	{
		TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::InProgress)
	{
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	else if (MatchState == MatchState::Cooldown)
	{
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	}
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = !BlasterGameMode ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
		if (BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountDown(TimeLeft);
		}
		else if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}		
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, LevelStartingTime, MatchTime, CooldownTime);
	}
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (RoundTripTime * 0.5f);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float LevelStarting, float Match, float Cooldown)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	MatchState = StateOfMatch;
	CooldownTime = Cooldown;
	LevelStartingTime = LevelStarting;
	OnMatchStateSet(MatchState);

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}

	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::TextBlinking()
{
	static int Counter = 0;
	if (Counter % 2 == 0)
	{
		BlasterHUD->CharacterOverlay->MatchCountDownText->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		BlasterHUD->CharacterOverlay->MatchCountDownText->SetVisibility(ESlateVisibility::Visible);
	}

	Counter += 1;
}