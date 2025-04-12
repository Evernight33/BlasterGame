// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileGrenade.generated.h"

class USoundCue;

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	UStaticMesh* GrenadeDefaultMesh;

	UPROPERTY(EditAnywhere)
	UStaticMesh* GrenadeThrownMesh;

	AProjectileGrenade();

	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
