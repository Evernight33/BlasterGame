// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/Projectile.h"
#include "ProjectileRocket.generated.h"

class URocketMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()
public:
	virtual void Destroyed() override;

	AProjectileRocket();

	UPROPERTY(VisibleAnywhere)
	URocketMovementComponent* RocketMovementComponent;

protected:
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void BeginPlay() override;

	void DestroyTimerFinished();

	UPROPERTY(EditAnywhere)
	USoundCue* ProjectileLoop;

	UPROPERTY(EditAnywhere)
	USoundAttenuation* LoopingSoundAttenuation;

	UPROPERTY()
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UAudioComponent* ProjectileLoopComponent;	

private:	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RocketMesh;

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	FTimerHandle DestroyTimer;
	float DestroyTime = 3.0f;
};
