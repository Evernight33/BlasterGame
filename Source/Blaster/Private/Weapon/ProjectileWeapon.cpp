// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);
		
	if (GetOwner())
	{
		APawn* InstigatorPawn = Cast<APawn>(GetOwner());
		if (USkeletalMeshComponent* WeaponMeshSuper = GetWeaponMesh())
		{
			UWorld* World = GetWorld();
			const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMeshSuper->GetSocketByName(FName("MuzzleFlash"));
			if (World && MuzzleFlashSocket)
			{
				FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMeshSuper);

				FVector ProjectileSpawnLocation = SocketTransform.GetLocation();

				// Vector from weapon muzzle to hit location
				FVector ToTarget = HitTarget - ProjectileSpawnLocation;
				FRotator TargetRotation = ToTarget.Rotation();

				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = GetOwner();
				SpawnParams.Instigator = InstigatorPawn;

				AProjectile* SpawnedProjectile = nullptr;
				if (bUseServerSideRewind)
				{
					if (InstigatorPawn->HasAuthority())
					{
						if (InstigatorPawn->IsLocallyControlled()) // server, host - use replicated projectile
						{
							SpawnedProjectile =
							World->SpawnActor<AProjectile>(
								ProjectileClass,
								ProjectileSpawnLocation,
								TargetRotation,
								SpawnParams);

							SpawnedProjectile->bUseServerSideRewind = false;
							SpawnedProjectile->Damage = Damage;
						}
						else // server, not locally controlled - spawn non-replicated projectile, no SSR
						{
							SpawnedProjectile =
								World->SpawnActor<AProjectile>(
									ServerSideRewindProjectileClass,
									ProjectileSpawnLocation,
									TargetRotation,
									SpawnParams);
							SpawnedProjectile->bUseServerSideRewind = false;
						}
					}
					else // client, using SSR
					{
						if (InstigatorPawn->IsLocallyControlled()) // client, locally controlled - spawn non-replicated projectile, use SSR
						{
							SpawnedProjectile =
								World->SpawnActor<AProjectile>(
									ServerSideRewindProjectileClass,
									ProjectileSpawnLocation,
									TargetRotation,
									SpawnParams);
							SpawnedProjectile->bUseServerSideRewind = true;
							SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
							SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
							SpawnedProjectile->Damage = Damage;
						}
						else // client, not locally controlled, spawn non-replicated projectile, no SSR
						{
							SpawnedProjectile =
								World->SpawnActor<AProjectile>(
									ServerSideRewindProjectileClass,
									ProjectileSpawnLocation,
									TargetRotation,
									SpawnParams);
							SpawnedProjectile->bUseServerSideRewind = false;
						}
					}
				}
				else // weapon not using SSR
				{
					if (InstigatorPawn->HasAuthority())
					{
						SpawnedProjectile =
							World->SpawnActor<AProjectile>(
								ProjectileClass,
								ProjectileSpawnLocation,
								TargetRotation,
								SpawnParams);

						SpawnedProjectile->bUseServerSideRewind = false;
						SpawnedProjectile->Damage = Damage;
					}
				}
			}
		}
	}
}