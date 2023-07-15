// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/OverheadWidget.h"
#include "Components/TextBlock.h"
#include "UObject/Object.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (InPawn)
	{
		ENetRole RemoteRole = InPawn->GetRemoteRole();
		FString Role;

		switch (RemoteRole)
		{
		case ENetRole::ROLE_Authority:
			Role = FString("Authority");
			break;
		case ENetRole::ROLE_AutonomousProxy:
			Role = FString("AutonomousProxy");
			break;
		case ENetRole::ROLE_SimulatedProxy:
			Role = FString("SimulatedProxy");
			break;
		case ENetRole::ROLE_None:
			Role = FString("SNone");
			break;
		}

		FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
		SetDisplayText(RemoteRoleString);
	}
}

void UOverheadWidget::ShowPlayersName(APawn* InPawn)
{
	if (InPawn)
	{
		APlayerState* PlayerState = InPawn->GetPlayerState<APlayerState>();
		
		if (PlayerState)
		{
			FString PlayerName = PlayerState->GetPlayerName();
			SetDisplayText(PlayerName);
		}
	}
}

void UOverheadWidget::NativeDestruct()
{
	RemoveFromParent();
	Super::NativeDestruct();
}