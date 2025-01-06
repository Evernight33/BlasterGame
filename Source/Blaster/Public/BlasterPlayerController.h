// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"
#include "BlasterPlayerController.generated.h"

class ABlasterHUD;
class UCharacterOverlay;
class ABlasterGameMode;

UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void SetHUDHealth(float CurrentHealth, float MaxHealth);
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDAnnouncementCountDown(float CountdownTime);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetElimTextVisibility(bool IsVisible);
	void SetTextWeaponType(EWeaponType WeaponType);
	void SetTextWeaponTypeInvisible();
	void SetHUDMatchCountdown(float CountdownTime);
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReceivedPlayer() override; // Syync with server clock as soon as possible
	virtual float GetServerTime(); // Sync with Server world clock

protected:
	virtual void BeginPlay() override;

	void SetHUDTime();
	void PollInit();
	/**
	 * Sync time between client and server
	 */

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	UFUNCTION(Server, Reliable)
	void ServerCheckMatchState();

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	UFUNCTION(Client, Reliable)
	void ClientJoinMidGame(FName StateOfMatch, float Warmup, float LevelStarting, float Match, float Cooldown);

	void CheckTimeSync(float DeltaTime);	

	float ClientServerDelta = 0.0f; // Difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.0f;

	float TimeSyncRunningTime = 0.0f;

private:
	UPROPERTY()
	ABlasterHUD* BlasterHUD;
	
	float LevelStartingTime = 0.0f;
	float MatchTime = 0.0f;
	float WarmupTime = 0.0f;
	float CooldownTime = 0.0f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;

	UFUNCTION()
	void OnRep_MatchState();
};
