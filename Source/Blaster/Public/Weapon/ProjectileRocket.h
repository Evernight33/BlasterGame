// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	AProjectileRocket();

	UPROPERTY(VisibleAnywhere)
	URocketMovementComponent* RocketMovementComponent;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;

private:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;
};
