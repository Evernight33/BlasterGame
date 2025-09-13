// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBuffComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void Heal(float HealAmount, float HealingTime);
	void ReplenishShield(float ShieldAmount, float ReplenishTime);
	void BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime);
	void BuffDamageMultiplier(float BuffTime);
	void BuffJump(float BuffJumpVelocity, float BuffTime);
	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float JumpVelocity);

	friend class ABlasterCharacter;

protected:
	virtual void BeginPlay() override;
	void HealRampUp(float DeltaTime);
	void ShieldRampUp(float DeltaTime);

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	/**
	* Speed buff
	*/

	bool bHealing = false;
	float HealingRate = 0;
	float AmountToHeal = 0.f;

	/**
	* Shield buff
	*/

	bool bReplenishingShield = false;
	float ShieldReplenishRate = 0;
	float ShieldReplenishAmount = 0.f;

	/**
	* Speed buff
	*/

	FTimerHandle SpeedBuffTimer;	
	void ResetSpeeds();

	float InitialBaseSpeed;
	float InitialCrouchSpeed;

	/**
	* Jump buff
	*/
	
	FTimerHandle JumpBuffTimer;
	void ResetJump();	
	float InitialJumpVelocity;
	
	/**
	* Damage multiplier buff
	*/

	FTimerHandle DamageMultiplierBuffTimer;
	void ResetDamage();
	float DamageMultiplier;

};
