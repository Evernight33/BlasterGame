// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/RocketMovementComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Sound/SoundCue.h"
#include "Blaster/Public/BlasterComponents/LagCompensationComponent.h"
#include "Blaster/Public/BlasterPlayerController.h"
#include "Components/AudioComponent.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
	RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}	

	SpawnTrailSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.0f,
			LoopingSoundAttenuation,
			(USoundConcurrency*) nullptr,
			false
		);
	}
}

void AProjectileRocket::Destroyed()
{
	AActor::Destroyed();
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner() || !GetWorld())
	{
		return;
	}

	ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
	APawn* FiringPawn = GetInstigator();
	AController* FiringController = FiringPawn == nullptr ? nullptr : FiringPawn->GetController();
	ABlasterPlayerController* BlasterController = nullptr;
	if (FiringController)
	{
		BlasterController = Cast<ABlasterPlayerController>(FiringController);
	}

	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && FiringController /* && bUseServerSideRewind*/ && OwnerCharacter->GetLagCompensation() && !HasAuthority())
	{
		TArray<FOverlapResult> Overlaps;

		FCollisionShape Sphere = FCollisionShape::MakeSphere(DamageInnerRadius * 1.1f);
		GetWorld()->OverlapMultiByObjectType(
			Overlaps, 
			GetActorLocation(),
			FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn),
			Sphere
		);

		TArray<ABlasterCharacter*> CharactersToRewind;
		for (auto& Overlap : Overlaps)
		{
			ABlasterCharacter* Character = Cast<ABlasterCharacter>(Overlap.GetActor());

			if (Character)
			{
				CharactersToRewind.Add(Character);
			}		
		}

		OwnerCharacter->GetLagCompensation()->ProjectileRocketServerScoreRequest(CharactersToRewind,
			this, 
			BlasterController->GetServerTime() - BlasterController->SingleTripTime);
	}
	else
	{
		ExplodeDamage();
	}

	StartDestroyTimer();

	if (ImpactParticles && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false);
	}

	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}

	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();
	}
}