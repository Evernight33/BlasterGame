// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "BlasterGameMode.generated.h"

class ABlasterCharacter;
class ABlasterPlayerController;
class ACharacter;

UCLASS()
class BLASTER_API ABlasterGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ABlasterGameMode();

	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController);

	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 3.0f;

	float LevelStartingTime = 0.0f;

private:
	float CountDownTime = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;
};
