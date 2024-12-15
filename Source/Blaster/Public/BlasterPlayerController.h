// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float CurrentHealth, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetElimTextVisibility(bool IsVisible);
	void SetTextWeaponType(EWeaponType WeaponType);
	void SetTextWeaponTypeInvisible();
	void SetHUDMatchCountdown(float CountdownTime);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();

private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	float MatchTime = 120.0f;
	uint32 CountdownInt = 0;
};
