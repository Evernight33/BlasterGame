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
			if (const USkeletalMeshSocket* MuzzleFlashSocket = WeaponMeshSuper->GetSocketByName(FName("MuzzleFlash")))
			{
				FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(WeaponMeshSuper);

				// Vector from weapon muzzle to hit location
				FVector ToTarget = HitTarget - SocketTransform.GetLocation();
				FRotator TargetRotation = ToTarget.Rotation();

				if (ProjectileClass && InstigatorPawn)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = GetOwner();
					SpawnParams.Instigator = InstigatorPawn;

					if (UWorld* World = GetWorld())
					{
						World->SpawnActor<AProjectile>(
							ProjectileClass,
							SocketTransform.GetLocation(),
							TargetRotation,
							SpawnParams);
					}
				}
			}
		}
	}
}