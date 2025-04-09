// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/Public/BlasterTypes/TurningInPlace.h"
#include "Blaster/Public/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/Public/InteractWithCrosshairsInterface.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"


class UWidgetComponent;
class ABaseWeapon;
class UCombatComponent;
class UAnimMontage;
class UCameraComponent;
class ABlasterPlayerController;
class AController;
class USoundCue;
class ABlasterPlayerState;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

private:
	void TurnInPlace(float DeltaTime);
	void TryToHideACamera();
	void PlayHitReactMontage();

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;
	virtual void Destroyed() override;

	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void SetOverlappingWeapon(ABaseWeapon* Weapon);
	void PlayElimintationMontage();
	void PlayThrowGrenadeMontage();
	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();	

	bool IsWeaponEquipped();
	bool IsAiming();

	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool GetShouldRotateRootBone() { return bRotateRootBone; };
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	ECombatState GetCombatState() const;

	ABaseWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonRealeased();
	void AimOffset(float DeltaTime);
	void SimProxiesTurn();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser);

	void UpdateHUDHealth();

	//Poll for any relevant classes and initialize our HUD
	void PollInit();

	void RotateInPlace(float DeltaTime);

	virtual void Jump() override;
	
	void FireButtonPressed();
	void FireButtonReleased();

	void ReloadButtonPressed();
	void GrenadeButtonPressed();

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	ABaseWeapon* OverlappingWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCombatComponent* Combat;

	void CalculateAO_Pitch();
	float CalculateSpeed();

	/**
	* Player health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	/**
	* Dissolve
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	//Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//Material instance set by default in BP
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/*
	* Elimination bot
	*/

	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	USoundCue* ElimBotSound;

	bool IsFirstEliminationCall = true;

	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;

	/*
	* Grenade
	*/

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	UFUNCTION()
	void OnRep_OverlappingWeapon(ABaseWeapon* LastWeapon);

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	void EliminateTimerFinished();

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	float AO_Yaw;
	float AO_Pitch;
	float InterpAO_Yaw;
	
	bool bRotateRootBone;
	bool bEliminated;
	float TurnThreshold = 0.5f;

	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotationCurrentFrame;

	float ProxyYaw;
	float TimeSinceLastMovementRep;

	FTimerHandle EliminateTimer;

	UPROPERTY()
	ABlasterPlayerController* BlasterPlayerController;

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.0f;

	UPROPERTY(EditDefaultsOnly)
	float EliminateDelay = 3.0f;

	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadSniperRifleMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* EliminationMontage;	

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;
};
