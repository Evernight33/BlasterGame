// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNotifies/ThrowGrenadeNotify.h"
#include "Blaster/Public/Character/BlasterCharacter.h"
#include "Blaster/Public/BlasterComponents/CombatComponent.h"

void UThrowGrenadeNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* AnimSequance)
{
	if (AActor* BlasterActor = MeshComp->GetOwner())
	{
		if (ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(BlasterActor))
		{
			if (BlasterCharacter->HasAuthority() && BlasterCharacter->GetCombat())
			{
				BlasterCharacter->GetCombat()->ShowAttachedGrenade(false);
				BlasterCharacter->MulticastThrowGrenade();
			}
		}
	}
}