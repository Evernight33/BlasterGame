// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/BlasterHUD.h"
#include "Blaster/Public/Weapon/WeaponTypes.h"
#include "Blaster/Public/BlasterTypes/CombatState.h"
#include "Components/TimelineComponent.h"
#include "CombatComponent.generated.h"

class ABaseWeapon;
class ABlasterCharacter;
class ABlasterPlayerController;
class ABlasterHUD;
class AProjectile;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	friend class ABlasterCharacter;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> GrenadeClass;

	FVector HitTarget;

	FTimerHandle KnifeStabTimer;

	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(ABaseWeapon* WeaponToEquip);
	void FireButtonPressed(bool bPressed);
	void Reload();
	void ThrowGrenade();
	void KnifeStab();
	void KnifeStabTimerFunction();

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();
	
	void JumpToShotgunEnd();
	void ShowAttachedGrenade(bool bVisible);
	void ShowAttachedKnife(bool bVisible);
	void PickupAmmo(EWeaponType WeaponType, int32 AmmoAmount);

	FORCEINLINE int32 GetGrenades() const { return Grenades; }
	FORCEINLINE void SetGrenades(int32 GrenadesAmount) { Grenades = GrenadesAmount; }
	FORCEINLINE ABaseWeapon* GetEquippedWeapon() { return EquippedWeapon; }

protected:	
	virtual void BeginPlay() override;
	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	UFUNCTION()
	void OnRep_CarryAmmo();

	UFUNCTION()
	void OnRep_CombatState();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade(const FVector_NetQuantize& Target);

	UFUNCTION(Server, Reliable)
	void ServerKnifeStab();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void ThrowGrenadeFinished();
	void KnifeStabFinished();

	void HandleReload();

	void DropEquippedWeapon() const;
	void AttachActorToHand(AActor* ActorToAttach, const FName& HandName);
	void UpdateCarryAmmo();
	void PlayEquippedWeaponSound();
	void ReloadEmptyWeapon();

	int32 AmountToReload();

private:
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	ABaseWeapon* EquippedWeapon;

	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	ABlasterPlayerController* Controller;

	UPROPERTY()
	ABlasterHUD* HUD;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	UPROPERTY(EditDefaultsOnly)
	float TextDelay = 0.05f;

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	bool bFireButtonPressed;

	FTimerHandle EquipTimer;

	/*
	* Hud
	*/	
	FHUDPackage HUDPackage;

	float CrosshairTraceLength = 8000.f;
	float CrosshairVelocityFactor;
	float CrosshairAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	float CrosshairEnemyFactor;

	/*
	* Aiming and FOV
	*/
	float DefaultFOV;
	float CurrentFOV;
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	bool bCanfire = true;

	// Carried ammo for the currently equipped weapon
	UPROPERTY(ReplicatedUsing = OnRep_CarryAmmo)
	int32 CarryAmmo;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 MaxGrenades = 4;

	UPROPERTY(ReplicatedUsing = OnRep_Grenades)
	int32 Grenades;

	FTimerHandle FireTimer;

	TMap<EWeaponType, int32> CarryAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 MaxCarryAmmo = 500;

	UFUNCTION()
	void OnRep_Grenades();

	void InterpFOV(float DeltaTime);

	void StartFireTimer();
	void FireTimerFinished();
	void Fire();
	bool CanFire();
	void InitializeCarryAmmo();
	void UpdateAmmoValues();
	void UpdateShotunAmmoValues();
	void SetHudText();
	void UpdateHudGrenades();
};
