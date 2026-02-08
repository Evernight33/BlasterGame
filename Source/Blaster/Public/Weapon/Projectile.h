// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundCue;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* ProjectileMesh;

	/*
	* Used with server side rewind
	*/

	UPROPERTY(EditAnywhere)
	float InitialSpeed = 15000.f;

	float Damage = 20.f;

	FVector_NetQuantize TraceStart;
	FVector_NetQuantize100 InitialVelocity;
	bool bUseServerSideRewind = false;

	AProjectile();

protected:
	virtual void BeginPlay() override;

	void StartDestroyTimer();
	void DestroyTimerFinished();
	void SpawnTrailSystem();
	void ExplodeDamage();

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;

	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* TrailSystem;

	UPROPERTY()
	UNiagaraComponent* TrailSystemComponent;

	UPROPERTY(EditAnywhere)
	float DamageInnerRadius = 200.0;

	UPROPERTY(EditAnywhere)
	float DamageOuterRadius = 200.0;

private:
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;

	UPROPERTY()
	UParticleSystemComponent* TracerComponent;	

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactCharacterParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactStoneParticles;	

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactCharacterSound;

	UPROPERTY(EditAnywhere)
	float DestroyTime = 3.0f;

	FTimerHandle DestroyTimer;

	bool bImpactCharacter = false;
	bool bImpactStone = false;

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
};
