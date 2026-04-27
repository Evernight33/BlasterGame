// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ElimAnnouncement.h"
#include "Components/TextBlock.h"


void UElimAnnouncement::SetEliminationAnnouncementText(FString AttackerName, FString VictimName)
{
	FString EliminationAnnouncementText = FString::Printf(TEXT("%s elimmed %s!"), *AttackerName, *VictimName);
	if (AnnouncementText)
	{
		AnnouncementText->SetText(FText::FromString(EliminationAnnouncementText));
	}
}