// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterPlayerState.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Defeats();

	UFUNCTION()
	virtual void OnRep_ElimText();

	void AddToScore(float ScoreAmount, bool IsInit = false);
	void AddToDefeats(int32 DefeatsAmount);
	void ElimTextVisibility(bool IsVisible);

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* Controller;	

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;	

	UPROPERTY(ReplicatedUsing = OnRep_ElimText)
	bool IsVisibleElimText;
};
