// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/ThrowGrenadeNotify.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"
#include "Blaster/Public/Weapon/Projectile.h"
#include "Blaster/Public/Weapon/ProjectileGrenade.h"

void UThrowGrenadeNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSequance)
{
	if (AActor* BlasterActor = MeshComp->GetOwner())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(BlasterActor))
		{
			if (UCombatComponent* Combat = BlasterCharacter->GetCombat())
			{
				if (Combat->GrenadeClass && BlasterCharacter->HasAuthority() && BlasterCharacter->GetAttachedGrenade())
				{
					BlasterCharacter->GetCombat()->ShowAttachedGrenade(false);
					BlasterCharacter->MulticastThrowGrenade();

					const FVector StartingLocation = BlasterCharacter->GetAttachedGrenade()->GetComponentLocation();
					FVector ToTarget = Combat->HitTarget - StartingLocation;
					FActorSpawnParameters SpawnParams;
					SpawnParams.Owner = BlasterCharacter;
					SpawnParams.Instigator = BlasterCharacter;
					UWorld* World = GetWorld();

					if (World)
					{
						AProjectileGrenade* ProjectileGrenade = World->SpawnActor<AProjectileGrenade>(
							Combat->GrenadeClass,
							StartingLocation,
							ToTarget.Rotation(),
							SpawnParams);
					}
				}				
			}
		}
	}
}